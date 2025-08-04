// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2019 - 2024 Intel Corporation.

#include <linux/acpi.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>

#include <media/v4l2-cci.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fwnode.h>
#include <linux/fs.h>
#include <linux/firmware.h>
#include <linux/regmap.h>
#include <linux/unaligned.h>

#define AR0830_ANALOG_GAIN_MIN		0
#define AR0830_ANALOG_GAIN_MAX		0x16
#define AR0830_ANALOG_GAIN_STEP		1
#define AR0830_ANALOG_GAIN_DEFAULT	0xe

#define AR0830_GLOBAL_GAIN_MIN		1
#define AR0830_GLOBAL_GAIN_MAX		0x162
#define AR0830_GLOBAL_GAIN_STEP		1
#define AR0830_GLOBAL_GAIN_DEFAULT	0x80

#define AR0830_COMMON_WIDTH			3840
#define AR0830_COMMON_HEIGHT		2160

#define AR0830_PIXEL_RATE		600000000ULL

#define BOOTDATA "NewBootdata.txt"
#define MAX_FIRMWARE_SIZE 			(1024*1024)
#define FIRMWARE_BLOCK_SIZE 		256
#define MAX_REGISTERS_PER_BLOCK 	50
#define STALL 1

#define SYS_START							0x601A
#define SYS_START_STALL_MODE_FRAME			(0U << 6)
#define SYS_START_STALL_MODE_DISABLED		(1U << 6)
#define SYS_START_STALL_MODE_ALL_POWER_DOWN	(3U << 6)
#define SYS_START_STALL_EN					BIT(8)
#define SYS_START_START_STREAM				(SYS_START_STALL_EN | SYS_START_STALL_MODE_FRAME)
#define SYS_START_STOP_ALL_STREAM			(SYS_START_STALL_EN | SYS_START_STALL_MODE_ALL_POWER_DOWN)

#define AR0830_REG_VALUE_16BIT			2
#define AR0830_REG_VALUE_32BIT			4
#define OFFSET_BASE 			6
#define OFFSET_STEP 			5

#define REG_VALUE_4			4
#define REG_VALUE_2			2
#define REG_VALUE_1			1

#define to_ar0830(_sd)	container_of(_sd, struct ar0830, sd)

static const struct regmap_config sensor_regmap_config = {
	.reg_bits = 16,
	.val_bits = 16,
};

struct ar0830_mode {
	u32 width;
	u32 height;
	u32 code;

};

static const s64 link_freq_menu_items[] = {
	600000000ULL, /* 600M */
};

static const struct ar0830_mode supported_modes[] = {
	{
		.width = AR0830_COMMON_WIDTH,
		.height = AR0830_COMMON_HEIGHT,
		.code = MEDIA_BUS_FMT_UYVY8_1X16,
	},
};

struct ar0830 {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_ctrl_handler ctrl_handler;

	/* V4L2 Controls */
	struct v4l2_ctrl *link_freq;

	struct regmap *regmap;
	unsigned long link_freq_bitmap;
	const struct ar0830_mode *cur_mode;
	struct gpio_desc *reset_gpio;

	/* firmware */
	const struct firmware *firmware;
};

static int ar0830_read_reg(struct ar0830 *ar0830, u16 reg, u16 len, u32 *val)
{
	struct i2c_client *client = v4l2_get_subdevdata(&ar0830->sd);
	struct i2c_msg msgs[2];
	u8 addr_buf[2];
	u8 data_buf[4] = {0};
	int ret;

	if (len > 4)
		return -EINVAL;

	put_unaligned_be16(reg, addr_buf);
	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = sizeof(addr_buf);
	msgs[0].buf = addr_buf;
	msgs[1].addr = client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = len;
	msgs[1].buf = &data_buf[4 - len];

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret != ARRAY_SIZE(msgs))
		return -EIO;

	*val = get_unaligned_be32(data_buf);

	return 0;
}
static void check_val(struct ar0830 *ar0830, u16 len, u16 addr, u8 *data)
{
	struct i2c_client *client = v4l2_get_subdevdata(&ar0830->sd);
	u32 reg_value_check;
	int ret;

	ret = ar0830_read_reg(ar0830, addr, len, &reg_value_check);
	if (ret) {
		dev_err(&client->dev, "failed to read 0x%04x: %d", addr, ret);
		return;
	} else {
		if (len == 1)
			dev_dbg(&client->dev, "0x%04x read value: 0x%x, write value: 0x%x, %s\n", addr, reg_value_check, data[0], reg_value_check == data[0] ? "OK" : "FAIL");
		if (len == 2)
			dev_dbg(&client->dev, "0x%04x read value: 0x%x%x, write value: 0x%x%x, %s\n", addr,
			       (reg_value_check >> 8) & 0xff, reg_value_check & 0xff,
			       data[0], data[1], reg_value_check == ((data[0] << 8) | data[1]) ? "OK" : "FAIL");
		if (len == 4)
			dev_dbg(&client->dev, "0x%04x read value: 0x%x%x%x%x, write value: 0x%x%x%x%x, %s\n", addr,
			       (reg_value_check >> 24) & 0xff,
			       (reg_value_check >> 16) & 0xff,
			       (reg_value_check >> 8) & 0xff,
			       reg_value_check & 0xff,
			       data[0], data[1], data[2], data[3], reg_value_check == ((data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]) ? "OK" : "FAIL");
	}

	return;
}
static int ar0830_write_reg(struct ar0830 *ar0830, u16 reg, u8 *data, u8 length)
{
    struct i2c_client *client = v4l2_get_subdevdata(&ar0830->sd);
    struct i2c_msg msg;
    u8 buf[6];
    int ret;

    if (length > 4) {
        dev_err(&client->dev, "Invalid length: %d\n", length);
        return -EINVAL;
    }

    /* Prepare the buffer: register address (big-endian) + data */
    buf[0] = reg >> 8;       /* High byte of register address */
    buf[1] = reg & 0xFF;     /* Low byte of register address */

	/* Copy the data into the buffer */
    memcpy(&buf[2], data, length);

    /* Prepare the I2C message */
    msg.addr = client->addr;
    msg.flags = 0;
    msg.len = length + 2;
    msg.buf = buf;

    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret != 1) {
        dev_err(&client->dev, "Failed to write register 0x%04x, ret: %d\n", reg, ret);
        return -EIO;
    }
    dev_dbg(&client->dev, "Successfully wrote to register 0x%04x, data: %*ph\n", reg, length, data);
    return 0;
}

static int ar0830_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct ar0830 *ar0830 =
		container_of(ctrl->handler, struct ar0830, ctrl_handler);
	struct i2c_client *client = v4l2_get_subdevdata(&ar0830->sd);
	struct v4l2_subdev_state *state;
	const struct v4l2_mbus_framefmt *format;
	int ret;

	state = v4l2_subdev_get_locked_active_state(&ar0830->sd);
	format = v4l2_subdev_state_get_format(state, 0);

	/* V4L2 controls values will be applied only when power is already up */
	if (!pm_runtime_get_if_in_use(&client->dev))
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_ANALOGUE_GAIN:
	case V4L2_CID_DIGITAL_GAIN:
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_VBLANK:
	case V4L2_CID_HFLIP:
	case V4L2_CID_VFLIP:
	case V4L2_CID_TEST_PATTERN:
		ret = 0;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	pm_runtime_put(&client->dev);

	return ret;
}

static const struct v4l2_ctrl_ops ar0830_ctrl_ops = {
	.s_ctrl = ar0830_set_ctrl,
};

static int ar0830_s_reset(struct ar0830 *ar0830, int on)
{
	struct i2c_client *client = v4l2_get_subdevdata(&ar0830->sd);
	int ret;

	if (on) {
		ret = pm_runtime_resume_and_get(&client->dev);
		if (ret < 0)
			return ret;
		/* exit RESET */
		gpiod_set_value_cansleep(ar0830->reset_gpio, 1);
	} else {
		/* enter reset */
		gpiod_set_value_cansleep(ar0830->reset_gpio, 0);

		pm_runtime_put(&client->dev);
	}
	return 0;
}

static int ar0830_init_controls(struct ar0830 *ar0830)
{
	struct i2c_client *client = v4l2_get_subdevdata(&ar0830->sd);
	struct v4l2_fwnode_device_properties props;
	struct v4l2_ctrl_handler *ctrl_hdlr;
	u32 link_freq_size;
	int ret;

	ctrl_hdlr = &ar0830->ctrl_handler;
	ret = v4l2_ctrl_handler_init(ctrl_hdlr, 10);
	if (ret)
		return ret;

	link_freq_size = ARRAY_SIZE(link_freq_menu_items) - 1;
	ar0830->link_freq = v4l2_ctrl_new_int_menu(ctrl_hdlr,
						   &ar0830_ctrl_ops,
						   V4L2_CID_LINK_FREQ,
						   link_freq_size, 0,
						   link_freq_menu_items);
	if (ar0830->link_freq)
		ar0830->link_freq->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	v4l2_ctrl_new_std(ctrl_hdlr, &ar0830_ctrl_ops, V4L2_CID_ANALOGUE_GAIN,
			  AR0830_ANALOG_GAIN_MIN, AR0830_ANALOG_GAIN_MAX,
			  AR0830_ANALOG_GAIN_STEP, AR0830_ANALOG_GAIN_DEFAULT);
	v4l2_ctrl_new_std(ctrl_hdlr, &ar0830_ctrl_ops, V4L2_CID_DIGITAL_GAIN,
			  AR0830_GLOBAL_GAIN_MIN, AR0830_GLOBAL_GAIN_MAX,
			  AR0830_GLOBAL_GAIN_STEP, AR0830_GLOBAL_GAIN_DEFAULT);
	v4l2_ctrl_new_std(ctrl_hdlr, &ar0830_ctrl_ops, V4L2_CID_PIXEL_RATE,
			  AR0830_PIXEL_RATE, AR0830_PIXEL_RATE, 1,
			  AR0830_PIXEL_RATE);
	
	if (ctrl_hdlr->error)
		return ctrl_hdlr->error;

	ret = v4l2_fwnode_device_parse(&client->dev, &props);
	if (ret)
		return ret;

	ret = v4l2_ctrl_new_fwnode_properties(ctrl_hdlr, &ar0830_ctrl_ops,
					      &props);
	if (ret)
		return ret;

	ar0830->sd.ctrl_handler = ctrl_hdlr;

	return 0;
}

static void ar0830_update_pad_format(const struct ar0830_mode *mode,
				     struct v4l2_mbus_framefmt *fmt)
{
	fmt->width = mode->width;
	fmt->height = mode->height;
	fmt->code = mode->code;
	fmt->field = V4L2_FIELD_NONE;
}
static int ar0830_stall(struct ar0830 *ar0830, int stall_en)
{
	int ret = 0;

	dev_info(ar0830->sd.dev, "stall: %s\n", (stall_en) ? "Enable" : "Disable");

	if (stall_en) { /* true: probe, stop streaming */
		ret = cci_write(ar0830->regmap, CCI_REG16(SYS_START), SYS_START_STOP_ALL_STREAM, NULL);
	} else {        /* false: start streaming */
		ret = cci_write(ar0830->regmap, CCI_REG16(SYS_START), SYS_START_START_STREAM, NULL);
	}

	if (ret < 0) {
		dev_err(ar0830->sd.dev, "regmap_write error (address 0x%x)\n", SYS_START);
		return ret;
	}

	usleep_range(100, 200);

	return ret;
}
static int ar0830_start_streaming(struct ar0830 *ar0830)
{
	struct i2c_client *client = v4l2_get_subdevdata(&ar0830->sd);
	int ret;

	ret = pm_runtime_resume_and_get(&client->dev);
	if (ret < 0)
		return ret;

#if STALL
	ret = ar0830_stall(ar0830, false);
	if (ret < 0) {
		dev_err(&client->dev, "failed to stall");
		return ret;
	}
#endif

	ret = __v4l2_ctrl_handler_setup(ar0830->sd.ctrl_handler);
	if (ret)
		goto err_rpm_put;

	return 0;

err_rpm_put:
	pm_runtime_put(&client->dev);
	return ret;
}

static int ar0830_stop_streaming(struct ar0830 *ar0830)
{
	int ret;
	struct i2c_client *client = v4l2_get_subdevdata(&ar0830->sd);

	/* keep as a reference */
	/*
	u64 value;
	ret = cci_read(ar0830->regmap, CCI_REG16(0x0002), &value, NULL); // frame_cnt
	*/
#if STALL
	ret = ar0830_stall(ar0830, true);
	if (ret < 0) {
		dev_err(&client->dev, "failed to stall");
		return ret;
	}
#endif

	pm_runtime_put(&client->dev);
	return 0;
}

static int ar0830_set_stream(struct v4l2_subdev *sd, int enable)
{
	struct ar0830 *ar0830 = to_ar0830(sd);
	struct v4l2_subdev_state *state;
	int ret = 0;

	state = v4l2_subdev_lock_and_get_active_state(sd);

	if (enable)
		ret = ar0830_start_streaming(ar0830);
	else
		ret = ar0830_stop_streaming(ar0830);

	v4l2_subdev_unlock_state(state);

	return ret;
}

static int ar0830_set_format(struct v4l2_subdev *sd,
			     struct v4l2_subdev_state *sd_state,
			     struct v4l2_subdev_format *fmt)
{
	struct ar0830 *ar0830 = to_ar0830(sd);
	const struct ar0830_mode *mode;

	mode = v4l2_find_nearest_size(supported_modes,
				      ARRAY_SIZE(supported_modes),
				      width, height,
				      fmt->format.width,
				      fmt->format.height);

	ar0830_update_pad_format(mode, &fmt->format);
	*v4l2_subdev_state_get_format(sd_state, fmt->pad) = fmt->format;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		return 0;

	ar0830->cur_mode = mode;

	return 0;
}

static int ar0830_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *sd_state,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index > 0)
		return -EINVAL;

	code->code = MEDIA_BUS_FMT_UYVY8_1X16;

	return 0;
}

static int ar0830_enum_frame_size(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *sd_state,
				  struct v4l2_subdev_frame_size_enum *fse)
{
	if (fse->index >= ARRAY_SIZE(supported_modes))
		return -EINVAL;

	if (fse->code != MEDIA_BUS_FMT_UYVY8_1X16)
		return -EINVAL;

	fse->min_width = supported_modes[fse->index].width;
	fse->max_width = fse->min_width;
	fse->min_height = supported_modes[fse->index].height;
	fse->max_height = fse->min_height;

	return 0;
}

static int ar0830_init_state(struct v4l2_subdev *sd,
			     struct v4l2_subdev_state *sd_state)
{
	struct v4l2_subdev_format fmt = {
		.which = V4L2_SUBDEV_FORMAT_TRY,
		.pad = 0,
		.format = {
			.code = MEDIA_BUS_FMT_UYVY8_1X16,
			.width = AR0830_COMMON_WIDTH,
			.height = AR0830_COMMON_HEIGHT,
		},
	};

	ar0830_set_format(sd, sd_state, &fmt);

	return 0;
}

static const struct v4l2_subdev_video_ops ar0830_video_ops = {
	.s_stream = ar0830_set_stream,
};

static const struct v4l2_subdev_pad_ops ar0830_pad_ops = {
	.set_fmt = ar0830_set_format,
	.get_fmt = v4l2_subdev_get_fmt,
	.enum_mbus_code = ar0830_enum_mbus_code,
	.enum_frame_size = ar0830_enum_frame_size,
};

static const struct v4l2_subdev_core_ops ar0830_core_ops = {
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_ops ar0830_subdev_ops = {
	.core = &ar0830_core_ops,
	.video = &ar0830_video_ops,
	.pad = &ar0830_pad_ops,
};

static const struct media_entity_operations ar0830_subdev_entity_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static const struct v4l2_subdev_internal_ops ar0830_internal_ops = {
	.init_state = ar0830_init_state,
};

static int ar0830_parse_fwnode(struct ar0830 *ar0830, struct device *dev)
{
	struct fwnode_handle *endpoint;
	struct v4l2_fwnode_endpoint bus_cfg = {
		.bus_type = V4L2_MBUS_CSI2_DPHY,
	};
	int ret;

	endpoint =
		fwnode_graph_get_endpoint_by_id(dev_fwnode(dev), 0, 0,
						FWNODE_GRAPH_ENDPOINT_NEXT);
	if (!endpoint) {
		dev_err(dev, "endpoint node not found");
		return -EPROBE_DEFER;
	}

	ret = v4l2_fwnode_endpoint_alloc_parse(endpoint, &bus_cfg);
	if (ret) {
		dev_err(dev, "parsing endpoint node failed");
		goto out_err;
	}

	/* Check the number of MIPI CSI2 data lanes */
	if (bus_cfg.bus.mipi_csi2.num_data_lanes != 2 &&
	    bus_cfg.bus.mipi_csi2.num_data_lanes != 4) {
		dev_err(dev, "only 2 or 4 data lanes are currently supported");
		goto out_err;
	}

	ret = v4l2_link_freq_to_bitmap(dev, bus_cfg.link_frequencies,
				       bus_cfg.nr_of_link_frequencies,
				       link_freq_menu_items,
				       ARRAY_SIZE(link_freq_menu_items),
				       &ar0830->link_freq_bitmap);
	if (ret)
		goto out_err;

out_err:
	v4l2_fwnode_endpoint_free(&bus_cfg);
	fwnode_handle_put(endpoint);
	return ret;
}

static void ar0830_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ar0830 *ar0830 = to_ar0830(sd);

	v4l2_async_unregister_subdev(&ar0830->sd);
	v4l2_subdev_cleanup(sd);
	media_entity_cleanup(&ar0830->sd.entity);
	v4l2_ctrl_handler_free(&ar0830->ctrl_handler);
	pm_runtime_disable(&client->dev);
	pm_runtime_set_suspended(&client->dev);
}
static int ar0830_request_firmware (struct ar0830 *ar0830)
{
	int ret = 0;

	dev_info (ar0830->sd.dev, "requesting firmware : %s\n", BOOTDATA);
	ret = request_firmware (&ar0830->firmware, BOOTDATA, ar0830->sd.dev);
	if (ret) {
		dev_err (ar0830->sd.dev, "error to request ar0830 firmware: %d", ret);
		return ret;
	}

	/* Check firmware is requested */
	if (!ar0830->firmware) {
		dev_err (ar0830->sd.dev, "firmware not requested");
		return -EINVAL;
	}
	
	if (ar0830->firmware->size > MAX_FIRMWARE_SIZE) {
		dev_err (ar0830->sd.dev, "firmware size is too big: %zu", ar0830->firmware->size);
		release_firmware(ar0830->firmware);
		return -EINVAL;
	} else
		dev_info (ar0830->sd.dev, "firmware_size : %zu\n", ar0830->firmware->size);

	dev_info(ar0830->sd.dev, "firmware request success.\n");

	return ret;
}
static int ar0830_write_firmware_window (struct ar0830 *ar0830)
{
	struct i2c_client *client = v4l2_get_subdevdata(&ar0830->sd);
	int ret = 0;
	u8 *buf = (u8 *)ar0830->firmware->data;
	u8 wbuf[4];
	int regAddr = 0x8000;
	int length = 0;
	int i,j;

	for (i = 0; i < ar0830->firmware->size / FIRMWARE_BLOCK_SIZE; i++) {
		buf = buf + FIRMWARE_BLOCK_SIZE;
		for (j = 0; j < MAX_REGISTERS_PER_BLOCK; j++) {
			if (*(buf + OFFSET_BASE + j * OFFSET_STEP) == REG_VALUE_4)
			{
				wbuf[0] = *(buf + OFFSET_BASE  + j * OFFSET_STEP + 1);
				wbuf[1] = *(buf + OFFSET_BASE  + j * OFFSET_STEP + 2);
				wbuf[2] = *(buf + OFFSET_BASE  + j * OFFSET_STEP + 3);
				wbuf[3] = *(buf + OFFSET_BASE  + j * OFFSET_STEP + 4);

				u8 i2c_buf[4];

				i2c_buf[0] = wbuf[0];
				i2c_buf[1] = wbuf[1];
				i2c_buf[2] = wbuf[2];
				i2c_buf[3] = wbuf[3];

				ret = ar0830_write_reg(ar0830, regAddr, i2c_buf, REG_VALUE_4);
				if (ret < 0) {
					dev_err(ar0830->sd.dev, "i2c_transfer failed at regAddr: 0x%x, ret: %d", regAddr, ret);
					return -EIO;
				}

				length += REG_VALUE_4;
				regAddr += REG_VALUE_4;
			}
			else if (*(buf + OFFSET_BASE + j * OFFSET_STEP) == REG_VALUE_2)
			{
				wbuf[0] = *(buf + OFFSET_BASE + j * OFFSET_STEP + 3);
				wbuf[1] = *(buf + OFFSET_BASE + j * OFFSET_STEP + 4);

				u8 i2c_buf[2];

				i2c_buf[0] = wbuf[0];
				i2c_buf[1] = wbuf[1];

				ret = ar0830_write_reg(ar0830, regAddr, i2c_buf, REG_VALUE_2);
				if (ret < 0) {
					dev_err(ar0830->sd.dev, "i2c_transfer failed at regAddr: 0x%x, ret: %d", regAddr, ret);
					return -EIO;
				}
				length += REG_VALUE_2;
				regAddr += REG_VALUE_2;
			}
			else if (*(buf + OFFSET_BASE + j * OFFSET_STEP) == REG_VALUE_1)
			{
				wbuf[0] = *(buf + OFFSET_BASE + j * OFFSET_STEP + 4);

				u8 i2c_buf[1];

				i2c_buf[0] = wbuf[0];

				ret = ar0830_write_reg(ar0830, regAddr, i2c_buf, REG_VALUE_1);
				if (ret < 0) {
					dev_err(ar0830->sd.dev, "i2c_transfer failed at regAddr: 0x%x, ret: %d", regAddr, ret);
					return -EIO;
				}

				length += REG_VALUE_1;
				regAddr += REG_VALUE_1;
			}
			else 
				break;

			if (regAddr > 0x9fff)
				regAddr = 0x8000;
		}
	}

	return 0;
}

static int ar0830_load_firmware (struct ar0830 *ar0830)
{
	struct i2c_client *client = v4l2_get_subdevdata(&ar0830->sd);
	int ret = 0;

	ret = ar0830_request_firmware (ar0830);
	if (ret) {
		dev_err (&client->dev, "error in ar0830_request_firmware: %d", ret);
		return ret;
	}

	/* write firmware data */
	ret = ar0830_write_firmware_window (ar0830);
	if (ret) {
		dev_err (&client->dev, "error in writing bootstage data: %d", ret);
		return ret;
	}

	return ret;
}
static int ar0830_board_setup(struct ar0830 *ar0830)
{
	struct i2c_client *client = v4l2_get_subdevdata(&ar0830->sd);
	int ret;
	u8 data[4];

	/* configuration before load flash */

	/* basic control reg */
	data[0] = 0x00;
	data[1] = 0x2A;
	data[2] = 0x00;
	data[3] = 0x00;
	ret = ar0830_write_reg(ar0830, 0xf038, data, 4);
	if (ret)
	{
		dev_err(&client->dev, "failed to write 0xf038: %d", ret);
		return ret;
	}
	check_val(ar0830, AR0830_REG_VALUE_32BIT, 0xf038, data);

	data[0] = 0x00;
	data[1] = 0x00;
	data[2] = 0x06;
	data[3] = 0x92;
	ret = ar0830_write_reg(ar0830, 0xe004, data, 4);
	if (ret)
	{
		dev_err(&client->dev, "failed to write 0xe004: %d", ret);
		return ret;
	}
	check_val(ar0830, AR0830_REG_VALUE_32BIT, 0xe004, data);

	data[0] = 0x00;
	data[1] = 0x00;
	data[2] = 0x04;
	data[3] = 0x10;
	ret = ar0830_write_reg(ar0830, 0xe000, data, 4);
	if (ret)
	{
		dev_err(&client->dev, "failed to write 0xe000: %d", ret);
		return ret;
	}
	check_val(ar0830, AR0830_REG_VALUE_32BIT, 0xe000, data);

	dev_info(&client->dev, "RESET LOW");
	msleep(50);

	data[0] = 0x00;
	data[1] = 0x00;
	data[2] = 0x04;
	data[3] = 0x12;
	ret = ar0830_write_reg(ar0830, 0xe000, data, 4);
	if (ret)
	{
		dev_err(&client->dev, "failed to write 0xe000: %d", ret);
		return ret;
	}
	check_val(ar0830, AR0830_REG_VALUE_32BIT, 0xe000, data);

	dev_info(&client->dev, "RESET HIGH");
	msleep(50);

	/* SIPS basic control reg */
	data[0] = 0x00;
	data[1] = 0x14;
	ret = ar0830_write_reg(ar0830, 0xf05a, data, 2);
	if (ret) {
		dev_err(&client->dev, "failed to write 0xf05a: %d", ret);
		return ret;
	}
	check_val(ar0830, AR0830_REG_VALUE_16BIT, 0xf05a, data);

	/* basic system reg */

	/* SYSTEM_FREQ_IN */
	data[0] = 0x00;
	data[1] = 0x30;
	data[2] = 0x00;
	data[3] = 0x00;
	ret = ar0830_write_reg(ar0830, 0x6024, data, 4);
	if (ret) {
		dev_err(&client->dev, "failed to write 0x6024: %d", ret);
		return ret;
	}
	check_val(ar0830, AR0830_REG_VALUE_32BIT, 0x6024, data);

	/* INF_MIPI_FREQ_TGT */
	data[0] = 0x03;
	data[1] = 0x80;
	data[2] = 0x00;
	data[3] = 0x00;
	ret = ar0830_write_reg(ar0830, 0x6034, data, 4);
	if (ret) {
		dev_err(&client->dev, "failed to write 0x6034: %d", ret);
		return ret;
	}
	check_val(ar0830, AR0830_REG_VALUE_32BIT, 0x6034, data);

	ret = ar0830_load_firmware (ar0830);
	if (ret) {
		dev_err(&client->dev, "failed to load firmware: %d", ret);
		release_firmware(ar0830->firmware);
		return ret;
	}

	/* ATOMIC */
	data[0] = 0x00;
	data[1] = 0x01;
	ret = ar0830_write_reg(ar0830, 0x1184, data, 2);
	if (ret) {
		dev_err(&client->dev, "failed to write 0x1184: %d", ret);
		return ret;
	}
	check_val(ar0830, AR0830_REG_VALUE_16BIT, 0x1184, data);

	/* PREVIEW_WIDTH */
	data[0] = 0xf;
	data[1] = 0x00;
	ret = ar0830_write_reg(ar0830, 0x2000, data, 2);
	if (ret) {
		dev_err(&client->dev, "failed to write 0x2000: %d", ret);
		return ret;
	}
	check_val(ar0830, AR0830_REG_VALUE_16BIT,  0x2000, data);

	/* PREVIEW_HEIGHT */
	data[0] = 0x8;
	data[1] = 0x70;
	ret = ar0830_write_reg(ar0830, 0x2002, data, 2);
	if (ret) {
		dev_err(&client->dev, "failed to write 0x2002: %d", ret);
		return ret;
	}
	check_val(ar0830, AR0830_REG_VALUE_16BIT,  0x2002, data);

	/* ATOMIC */
	data[0] = 0x00;
	data[1] = 0x13;
	ret = ar0830_write_reg(ar0830, 0x1184, data, 2);
	if (ret) {
		dev_err(&client->dev, "failed to write 0x1184: %d", ret);
		return ret;
	}
	check_val(ar0830, AR0830_REG_VALUE_16BIT,  0x1184, data);
	
	msleep(200);

	/* BOOTDATA_STAGE */
	data[0] = 0xff;
	data[1] = 0xff;
	ret = ar0830_write_reg(ar0830, 0x6002, data, 2);
	if (ret) {
		dev_err(&client->dev, "failed to write 0x6002: %d", ret);
		return ret;
	}
	check_val(ar0830, AR0830_REG_VALUE_16BIT, 0x6002, data);

	msleep(500);

	/* BOOTDATA_CHECKSUM */
	data[0] = 0xff;
	data[1] = 0xff;
	ret = ar0830_write_reg(ar0830, 0x6134, data, 2);
	if (ret) {
		dev_err(&client->dev, "failed to write 0x6134: %d", ret);
		return ret;
	}
	check_val(ar0830, AR0830_REG_VALUE_16BIT,  0x6134, data);

	msleep(500);

	/* PREVIEW_MAX_FPS */
	data[0] = 0x0f;
	data[1] = 0x00;
	ret = ar0830_write_reg(ar0830, 0x2020, data, 2);
	if (ret) {
		dev_err(&client->dev, "failed to write 0x2020: %d", ret);
		return ret;
	}
	check_val(ar0830, AR0830_REG_VALUE_16BIT, 0x2020, data);

	msleep(500);

	return 0;
}
static int ar0830_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct ar0830 *ar0830;

	int ret;

	ar0830 = devm_kzalloc(&client->dev, sizeof(*ar0830), GFP_KERNEL);
	if (!ar0830)
		return -ENOMEM;

	ret = ar0830_parse_fwnode(ar0830, dev);
	if (ret)
		return ret;

	ar0830->regmap = devm_regmap_init_i2c(client, &sensor_regmap_config);
	if (IS_ERR(ar0830->regmap))
		return dev_err_probe(dev, PTR_ERR(ar0830->regmap),
				     "failed to init CCI");

	msleep(100);

	v4l2_i2c_subdev_init(&ar0830->sd, client, &ar0830_subdev_ops);

	ar0830->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_IN);
	if (IS_ERR(ar0830->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ar0830->reset_gpio),
				     "failed to get reset gpio\n");

	ar0830_s_reset(ar0830, 1);

	msleep(100);

	ar0830_s_reset(ar0830, 0);

	msleep(100);

	ret = ar0830_board_setup(ar0830);
	if (ret) {
		dev_err(dev, "failed to setup board: %d", ret);
		return ret;
	}

	ar0830->cur_mode = &supported_modes[0];
	ret = ar0830_init_controls(ar0830);
	if (ret) {
		dev_err(&client->dev, "failed to init controls: %d", ret);
		goto probe_error_v4l2_ctrl_handler_free;
	}

	ar0830->sd.internal_ops = &ar0830_internal_ops;
	ar0830->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
			    V4L2_SUBDEV_FL_HAS_EVENTS;
	ar0830->sd.entity.ops = &ar0830_subdev_entity_ops;
	ar0830->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;

	ar0830->pad.flags = MEDIA_PAD_FL_SOURCE;
	ret = media_entity_pads_init(&ar0830->sd.entity, 1, &ar0830->pad);
	if (ret) {
		dev_err(&client->dev, "failed to init entity pads: %d", ret);
		goto probe_error_v4l2_ctrl_handler_free;
	}

	ar0830->sd.state_lock = ar0830->ctrl_handler.lock;
	ret = v4l2_subdev_init_finalize(&ar0830->sd);
	if (ret < 0) {
		dev_err(dev, "v4l2 subdev init error: %d", ret);
		goto probe_error_media_entity_cleanup;
	}

	/*
	 * Device is already turned on by i2c-core with ACPI domain PM.
	 * Enable runtime PM and turn off the device.
	 */
	pm_runtime_set_active(&client->dev);
	pm_runtime_enable(&client->dev);
	pm_runtime_idle(&client->dev);

	ret = v4l2_async_register_subdev_sensor(&ar0830->sd);
	if (ret < 0) {
		dev_err(&client->dev, "failed to register V4L2 subdev: %d",
			ret);
		goto probe_error_rpm;
	}

#if STALL
	ret = ar0830_stall(ar0830, true);
	if (ret) {
		dev_err (ar0830->sd.dev, "error to stall: %d", ret);
		return ret;
	}
#endif

	return 0;
probe_error_rpm:
	pm_runtime_disable(&client->dev);
	v4l2_subdev_cleanup(&ar0830->sd);

probe_error_media_entity_cleanup:
	media_entity_cleanup(&ar0830->sd.entity);

probe_error_v4l2_ctrl_handler_free:
	v4l2_ctrl_handler_free(ar0830->sd.ctrl_handler);

	return ret;
}

static const struct acpi_device_id ar0830_acpi_ids[] = {
	{ "LIAR0830" },
	{}
};
MODULE_DEVICE_TABLE(acpi, ar0830_acpi_ids);

static struct i2c_driver ar0830_i2c_driver = {
	.driver = {
		.name = "ar0830_intel",
		.acpi_match_table = ACPI_PTR(ar0830_acpi_ids),
	},
	.probe = ar0830_probe,
	.remove = ar0830_remove,
};

module_i2c_driver(ar0830_i2c_driver);

MODULE_DESCRIPTION("ON Semiconductor AR0830 sensor driver");
MODULE_AUTHOR("Ng, Khai Wen <khai.wen.ng@intel.com>");
MODULE_LICENSE("GPL");
