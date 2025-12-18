/* Copyright (C) 2024 Innodisk Corporation */
#include <linux/unaligned.h>
#include <linux/acpi.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/gpio.h>
#include <linux/regmap.h>
#include <linux/firmware.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fwnode.h>
#include "media/i2c/ar0822.h"

// #define CONT_CLK_ENABLE 1

static inline void msleep_range(unsigned int delay_base)
{
	usleep_range(delay_base * 1000, delay_base * 1000 + 500);
}

static int __ar0822_write(struct ar0822 *ar0822, u32 reg, u32 val)
{
	unsigned int size = AR0822_REG_SIZE(reg);
	u16 addr = AR0822_REG_ADDR(reg);
	int ret;

	switch (size) {
	case 2:
		ret = regmap_write(ar0822->regmap, addr, val);
		break;
	case 4:
		ret = regmap_write(ar0822->regmap32, addr, val);
		break;
	default:
		return -EINVAL;
	}

	if (ret) {
		dev_err(ar0822->sd.dev, "%s: register 0x%04x %s failed: %d\n",
			__func__, addr, "write", ret);
		return ret;
	}

	return 0;
}

static int ar0822_write(struct ar0822 *ar0822, u32 reg, u32 val,
			int *err)
{
	u32 page = AR0822_REG_PAGE(reg);
	int ret;

	if (err && *err)
		return *err;

	if (page) {
		if (ar0822->reg_page != page) {
			ret = __ar0822_write(ar0822, AR0822_ADVANCED_BASE,
					     page);
			if (ret < 0)
				goto done;

			ar0822->reg_page = page;
		}

		reg &= ~AR0822_REG_PAGE_MASK;
		reg += AR0822_REG_ADV_START;
	}

	ret = __ar0822_write(ar0822, reg, val);

done:
	if (err && ret)
		*err = ret;

	return ret;
}

static int __ar0822_read(struct ar0822 *ar0822, u32 reg, u32 *val)
{
	unsigned int size = AR0822_REG_SIZE(reg);
	u16 addr = AR0822_REG_ADDR(reg);
	int ret;

	switch (size) {
	case 2:
		ret = regmap_read(ar0822->regmap, addr, val);
		break;
	case 4:
		ret = regmap_read(ar0822->regmap32, addr, val);
		break;
	default:
		return -EINVAL;
	}

	if (ret) {
		dev_err(ar0822->sd.dev, "%s: register 0x%04x %s failed: %d\n",
			__func__, addr, "read", ret);
		return ret;
	}

	dev_info(ar0822->sd.dev, "%s: R0x%04x = 0x%0*x\n", __func__,
		addr, size * 2, *val);

	return 0;
}

static int ar0822_read(struct ar0822 *ar0822, u32 reg, u32 *val)
{
	u32 page = AR0822_REG_PAGE(reg);
	int ret;

	if (page) {
		if (ar0822->reg_page != page) {
			ret = __ar0822_write(ar0822, AR0822_ADVANCED_BASE,
					     page);
			if (ret < 0)
				return ret;

			ar0822->reg_page = page;
		}

		reg &= ~AR0822_REG_PAGE_MASK;
		reg += AR0822_REG_ADV_START;
	}

	return __ar0822_read(ar0822, reg, val);
}

/* Setup for regmap poll */
static int __ar0822_poll_param(struct ar0822 *ar0822, u32 reg,
		struct regmap **regmap,u16 *addr)
{
	u32 page = AR0822_REG_PAGE(reg);
	int ret;

	if (page) {
		if (ar0822->reg_page != page) {
			ret = __ar0822_write(ar0822, AR0822_ADVANCED_BASE,
					     page);
			if (ret < 0)
				return ret;

			ar0822->reg_page = page;
		}

		reg &= ~AR0822_REG_PAGE_MASK;
		reg += AR0822_REG_ADV_START;
	}

	*addr = AR0822_REG_ADDR(reg);

	switch (AR0822_REG_SIZE(reg)) {
	case 2:
		*regmap=ar0822->regmap;
		break;
	case 4:
		*regmap=ar0822->regmap32;
		break;
	default:
		return -EINVAL;
	}

	dev_dbg(ar0822->sd.dev, "%s: R0x%08x -> 0x%04x\n", __func__,
			reg,*addr);

	return 0;
}

#define ar0822_poll_timeout(ar0822,reg,val,cond,sleep_us,timeout_us) \
({ \
	struct regmap *__regmap; \
	u16 addr; \
	int __retpoll; \
	__retpoll = __ar0822_poll_param(ar0822,reg,&__regmap,&addr); \
	if (!__retpoll) \
		__retpoll = regmap_read_poll_timeout(__regmap, addr, val, cond, sleep_us, timeout_us); \
	__retpoll; \
})

static int ar0822_stall(struct ar0822 *ar0822, bool stall_en)
{
	int ret = 0;

	dev_info(ar0822->sd.dev, "stall: %s\n", (stall_en) ? "Enable" : "Disable");

	if (stall_en)
	{
		ar0822_write(ar0822, AR0822_SYS_START,
					AR0822_SYS_START_STALL_EN |
					AR0822_SYS_START_STALL_MODE_STANDBY_SENSOR_OFF, &ret);
		if (ret < 0) {
		dev_err(ar0822->sd.dev, "regmap_write error (address 0x%x)\n", AR0822_SYS_START);
		return ret;
		}
	}
	else
	{
		ar0822_write(ar0822, AR0822_SYS_START,
					AR0822_SYS_START_STALL_EN,
				    &ret);
		if (ret < 0) {
			dev_err(ar0822->sd.dev, "regmap_write error (address 0x%x)\n", AR0822_SYS_START);
			return ret;
		}
	}

	return ret;
}

#ifdef CONT_CLK_ENABLE
static int ar0822_setup_cont_clk(struct ar0822 *ar0822, bool en)
{
	u32 hinf_ctrl = 0U;
	int ret = 0;

	dev_info(ar0822->sd.dev, "%s\n", __func__);

	ret = ar0822_read(ar0822, AR0822_PREVIEW_HINF_CTRL, &hinf_ctrl);
	if (ret < 0) {
		dev_err(ar0822->sd.dev, "error reading (address 0x%x)\n", AR0822_REG_ADDR(AR0822_PREVIEW_HINF_CTRL));
		return ret;
	}

	hinf_ctrl &= ~(HINF_CTRL_CONT_CLK_MASK);
	if (en)
	{
		hinf_ctrl |= HINF_CTRL_EN_CONT_CLK;
	}
	else
	{
		hinf_ctrl |= HINF_CTRL_DIS_CONT_CLK;
	}

	ar0822_write(ar0822, AR0822_PREVIEW_HINF_CTRL, hinf_ctrl, &ret);
	if (ret < 0) {
		dev_err(ar0822->sd.dev, "error writing (address 0x%x)\n", AR0822_REG_ADDR(AR0822_PREVIEW_HINF_CTRL));
		return ret;
	}

	msleep_range(100);

	return ret;
}
#endif

static int ar0822_setup_mipi_dat_lane(struct ar0822 *ar0822, u32 num_lanes)
{
	u32 hinf_ctrl = 0U;
	int ret = 0;

	dev_info(ar0822->sd.dev, "MIPI Data %dLane\n", num_lanes);

	ret = ar0822_read(ar0822, AR0822_PREVIEW_HINF_CTRL, &hinf_ctrl);
	if (ret < 0) {
		dev_err(ar0822->sd.dev, "error reading (address 0x%x)\n", AR0822_REG_ADDR(AR0822_PREVIEW_HINF_CTRL));
		return ret;
	}

	hinf_ctrl &= 0xFFFF;
	hinf_ctrl &= ~(PREVIEW_HINF_CTRL_MIPI_MASK);
	hinf_ctrl |= num_lanes;

	ar0822_write(ar0822, AR0822_PREVIEW_HINF_CTRL, hinf_ctrl, &ret);
	if (ret < 0) {
		dev_err(ar0822->sd.dev, "error writing (address 0x%x)\n", AR0822_REG_ADDR(AR0822_PREVIEW_HINF_CTRL));
		return ret;
	}

	msleep_range(100);

	return ret;
}

static u64 get_pixel_rate(struct ar0822 *ar0822)
{
	return ar0822->cur_mode->width * ar0822->cur_mode->height * ar0822->cur_mode->fps * 16 / ar0822->cur_mode->lanes;
}

static int ar0822_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct ar0822 *ar0822 = container_of(ctrl->handler,
					     struct ar0822, ctrl_handler);
	struct i2c_client *client = v4l2_get_subdevdata(&ar0822->sd);
	struct v4l2_subdev_state *state;
	const struct v4l2_mbus_framefmt *format;
	int ret = 0;

	/* V4L2 controls values will be applied only when power is already up */
	if (!pm_runtime_get_if_in_use(&client->dev))
		return 0;

	state = v4l2_subdev_get_locked_active_state(&ar0822->sd);
	format = v4l2_subdev_state_get_format(state, 0);

	switch (ctrl->id) {
	case V4L2_CID_ANALOGUE_GAIN:
		dev_dbg(&client->dev, "set analogue gain.\n");
		break;

	case V4L2_CID_DIGITAL_GAIN:
		dev_dbg(&client->dev, "set digital gain.\n");
		break;

	case V4L2_CID_EXPOSURE:
		dev_dbg(&client->dev, "set exposure time.\n");
		break;

	case V4L2_CID_VBLANK:
		dev_dbg(&client->dev, "set vblank.\n");
		break;

	default:
		dev_err(&client->dev, "unexpected ctrl id 0x%08x.\n", ctrl->id);
		ret = -EINVAL;
		break;
	}

	pm_runtime_put(&client->dev);

	return ret;
}

static void check_val(struct ar0822 *ar0822, u32 addr, u32 write_val)
{
	struct i2c_client *client = v4l2_get_subdevdata(&ar0822->sd);
	u32 reg_value_check;
	int ret;

	ret = ar0822_read(ar0822, addr, &reg_value_check);
	dev_dbg(&client->dev, "0x%04x read value: 0x%x, write value: 0x%x, %s\n", AR0822_REG_ADDR(addr),
			reg_value_check, write_val, reg_value_check == write_val ? "OK" : "FAIL");
	if (ret) {
		dev_err(ar0822->sd.dev, "failed to read 0x%04x: %d", addr, ret);
	}
	return;
}

static int ar0822_init_controls(struct ar0822 *ar0822)
{
	struct i2c_client *client = v4l2_get_subdevdata(&ar0822->sd);
	struct v4l2_ctrl_handler *ctrl_hdlr;
	int ret;

	ctrl_hdlr = &ar0822->ctrl_handler;
	ret = v4l2_ctrl_handler_init(ctrl_hdlr, 8);
	if (ret)
		return ret;

	ar0822->link_freq = v4l2_ctrl_new_int_menu(ctrl_hdlr, &ar0822_ctrl_ops, V4L2_CID_LINK_FREQ, ARRAY_SIZE(link_freq_menu_items) - 1, 0, link_freq_menu_items);
	if (ar0822->link_freq)
		ar0822->link_freq->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	ar0822->vblank = v4l2_ctrl_new_std(ctrl_hdlr, &ar0822_ctrl_ops, V4L2_CID_VBLANK, 0, 1, 1, 1);
	v4l2_ctrl_new_std(ctrl_hdlr, &ar0822_ctrl_ops, V4L2_CID_ANALOGUE_GAIN, 0, 1, 1, 1);
	v4l2_ctrl_new_std(ctrl_hdlr, &ar0822_ctrl_ops, V4L2_CID_DIGITAL_GAIN, 0, 1, 1, 1);
	ar0822->exposure = v4l2_ctrl_new_std(ctrl_hdlr, &ar0822_ctrl_ops, V4L2_CID_EXPOSURE, 0, 1, 1, 1);
	ar0822_i2c_bus.def = i2c_adapter_id(client->adapter);
	ar0822->i2c_bus = v4l2_ctrl_new_custom(ctrl_hdlr, &ar0822_i2c_bus, NULL);
	ar0822_i2c_id.def = client->addr;
	ar0822->i2c_id = v4l2_ctrl_new_custom(ctrl_hdlr, &ar0822_i2c_id, NULL);
	ar0822_fps.def = ar0822->cur_mode->fps;
	ar0822->fps = v4l2_ctrl_new_custom(ctrl_hdlr, &ar0822_fps, NULL);
	ar0822_frame_interval.def = 1000 / ar0822->cur_mode->fps;
	ar0822->frame_interval = v4l2_ctrl_new_custom(ctrl_hdlr, &ar0822_frame_interval, NULL);

	ar0822->pixel_rate = v4l2_ctrl_new_std(ctrl_hdlr, &ar0822_ctrl_ops, V4L2_CID_PIXEL_RATE, get_pixel_rate(ar0822), get_pixel_rate(ar0822), 1, get_pixel_rate(ar0822));
	if (ar0822->pixel_rate)
		ar0822->pixel_rate->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	ar0822->hblank = v4l2_ctrl_new_std(ctrl_hdlr, &ar0822_ctrl_ops, V4L2_CID_HBLANK, 0, 1, 1, 1);
	if (ar0822->hblank)
		ar0822->hblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	if (ctrl_hdlr->error)
		return ctrl_hdlr->error;

	ar0822->sd.ctrl_handler = ctrl_hdlr;

	return ret;
}

static void ar0822_update_pad_format(const struct ar0822_mode *mode,
				     struct v4l2_mbus_framefmt *fmt)
{
	fmt->width = mode->width;
	fmt->height = mode->height;
	fmt->code = mode->code;
	fmt->field = V4L2_FIELD_NONE;
}

#ifdef DEBUG
static ssize_t ar0822_show_sensor_lane_status(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(to_i2c_client(dev));
	struct ar0822 *ar0822 = to_ar0822(sd);
	static const char * const lp_states[] = {
		"00", "10", "01", "11",
	};
	// unsigned int index;
	unsigned int counts[4][ARRAY_SIZE(ar0822_lane_states)];
	unsigned int samples = 0;
	unsigned int lane;
	unsigned int i;
	u32 first[4] = { 0, };
	u32 last[4] = { 0, };
	int ret;
	u32 buf_len=0;

	samples = 0;
	memset(first, 0, sizeof(first));
	memset(last, 0, sizeof(last));
	memset(counts, 0, sizeof(counts));

	for (i = 0; i < 1000; ++i) {
		u32 values[4];
		/*
			* Read the state of all lanes and skip read errors and invalid
			* values.
			*/
		for (lane = 0; lane < 4; ++lane) {
			ret = ar0822_read(ar0822,
						AR0822_ADV_SINF_MIPI_INTERNAL_p_LANE_n_STAT(0, lane),
						&values[lane]);
			if (ret < 0)
				break;

			if (AR0822_LANE_STATE(values[lane]) >=
				ARRAY_SIZE(ar0822_lane_states)) {
				ret = -EINVAL;
				break;
			}
		}

		if (ret < 0)
			continue;

		/* Accumulate the samples and save the first and last states. */
		for (lane = 0; lane < 4; ++lane)
			counts[lane][AR0822_LANE_STATE(values[lane])]++;

		if (!samples)
			memcpy(first, values, sizeof(first));
		memcpy(last, values, sizeof(last));

		samples++;
	}

	if (!samples)
		return buf_len;

	/*
		* Print the LP state from the first sample, the error state from the
		* last sample, and the states accumulators for each lane.
		*/
	for (lane = 0; lane < 4; ++lane) {
		u32 state = last[lane];
		char error_msg[25] = "";

		if (state & (AR0822_LANE_ERR | AR0822_LANE_ABORT)) {
			unsigned int err = AR0822_LANE_ERR_STATE(state);
			const char *err_state = NULL;

			err_state = err < ARRAY_SIZE(ar0822_lane_states)
					? ar0822_lane_states[err] : "INVALID";

			scnprintf(error_msg, sizeof(error_msg), "ERR (%s%s) %s LP%s",
					state & AR0822_LANE_ERR ? "E" : "",
					state & AR0822_LANE_ABORT ? "A" : "",
					err_state,
					lp_states[AR0822_LANE_ERR_LP_VAL(state)]);
		}
		buf_len += scnprintf(buf+buf_len,PAGE_SIZE-buf_len,
				"SINF%u L%u state: LP%s %s\n",
					0, lane,
					lp_states[AR0822_LANE_LP_VAL(first[lane])],
					error_msg);

		for (i = 0; i < ARRAY_SIZE(ar0822_lane_states); ++i) {
			if (counts[lane][i])
				buf_len += scnprintf(buf+buf_len,
					PAGE_SIZE-buf_len," %s:%u",
					ar0822_lane_states[i],
					counts[lane][i]);
		}
		buf_len += scnprintf(buf+buf_len,PAGE_SIZE-buf_len,"\n");
	}


	return buf_len;
}

static ssize_t ar0822_show_host_lane_status(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(to_i2c_client(dev));
	struct ar0822 *ar0822 = to_ar0822(sd);
	static const char * const lp_states[] = {
		"00", "10", "01", "11",
	};
	// unsigned int index;
	unsigned int counts[4][ARRAY_SIZE(ar0822_lane_states)];
	unsigned int samples = 0;
	unsigned int lane;
	unsigned int i;
	u32 first[4] = { 0, };
	u32 last[4] = { 0, };
	int ret;
	u32 buf_len=0;

	/* Sample the lane state. */
	samples = 0;
	memset(first, 0, sizeof(first));
	memset(last, 0, sizeof(last));
	memset(counts, 0, sizeof(counts));

	for (i = 0; i < 1000; ++i) {
		u32 values[4];

		/*
			* Read the state of all lanes and skip read errors and invalid
			* values.
			*/
		for (lane = 0; lane < 4; ++lane) {
			ret = ar0822_read(ar0822,
						AR0822_HINF_MIPI_INTERNAL_LANE_n_CTRL(lane),
						&values[lane]);
			if (ret < 0)
				break;

			if (AR0822_LANE_STATE(values[lane]) >=
				ARRAY_SIZE(ar0822_lane_states)) {
				ret = -EINVAL;
				break;
			}
		}

		if (ret < 0)
			continue;

		/* Accumulate the samples and save the first and last states. */
		for (lane = 0; lane < 4; ++lane)
			counts[lane][AR0822_LANE_STATE(values[lane])]++;

		if (!samples)
			memcpy(first, values, sizeof(first));
		memcpy(last, values, sizeof(last));

		samples++;
	}

	if (!samples)
		return buf_len;

	/*
		* Print the LP state from the first sample, the error state from the
		* last sample, and the states accumulators for each lane.
		*/
	for (lane = 0; lane < 4; ++lane) {
		u32 state = last[lane];
		char error_msg[25] = "";

		if (state & (AR0822_LANE_ERR | AR0822_LANE_ABORT)) {
			unsigned int err = AR0822_LANE_ERR_STATE(state);
			const char *err_state = NULL;

			err_state = err < ARRAY_SIZE(ar0822_lane_states)
					? ar0822_lane_states[err] : "INVALID";

			scnprintf(error_msg, sizeof(error_msg), "ERR (%s%s) %s LP%s",
					state & AR0822_LANE_ERR ? "E" : "",
					state & AR0822_LANE_ABORT ? "A" : "",
					err_state,
					lp_states[AR0822_LANE_ERR_LP_VAL(state)]);
		}
		buf_len += scnprintf(buf+buf_len,PAGE_SIZE-buf_len,
				"HINF%u L%u state: LP%s %s\n",
					0, lane,
					lp_states[AR0822_LANE_LP_VAL(first[lane])],
					error_msg);

		for (i = 0; i < ARRAY_SIZE(ar0822_lane_states); ++i) {
			if (counts[lane][i])
				buf_len += scnprintf(buf+buf_len,
					PAGE_SIZE-buf_len," %s:%u",
					ar0822_lane_states[i],
					counts[lane][i]);
		}
		buf_len += scnprintf(buf+buf_len,PAGE_SIZE-buf_len,"\n");
	}

	return buf_len;
}

static int ar0822_dump_console(struct ar0822  *ar0822)
{
	u8* buffer;
	u8* endp;
	u8* p;
	int ret;
	u32 warning[4];
	int i;
	buffer = kmalloc(AR0822_CON_BUF_SIZE + 1, GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	ret = regmap_raw_read(ar0822->regmap, AR0822_CON_BUF(0), buffer,
			      AR0822_CON_BUF_SIZE);
	if (ret < 0) {
		dev_err(ar0822->sd.dev, "Failed to read console buffer: %d\n",
			ret);
		goto done;
	}

	/* Print warnings. */
	for (i = 0; i < ARRAY_SIZE(warning); ++i) {
		ret = ar0822_read(ar0822, AR0822_WARNING(i), &warning[i]);
		if (ret < 0)
			return ret;
	}

	dev_err(ar0822->sd.dev,
		 "WARNING [0] 0x%04x [1] 0x%04x [2] 0x%04x [3] 0x%04x\n",
		 warning[0], warning[1], warning[2], warning[3]);

	for (i = 0; i < ARRAY_SIZE(ar0822_warnings); ++i) {
		if ((warning[i / 16] & BIT(i % 16)) &&
		    ar0822_warnings[i])
			dev_err(ar0822->sd.dev, "- WARN_%s\n",
				 ar0822_warnings[i]);
	}

	u32 value = 0;
	u16 frame_count_icp;
	u16 frame_count_brac;
	u16 frame_count_hinf;
	ret = ar0822_read(ar0822, AR0822_FRAME_CNT, &value);
	if (ret < 0)
		return ret;

	frame_count_hinf = value >> 8;
	frame_count_brac = value & 0xff;

	ret = ar0822_read(ar0822, AR0822_ADV_CAPTURE_A_FV_CNT, &value);
	if (ret < 0)
		return ret;

	frame_count_icp = value & 0xffff;

	dev_err(ar0822->sd.dev, "Frame counters: ICP %u, HINF %u, BRAC %u\n",
		 frame_count_icp, frame_count_hinf, frame_count_brac);

	print_hex_dump(KERN_ERR, "ev8 ar0822 Raw: ", DUMP_PREFIX_OFFSET, 16, 1,
		       buffer, AR0822_CON_BUF_SIZE, true);

	// Optional debug print
	// buffer[AR0822_CON_BUF_SIZE] = '\0';
	// for (p = buffer; p < buffer + AR0822_CON_BUF_SIZE; p = endp + 1) {
	// 	endp = memchr(p, '\n', buffer + AR0822_CON_BUF_SIZE - p);
	// 	if (!endp) {
	// 		if (*p) {
	// 			dev_info(ar0822->sd.dev, "log > %s\n", p);
	// 		}
	// 		break;
	// 	}
	// 	*endp = '\0';
	// 	dev_info(ar0822->sd.dev, "log > %s\n", p);
	// }
	ret = 0;

	done:
	kfree(buffer);
	return ret;
}
#endif // DEBUG

static int ar0822_identify_module(struct ar0822 *ar0822)
{
	struct i2c_client *client = v4l2_get_subdevdata(&ar0822->sd);
	int ret;
	u32 val;

	ret = ar0822_read(ar0822, AR0822_REG_CHIP_ID, &val);

	if (ret)
	{
		dev_err(&client->dev, "failed to read chip id: %d", ret);
		return ret;
	}

	if (val != AR0822_CHIP_ID) {
		dev_err(&client->dev, "chip id mismatch: %x!=%x",
			AR0822_CHIP_ID, val);
		return -ENXIO;
	}
	else
		dev_info(&client->dev, "chip id: %x\n", val);

	return 0;
}

static int ar0822_start_streaming(struct ar0822 *ar0822)
{
	struct i2c_client *client = v4l2_get_subdevdata(&ar0822->sd);
	int ret = 0;

	ret = pm_runtime_resume_and_get(&client->dev);
	if (ret < 0)
		return ret;

	dev_info (ar0822->sd.dev, "ar0822_start_streaming\n");

	ret = __v4l2_ctrl_handler_setup(ar0822->sd.ctrl_handler);
	if (ret)
		{goto err_rpm_put;}

	ret = ar0822_stall(ar0822, false);
	if (ret) {
		dev_err (ar0822->sd.dev, "error to disable stall: %d", ret);
		return ret;
	}

	return 0;

err_rpm_put:
	pm_runtime_put(&client->dev);
	return ret;
}

static int ar0822_stop_streaming(struct ar0822 *ar0822)
{
	struct i2c_client *client = v4l2_get_subdevdata(&ar0822->sd);

	int ret = ar0822_stall(ar0822, true);
		if (ret) {
		dev_err (ar0822->sd.dev, "error to enable stall: %d", ret);
		return ret;
	}

	dev_info (ar0822->sd.dev, "ar0822_stop_streaming\n");

	pm_runtime_put(&client->dev);

	return 0;
}

static int ar0822_set_stream(struct v4l2_subdev *sd, int enable)
{
	struct ar0822 *ar0822 = to_ar0822(sd);
	struct v4l2_subdev_state *state;
	int ret = 0;

	state = v4l2_subdev_lock_and_get_active_state(sd);
	if (enable)
		ret = ar0822_start_streaming(ar0822);
	else
		ret = ar0822_stop_streaming(ar0822);

	v4l2_subdev_unlock_state(state);

#ifdef CONT_CLK_ENABLE
	ar0822_setup_cont_clk(ar0822, false);
	msleep(20);
	ar0822_setup_cont_clk(ar0822, true);
#endif
	return ret;
}

static int ar0822_set_format(struct v4l2_subdev *sd,
			     struct v4l2_subdev_state *sd_state,
			     struct v4l2_subdev_format *fmt)
{
	struct ar0822 *ar0822 = to_ar0822(sd);
	struct i2c_client *client = v4l2_get_subdevdata(&ar0822->sd);
	const struct ar0822_mode *mode;
	struct v4l2_rect *crop;
	int ret;

	mode = v4l2_find_nearest_size(supported_modes,
					ARRAY_SIZE(supported_modes),
					width, height,
					fmt->format.width,
					fmt->format.height);

	crop = v4l2_subdev_state_get_crop(sd_state, fmt->pad);
	crop->width = mode->width;
	crop->height = mode->height;

	ar0822_update_pad_format(mode, &fmt->format);
	*v4l2_subdev_state_get_format(sd_state, fmt->pad) = fmt->format;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		return 0;

	ar0822->cur_mode = mode;
	ret = __v4l2_ctrl_modify_range(ar0822->hblank, 0, 1, 1, 1);
	if (ret) {
		dev_err(&client->dev, "HB ctrl range update failed");
		return ret;
	}

	/* Update limits and set FPS to default */
	ret = __v4l2_ctrl_modify_range(ar0822->vblank, 0, 1, 1, 1);
	if (ret) {
		dev_err(&client->dev, "VB ctrl range update failed");
		return ret;
	}

	ret = __v4l2_ctrl_s_ctrl(ar0822->vblank, 1);
	if (ret) {
		dev_err(&client->dev, "VB ctrl set failed");
		return ret;
	}

	return 0;
}

static int ar0822_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *sd_state,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index >= ARRAY_SIZE(supported_formats))
		return -EINVAL;

	code->code = supported_formats[code->index];

	return 0;
}

static int ar0822_enum_frame_size(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *sd_state,
				  struct v4l2_subdev_frame_size_enum *fse)
{
	if (fse->index >= ARRAY_SIZE(supported_modes))
		return -EINVAL;

	fse->min_width = supported_modes[fse->index].width;
	fse->max_width = fse->min_width;
	fse->min_height = supported_modes[fse->index].height;
	fse->max_height = fse->min_height;

	return 0;
}

static int ar0822_get_selection(struct v4l2_subdev *sd,
	struct v4l2_subdev_state *state,
	struct v4l2_subdev_selection *sel)
{
	switch (sel->target) {
		case V4L2_SEL_TGT_CROP_DEFAULT:
		case V4L2_SEL_TGT_CROP_BOUNDS:
		case V4L2_SEL_TGT_NATIVE_SIZE:
			sel->r.top = AR0822_PIXEL_ARRAY_TOP;
			sel->r.left = AR0822_PIXEL_ARRAY_LEFT;
			sel->r.width = AR0822_COMMON_WIDTH;
			sel->r.height = AR0822_COMMON_HEIGHT;
			break;

		case V4L2_SEL_TGT_CROP:
			sel->r = *v4l2_subdev_state_get_crop(state, 0);
			break;

		default:
			return -EINVAL;
	}

	return 0;
}

static int ar0822_init_state(struct v4l2_subdev *sd,
	struct v4l2_subdev_state *sd_state)
{
	struct v4l2_subdev_format fmt = {
		.which = V4L2_SUBDEV_FORMAT_TRY,
		.pad = 0,
		.format = {
			.code = MEDIA_BUS_FMT_UYVY8_1X16,
			.width = AR0822_COMMON_WIDTH,
			.height = AR0822_COMMON_HEIGHT,
		},
	};

	ar0822_set_format(sd, sd_state, &fmt);

	return 0;
}

static const struct v4l2_subdev_video_ops ar0822_video_ops = {
	.s_stream = ar0822_set_stream,
};

static const struct v4l2_subdev_pad_ops ar0822_pad_ops = {
	.set_fmt = ar0822_set_format,
	.get_fmt = v4l2_subdev_get_fmt,
	.enum_mbus_code = ar0822_enum_mbus_code,
	.enum_frame_size = ar0822_enum_frame_size,
	.get_selection = ar0822_get_selection,
};

static const struct v4l2_subdev_core_ops ar0822_core_ops = {
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_ops ar0822_subdev_ops = {
	.core = &ar0822_core_ops,
	.video = &ar0822_video_ops,
	.pad = &ar0822_pad_ops,
};

static const struct media_entity_operations ar0822_subdev_entity_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static const struct v4l2_subdev_internal_ops ar0822_internal_ops = {
	.init_state = ar0822_init_state,
};

static int ar0822_parse_fwnode(struct ar0822 *ar0822, struct device *dev)
{
	struct fwnode_handle *endpoint;
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
		ret = -EINVAL;
		goto out_err;
	}

	ret = v4l2_link_freq_to_bitmap(dev, bus_cfg.link_frequencies,
				       bus_cfg.nr_of_link_frequencies,
				       link_freq_menu_items,
				       ARRAY_SIZE(link_freq_menu_items),
				       &ar0822->link_freq_bitmap);

out_err:
	v4l2_fwnode_endpoint_free(&bus_cfg);
	fwnode_handle_put(endpoint);
	return ret;
}


static int ar0822_parse_gpio(struct ar0822 *ar0822, struct device *dev)
{
	ar0822->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ar0822->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ar0822->reset_gpio),
				     "failed to get reset gpio\n");

	return 0;
}

static void ar0822_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ar0822 *ar0822 = to_ar0822(sd);

	v4l2_async_unregister_subdev(&ar0822->sd);
	v4l2_subdev_cleanup(sd);
	media_entity_cleanup(&ar0822->sd.entity);
	v4l2_ctrl_handler_free(&ar0822->ctrl_handler);
	pm_runtime_disable(&client->dev);
	pm_runtime_set_suspended(&client->dev);

	gpiod_set_value(ar0822->reset_gpio, 1);
	return;
}

static int ar0822_request_firmware (struct ar0822 *ar0822)
{
	const struct ar0822_firmware *firmware_header;
	unsigned int firmware_size = 0;
	int ret = 0;

	dev_info (ar0822->sd.dev, "requesting firmware : %s\n", EV8M_OOM1_FW_FILENAME);
	ret = request_firmware (&ar0822->firmware, EV8M_OOM1_FW_FILENAME, ar0822->sd.dev);
	if (ret) {
		dev_err (ar0822->sd.dev, "error to request ar0822 firmware: %d", ret);
		return ret;
	}

	// Check firmware is requested
	if (!ar0822->firmware) {
		dev_err (ar0822->sd.dev, "firmware not requested");
		return -EINVAL;
	}

	// Check firmware size
	firmware_header = (const struct ar0822_firmware *) ar0822->firmware->data;
	firmware_size = ar0822->firmware->size - sizeof(*firmware_header);

	if (firmware_header->pll_init_size > firmware_size) {
		dev_err (ar0822->sd.dev, "invalid firmware, size too large: %d", ret);
		return -EINVAL;
	}

	dev_info (ar0822->sd.dev, "firmware_size : %d, firmware_header->pll_init_size : %hd\n", firmware_size, firmware_header->pll_init_size);
	return ret;
}

static int ar0822_write_firmware_window(struct ar0822 *ar0822, u16 *win_pos, const u8 *buf, u32 len)
{
	int ret = 0;
	u32 pos;
	u32 sub_len;

	for (pos = 0; pos < len; pos += sub_len) {
		/* Checking remaining addresses in window */
		if (len - pos < (AR0822_FIRMWARE_WINDOW_SIZE - *win_pos))
			sub_len = len - pos;
		else
			sub_len = AR0822_FIRMWARE_WINDOW_SIZE - *win_pos;

		/* Limiting max addresses to use per write to 0x0FF0,
		 * more addresses in one write will fail */
		if (sub_len > AR0822_FIRMWARE_BUFFER)
			sub_len = AR0822_FIRMWARE_BUFFER;

		ret = regmap_raw_write(ar0822->regmap, *win_pos + AR0822_FIRMWARE_WINDOW_OFFSET, buf + pos, sub_len);
		if (ret < 0) {
			dev_err(ar0822->sd.dev, "error to write %d bytes to address 0x%x\n", sub_len, pos);
			return ret;
		}

		*win_pos += sub_len;
		if (*win_pos >= AR0822_FIRMWARE_WINDOW_SIZE)
			*win_pos = 0;
		usleep_range(2000, 3000);
	}
	return 0;
}

static int ar0822_load_firmware(struct ar0822 *ar0822)
{
	struct i2c_client *client = v4l2_get_subdevdata(&ar0822->sd);
	const struct ar0822_firmware *firmware_header;
	const u8 *firmware_data;
	u16 win_pos = 0;
	int ret = 0;
	u32 val = 0;

	ret = ar0822_request_firmware (ar0822);
	if (ret) {
		dev_err (&client->dev, "error in ar0822_request_firmware: %d", ret);
		return ret;
	}

	/* clear CRC register */
	ar0822_write (ar0822, AR0822_SIP_CRC, 0xFFFF, &ret);
	if (ret) {
		dev_err (&client->dev, "error in clear CRC register: %d", ret);
		return ret;
	}

	/* load firmware data for pll init stage */
	firmware_header = (const struct ar0822_firmware *)ar0822->firmware->data;
	firmware_data = (u8 *)&firmware_header[1];

	ret = ar0822_write_firmware_window(ar0822, &win_pos, firmware_data, firmware_header->pll_init_size);
	if (ret) {
		dev_err (&client->dev, "error in firmware PLL init: %d", ret);
		return ret;
	}

	/* to enable PLL */
	ar0822_write (ar0822, AR0822_BOOTDATA_STAGE, 0x0002, &ret);
	if (ret) {
		dev_err (&client->dev, "error to write 0x0002 to bootdata stage: %d", ret);
		return ret;
	}

	/* wait 1ms for PLL to lock */
	usleep_range(1000, 2000);

	/* load rest of bootdata */
	ret = ar0822_write_firmware_window (ar0822, &win_pos, firmware_data + firmware_header->pll_init_size, ar0822->firmware->size - firmware_header->pll_init_size - sizeof (*firmware_header));

	if (ret) {
		dev_err (&client->dev, "error to load remain bootdata: %d", ret);
		return ret;
	}

	msleep(400);

	/* to indicate sensor the whole bootdata content has been loaded */
	ar0822_write (ar0822, AR0822_BOOTDATA_STAGE, 0xFFFF, &ret);
	if (ret) {
		dev_err (&client->dev, "error to write 0xFFFF to bootdata stage: %d", ret);
		return ret;
	}

	msleep(400);
	/*
	* Wait for Bootdata Stage Complete
	*/
	ret = ar0822_poll_timeout(ar0822, AR0822_BOOTDATA_STAGE, val,
			(val == BOOTSTAGE_COMPLETE),
			10000, 5000000);

	return ret;
}

static void ar0822_gpio_reset(struct ar0822 *ar0822)
{
	if (ar0822->reset_gpio == NULL)
		return;

	gpiod_set_value_cansleep(ar0822->reset_gpio, 1);
	msleep (100);
	gpiod_set_value_cansleep(ar0822->reset_gpio, 0);
	msleep (100);
	int ret = gpiod_get_value_cansleep(ar0822->reset_gpio);
	return;
}

/* Call ar0822 to wake up by gpio */
static int ar0822_board_setup (struct ar0822 *ar0822)
{
	struct device *dev = ar0822->sd.dev;
	struct v4l2_subdev_state *state;
	int ret = 0;

	ret = ar0822_load_firmware(ar0822);
	if (ret) {
		dev_err (dev, "error to load firmware: %d", ret);
		return ret;
	}

	/* SYSTEM_FREQ_IN in MHz */
	ar0822_write(ar0822, AR0822_SYSTEM_FREQ_IN, TO_S15_16(48), &ret);

	state = v4l2_subdev_lock_and_get_active_state(&ar0822->sd);
	ret = ar0822_setup_mipi_dat_lane(ar0822, bus_cfg.bus.mipi_csi2.num_data_lanes);
	if (ret) {
		dev_err (dev, "error to setup mipi data lane: %d", ret);
		return ret;
	}
	dev_info (dev, "ar0822 setup %d mipi data lane success.\n", bus_cfg.bus.mipi_csi2.num_data_lanes);
	v4l2_subdev_unlock_state(state);

	/* ATOMIC START RECORDING */
	ar0822_write(ar0822, AR0822_ATOMIC, ATOMIC_RECORD, &ret);
	if (ret) {
		dev_err(dev, "failed to write to 0x%04x to start atomic record: %d",
				AR0822_REG_ADDR(AR0822_ATOMIC), ret);
		return ret;
	}
	msleep(5);

	/* PREVIEW_WIDTH */
	ar0822_write(ar0822, AR0822_PREVIEW_WIDTH, AR0822_COMMON_WIDTH, &ret);
	if (ret) {
		dev_err(dev, "failed to write to 0x%04x: %d",
				AR0822_REG_ADDR(AR0822_PREVIEW_WIDTH), ret);
		return ret;
	}

	/* PREVIEW_HEIGHT */
	ar0822_write(ar0822, AR0822_PREVIEW_HEIGHT, AR0822_COMMON_HEIGHT, &ret);
	if (ret) {
		dev_err(dev, "failed to write to 0x%04x: %d",
				AR0822_REG_ADDR(AR0822_PREVIEW_HEIGHT), ret);
		return ret;
	}

	/* PREVIEW_MAX_FPS */
	ar0822_write(ar0822, AR0822_PREVIEW_MAX_FPS, PREVIEW_MAX_FPS(30), &ret);
	if (ret) {
		dev_err(dev, "failed to write to 0x%04x: %d",
				AR0822_REG_ADDR(AR0822_PREVIEW_MAX_FPS), ret);
		return ret;
	}

	msleep(5);

	/* ATOMIC FINISH */
	ar0822_write(ar0822, AR0822_ATOMIC, ATOMIC_UPDATE_ALL, &ret);
	if (ret) {
		dev_err(dev, "failed to write to 0x%04x to finish atomic record: %d",
				AR0822_REG_ADDR(AR0822_ATOMIC), ret);
		return ret;
	}

	msleep(200);

	/* Validating the values */
	check_val(ar0822, AR0822_SYSTEM_FREQ_IN, TO_S15_16(48));
	check_val(ar0822, AR0822_HINF_MIPI_FREQ_TGT, TO_S15_16(600));
	check_val(ar0822, AR0822_PREVIEW_WIDTH, AR0822_COMMON_WIDTH);
	check_val(ar0822, AR0822_PREVIEW_HEIGHT, AR0822_COMMON_HEIGHT);
	check_val(ar0822, AR0822_PREVIEW_MAX_FPS, PREVIEW_MAX_FPS(30));

	return ret;
}

static int ar0822_probe(struct i2c_client *client)
{
	struct ar0822 *ar0822;
	int ret;

	ar0822 = devm_kzalloc(&client->dev, sizeof(*ar0822), GFP_KERNEL);
	if (!ar0822)
	{
		dev_err(&client->dev, "ar0822 devm_kzalloc failed\n");
		return -ENOMEM;
	}

	ret = ar0822_parse_fwnode(ar0822, &client->dev);
	if (ret)
	{
		return ret;
	}
	v4l2_i2c_subdev_init(&ar0822->sd, client, &ar0822_subdev_ops);

	ret = ar0822_parse_gpio(ar0822, &client->dev);
	if (ret) {
		dev_err(&client->dev, "failed to get GPIO control: %d\n", ret);
		return ret;
	}

	ar0822_gpio_reset(ar0822);
	dev_info (ar0822->sd.dev, "ar0822 reset gpio success.\n");

	ar0822->cur_mode = &supported_modes[0];
	ret = ar0822_init_controls(ar0822);
	if (ret) {
		dev_err(&client->dev, "failed to init controls: %d", ret);
		goto probe_error_v4l2_ctrl_handler_free;
	}

	ar0822->sd.internal_ops = &ar0822_internal_ops;
	ar0822->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_HAS_EVENTS;
	ar0822->sd.entity.ops = &ar0822_subdev_entity_ops;
	ar0822->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	ar0822->pad.flags = MEDIA_PAD_FL_SOURCE;
	ret = media_entity_pads_init(&ar0822->sd.entity, 1, &ar0822->pad);
	if (ret) {
		dev_err(&client->dev, "failed to init entity pads: %d", ret);
		goto probe_error_v4l2_ctrl_handler_free;
	}

	ar0822->sd.state_lock = ar0822->ctrl_handler.lock;
	ar0822->sd.dev = &client->dev;
	ret = v4l2_subdev_init_finalize(&ar0822->sd);
	if (ret < 0) {
		dev_err(&client->dev, "v4l2 subdev init error: %d", ret);
		goto probe_error_media_entity_cleanup;
	}

	ar0822->regmap = devm_regmap_init_i2c(client, &ar0822_reg16_config);
	if (IS_ERR(ar0822->regmap)) {
		dev_err(&client->dev, "error init sensor regmap 16 : %ld\n", PTR_ERR(ar0822->regmap));
		return -ENODEV;
	}

	ar0822->regmap32 = devm_regmap_init_i2c(client, &ar0822_reg32_config);
	if (IS_ERR(ar0822->regmap32)) {
		dev_err(&client->dev, "error init sensor regmap 32 : %ld\n", PTR_ERR(ar0822->regmap32));
		return -ENODEV;
    }

	msleep(100);

	ret = ar0822_identify_module(ar0822);
	if (ret) {
		dev_err(&client->dev, "failed to find sensor: %d", ret);
		return ret;
	}

	ret = ar0822_board_setup(ar0822);
	if (ret)
	{
		dev_err(&client->dev, "board setup failed: %d", ret);
		goto probe_error_media_entity_cleanup;
	}
#ifdef CONT_CLK_ENABLE
	ar0822_setup_cont_clk(ar0822, true);
#endif
	ret = ar0822_stall(ar0822, true);
	if (ret) {
			dev_err (ar0822->sd.dev, "error to stall: %d", ret);
			return ret;
	}

	/*
	 * Device is already turned on by i2c-core with ACPI domain PM.
	 * Enable runtime PM and turn off the device.
	 */
	pm_runtime_set_active(&client->dev);
	pm_runtime_enable(&client->dev);
	pm_runtime_idle(&client->dev);

	ret = v4l2_async_register_subdev_sensor(&ar0822->sd);
	if (ret < 0) {
		dev_err(&client->dev, "failed to register V4L2 subdev: %d",
			ret);
		goto probe_error_rpm;
	}

	return 0;

probe_error_rpm:
	pm_runtime_disable(&client->dev);
	v4l2_subdev_cleanup(&ar0822->sd);

probe_error_media_entity_cleanup:
	media_entity_cleanup(&ar0822->sd.entity);

probe_error_v4l2_ctrl_handler_free:
	v4l2_ctrl_handler_free(ar0822->sd.ctrl_handler);
	return ret;
}

static int __maybe_unused ar0822_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ar0822 *ar0822 = to_ar0822(sd);

	dev_dbg(&client->dev, "%s\n", __func__);
	gpiod_set_value_cansleep(ar0822->reset_gpio, 1);

	return 0;
}

static int __maybe_unused ar0822_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ar0822 *ar0822 = to_ar0822(sd);
	int ret = 0;
	int count = 0;
	u32 val = 0;

	dev_dbg(&client->dev, "%s\n", __func__);

	if (ar0822->reset_gpio != NULL) {
		do {
			gpiod_set_value_cansleep(ar0822->reset_gpio, 0);
			ret = gpiod_get_value_cansleep(ar0822->reset_gpio);
			usleep_range(200 * 1000, 200 * 1000 + 500);

            if (++count >= 10) {
                dev_err(&client->dev, "%s: failed to power on reset gpio, reset gpio is %d", __func__, ret);
                return -ETIMEDOUT;
            }

		} while (ret != 0);
	}

	ret = ar0822_read(ar0822, AR0822_BOOTDATA_STAGE, &val);
	dev_dbg(&client->dev, "%s Validating bootstage data: 0x%x \n", __func__, val);

	if (val != BOOTSTAGE_COMPLETE) {
		ret = ar0822_board_setup(ar0822);
		if (ret) {
			dev_err(&client->dev, "Setting up board again failed");
			return ret;
		}

		ret = ar0822_stall(ar0822, true);
		if (ret) {
			dev_err (ar0822->sd.dev, "error to stall: %d", ret);
			return ret;
		}
	}

	return 0;
}

static const struct dev_pm_ops ar0822_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ar0822_suspend, ar0822_resume)
};

static const struct acpi_device_id ar0822_acpi_ids[] = {
	{ "EV8MOOM1" },
	{}
};
MODULE_DEVICE_TABLE(acpi, ar0822_acpi_ids);

static struct i2c_driver ar0822_i2c_driver = {
	.driver = {
		.name = "ar0822",
		.acpi_match_table = ACPI_PTR(ar0822_acpi_ids),
		.pm = &ar0822_pm_ops,
	},
	.probe = ar0822_probe,
	.remove = ar0822_remove,
};

module_i2c_driver(ar0822_i2c_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yi-Chen, Liu <yichen_liu@innodisk.com>");
MODULE_AUTHOR("Ng Khai Wen <khai.wen.ng@intel.com>");
MODULE_AUTHOR("Ch'ng Seng Guan <seng.guan.chng@intel.com>");
MODULE_AUTHOR("Jonathan Lui <jonathan.ming.jun.lui@intel.com>");
MODULE_DESCRIPTION("Innodisk EV8M-OOM1 ar0822 mimic ar0822");
MODULE_ALIAS("Innodisk EV8M-OOM1");
MODULE_VERSION("v20251024_2");

/*
 *	v1.0: first steady version
 *	v1.1: support Linux Kernel 6.10
 *  v1.2: support Linux Kernel 6.12
 */
