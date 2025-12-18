/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * include/media/i2c/ar0822.h
 *
 * Copyright (C) 2025 Intel Corporation
 *
 */

#ifndef AR0822_H
#define AR0822_H

#define AR0822_REG_16BIT(n)			((2 << 24) | (n))
#define AR0822_CON_BUF(n)			AR0822_REG_16BIT(0x0a2c + (n))
#define AR0822_CON_BUF_SIZE			512

#define AR0822_REG_VALUE_08BIT			1
#define AR0822_REG_VALUE_16BIT			2
#define AR0822_REG_VALUE_32BIT			4
#define AR0822_VERSION_LEN_MAX			64

#define BOOTSTAGE_COMPLETE			0xFFFF

#define AR0822_REG_CHIP_ID				AR0822_REG_16BIT(0x0000)
#define AR0822_ADV_HINF_MIPI_INTERNAL_LANE_CLK_CTL				AR0822_REG_32BIT(0x0085008C)
#define AR0822_CHIP_ID					0x0265

#define AR0822_LINK_FREQ_360MHZ			360000000ULL
#define AR0822_COMMON_WIDTH				3840
#define AR0822_COMMON_HEIGHT			2160
#define AR0822_PIXEL_ARRAY_LEFT			320	// To-Do
#define AR0822_PIXEL_ARRAY_TOP			60	// To-Do

#define AR0822_CID_CSI_PORT         	(V4L2_CID_USER_BASE | 0x1001)
#define AR0822_CID_I2C_BUS         		(V4L2_CID_USER_BASE | 0x1002)
#define AR0822_CID_I2C_ID         		(V4L2_CID_USER_BASE | 0x1003)
#define AR0822_CID_I2C_SLAVE_ADDRESS	(V4L2_CID_USER_BASE | 0x1004)
#define AR0822_CID_FPS         			(V4L2_CID_USER_BASE | 0x1005)
#define AR0822_CID_FRAME_INTERVAL		(V4L2_CID_USER_BASE | 0x1006)

#define to_ar0822(_sd)					container_of(_sd, struct ar0822, sd)

#define MIPI_CSI2_TYPE_YUV422_8			0x1e

/* AR0822 ISP register address */
#define AR0822_PREVIEW_WIDTH				AR0822_REG_16BIT(0x2000)
#define AR0822_PREVIEW_HEIGHT				AR0822_REG_16BIT(0x2002)
#define AR0822_PREVIEW_MAX_FPS				AR0822_REG_16BIT(0x2020)
#define AR0822_BOOTDATA_STAGE				AR0822_REG_16BIT(0x6002)
#define AR0822_SYSTEM_FREQ_IN				AR0822_REG_32BIT(0x6024)
#define AR0822_HINF_MIPI_FREQ_TGT			AR0822_REG_32BIT(0x6034)
#define AR0822_BOOTDATA_CHECKSUM			AR0822_REG_16BIT(0x6134)
#define AR0822_SIP_CRC						AR0822_REG_16BIT(0xF052)
#define AR0822_TRIG_CTRL					AR0822_REG_16BIT(0x1186)
#define AR0822_ATOMIC						AR0822_REG_16BIT(0x1184)
#define ATOMIC_RECORD						BIT(0)
#define ATOMIC_FINISH						BIT(1)
#define ATOMIC_MODE							BIT(2)
#define ATOMIC_UPDATE_FORMAT				BIT(3)
#define ATOMIC_UPDATE						BIT(4)
#define ATOMIC_UPDATE_ALL					(ATOMIC_UPDATE | ATOMIC_UPDATE_FORMAT | ATOMIC_MODE | ATOMIC_FINISH | ATOMIC_RECORD)

#define AR0822_FIRMWARE_WINDOW_OFFSET		0x8000
#define AR0822_FIRMWARE_WINDOW_SIZE			0x2000
#define AR0822_FIRMWARE_BUFFER				0x0FF0
#define PREVIEW_HINF_CTRL_MIPI_MASK			(7U << 0)
#define HINF_CTRL_CONT_CLK_MASK				(1U << 5)
#define HINF_CTRL_DIS_CONT_CLK				(0U << 5)
#define HINF_CTRL_EN_CONT_CLK				(1U << 5)

#define EV8M_OOM1_FW_FILENAME			"Innodisk_EV8M_OOM1.bin"

/* Misc Registers */
#define AR0822_REG_ADDR(n)								((n) & 0x0000ffff)
#define AR0822_REG_PAGE(n)								((n) & 0x00ff0000)
#define AR0822_REG_SIZE(n)								((n) >> 24)
#define AR0822_REG_PAGE_MASK							0x00ff0000
#define AR0822_REG_16BIT(n)								((2 << 24) | (n))
#define AR0822_REG_32BIT(n)								((4 << 24) | (n))

#define PREVIEW_MAX_FPS(fps)							(fps << 8)
#define TO_S15_16(x)									((s32)((x) << 16))

#define AR0822_REG_ADV_START							0xe000
#define AR0822_ADVANCED_BASE							AR0822_REG_32BIT(0xf038)
#define AR0822_HINF_MIPI_FREQ 							AR0822_REG_32BIT(0x0068)
#define AR0822_PRI_SENSOR_FREQ 							AR0822_REG_32BIT(0x0DC4)
#define AR0822_PREVIEW_SENSOR_MODE 						AR0822_REG_16BIT(0x2014)
#define AR0822_PREVIEW_HINF_CTRL						AR0822_REG_16BIT(0x2030)
#define AR0822_SNAPSHOT_HINF_CTRL 						AR0822_REG_16BIT(0x3030)

#define AR0822_SYS_START								AR0822_REG_16BIT(0x601a)
#define AR0822_SYS_START_PLL_LOCK						BIT(15)
#define AR0822_SYS_START_LOAD_OTP						BIT(12)
#define AR0822_SYS_START_RESTART_ERROR					BIT(11)
#define AR0822_SYS_START_STALL_STATUS					BIT(9)
#define AR0822_SYS_START_STALL_EN						BIT(8)
#define AR0822_SYS_START_STALL_MODE_FRAME				(0U << 6)
#define AR0822_SYS_START_STALL_MODE_DISABLED			(1U << 6)
#define AR0822_SYS_START_STALL_MODE_STANDBY				(2U << 6)
#define AR0822_SYS_START_STALL_MODE_STANDBY_SENSOR_OFF	(3U << 6)
#define AR0822_SYS_START_GO								BIT(4)
#define AR0822_SYS_START_PATCH_FUN						BIT(1)
#define AR0822_SYS_START_PLL_INIT						BIT(0)
#define REGMAP_WAIT_5S									5000000

#ifdef DEBUG
#define AR0822_CON_BUF_SIZE								512
#define AR0822_DBG										AR0822_REG_16BIT(0x6000)
#define AR0822_CON_BUF(n)								AR0822_REG_16BIT(0x0a2c + (n))
#define AR0822_WARNING(n)								AR0822_REG_16BIT(0x6004 + (n) * 2)
#define AR0822_ADV_SINF_MIPI_INTERNAL_p_LANE_n_STAT(p, n) \
	AR0822_REG_32BIT(0x00420008 + (p) * 0x50000 + (n) * 0x20)
#define AR0822_LANE_ERR_LP_VAL(n)						(((n) >> 30) & 3)
#define AR0822_LANE_ERR_STATE(n)						(((n) >> 24) & 0xf)
#define AR0822_LANE_ERR									BIT(18)
#define AR0822_LANE_ABORT								BIT(17)
#define AR0822_LANE_LP_VAL(n)							(((n) >> 6) & 3)
#define AR0822_LANE_STATE(n)							((n) & 0xf)
#define AR0822_LANE_STATE_STOP_S						0x0
#define AR0822_LANE_STATE_HS_REQ_S						0x1
#define AR0822_LANE_STATE_LP_REQ_S						0x2
#define AR0822_LANE_STATE_HS_S							0x3
#define AR0822_LANE_STATE_LP_S							0x4
#define AR0822_LANE_STATE_ESC_REQ_S						0x5
#define AR0822_LANE_STATE_TURN_REQ_S					0x6
#define AR0822_LANE_STATE_ESC_S							0x7
#define AR0822_LANE_STATE_ESC_0							0x8
#define AR0822_LANE_STATE_ESC_1							0x9
#define AR0822_LANE_STATE_TURN_S						0xa
#define AR0822_LANE_STATE_TURN_MARK						0xb
#define AR0822_LANE_STATE_ERROR_S						0xc

#define AR0822_HINF_MIPI_INTERNAL_LANE_n_CTRL(n) \
	AR0822_REG_32BIT(0x00850008 + (n) * 0x20)
#endif // DEBUG

struct ar0822_mode {
	/* Frame width in pixels */
	u32 width;

	/* Frame height in pixels */
	u32 height;

	/* Horizontal timining size */
	u32 hts;

	/* Default vertical timining size */
	u32 vts_def;

	/* Min vertical timining size */
	u32 vts_min;

	/* Link frequency needed for this resolution */
	u32 link_freq_index;

	/* MEDIA_BUS_FMT */
	u32 code;

	/* MIPI_LANES */
	s32 lanes;

	/* MODE_FPS*/
	u32 fps;

	/* bit per pixel */
	u32 bpp;
};

static const s64 link_freq_menu_items[] = {
	AR0822_LINK_FREQ_360MHZ,
};

static const struct ar0822_mode supported_modes[] = {
	{
		.width 			 = 3840,
		.height 		 = 2160,
		.code			 = MEDIA_BUS_FMT_UYVY8_1X16,
		.lanes 	 		 = 2,
		.fps 	 		 = 30,
		.bpp 			 = 16,
	},
};

static u32 supported_formats[] = {
	MEDIA_BUS_FMT_UYVY8_1X16
};

struct v4l2_fwnode_endpoint bus_cfg = {
	.bus_type = V4L2_MBUS_CSI2_DPHY,
};

struct ar0822_firmware {
	u32 magic;
	u32 version;
	char desc [256];
	u16 pll_init_size;
	u16 crc;
} __packed;

struct ar0822 {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_ctrl_handler ctrl_handler;
	struct regmap *regmap;
	struct regmap *regmap32;
	u32 reg_page;

	/* V4L2 Controls */
	struct v4l2_ctrl *link_freq;
	struct v4l2_ctrl *vblank;
	struct v4l2_ctrl *exposure;
	struct v4l2_ctrl *analogue_gain;
	struct v4l2_ctrl *digital_gain;
	struct v4l2_ctrl *strobe_source;
	struct v4l2_ctrl *strobe;
	struct v4l2_ctrl *strobe_stop;
	struct v4l2_ctrl *timeout;
	struct v4l2_ctrl *csi_port;
	struct v4l2_ctrl *i2c_bus;
	struct v4l2_ctrl *i2c_id;
	struct v4l2_ctrl *i2c_slave_address;
	struct v4l2_ctrl *fps;
	struct v4l2_ctrl *frame_interval;
	struct v4l2_ctrl *pixel_rate;
	struct v4l2_ctrl *hblank;
	struct v4l2_ctrl *vflip;
	struct v4l2_ctrl *hflip;

	struct gpio_desc *reset_gpio;

	unsigned long link_freq_bitmap;

	/* Current mode */
	const struct ar0822_mode *cur_mode;

	const struct firmware *firmware;

	/* Periodic console logging */
	struct delayed_work log_work;
	bool logging_enabled;
	unsigned int log_interval_ms;
};

static const struct regmap_config ar0822_reg16_config = {
	.reg_bits = 16,
	.val_bits = 16,
	.reg_stride = 2,
	.reg_format_endian = REGMAP_ENDIAN_BIG,
	.val_format_endian = REGMAP_ENDIAN_BIG,
	.cache_type = REGCACHE_NONE,
};

static const struct regmap_config ar0822_reg32_config = {
	.reg_bits = 16,
	.val_bits = 32,
	.reg_stride = 4,
	.reg_format_endian = REGMAP_ENDIAN_BIG,
	.val_format_endian = REGMAP_ENDIAN_BIG,
	.cache_type = REGCACHE_NONE,
};

static int ar0822_board_setup 	 (struct ar0822 *ar0822);
static int ar0822_load_firmware (struct ar0822 *ar0822);
static void ar0822_gpio_reset 	 (struct ar0822 *ar0822);
static int ar0822_set_ctrl(struct v4l2_ctrl *ctrl);

static const struct v4l2_ctrl_ops ar0822_ctrl_ops = {
	.s_ctrl = ar0822_set_ctrl,
};

static struct v4l2_ctrl_config ar0822_i2c_bus = {
	.ops	= &ar0822_ctrl_ops,
	.id		= AR0822_CID_I2C_BUS,
	.type	= V4L2_CTRL_TYPE_INTEGER,
	.name	= "I2C bus",
	.min	= 0,
	.max	= MINORMASK,
	.def	= 0,
	.step	= 1,
	.flags	= V4L2_CTRL_FLAG_READ_ONLY,
};

static struct v4l2_ctrl_config ar0822_i2c_id = {
	.ops	= &ar0822_ctrl_ops,
	.id		= AR0822_CID_I2C_ID,
	.type	= V4L2_CTRL_TYPE_INTEGER,
	.name	= "I2C id",
	.min	= 0x3D,
	.max	= 0x77,
	.def	= 0x3D,
	.step	= 1,
	.flags	= V4L2_CTRL_FLAG_READ_ONLY,
};

static struct v4l2_ctrl_config ar0822_fps = {
	.ops	= &ar0822_ctrl_ops,
	.id		= AR0822_CID_FPS,
	.type	= V4L2_CTRL_TYPE_INTEGER,
	.name	= "fps",
	.min	= 1,
	.max	= 120,
	.def	= 30,
	.step	= 1,
	.flags	= V4L2_CTRL_FLAG_READ_ONLY,
};

static struct v4l2_ctrl_config ar0822_frame_interval = {
	.ops	= &ar0822_ctrl_ops,
	.id		= AR0822_CID_FRAME_INTERVAL,
	.type	= V4L2_CTRL_TYPE_INTEGER,
	.name	= "frame interval",
	.min	= 0,
	.max	= 1000,
	.def	= 25,
	.step	= 1,
	.flags	= V4L2_CTRL_FLAG_READ_ONLY,
};

#ifdef DEBUG
#define AR0822_ADV_CAPTURE_A_FV_CNT		AR0822_REG_32BIT(0x00490040)
#define AR0822_FRAME_CNT			AR0822_REG_16BIT(0x0002)
static const char * const ar0822_warnings[] = {
	"HINF_BANDWIDTH",
	"FLICKER_DETECTION",
	"FACED_NE",
	"SMILED_NE",
	"HINF_OVERRUN",
	NULL,
	"FRAME_TOO_SMALL",
	"MISSING_PHASES",
	"SPOOF_UNDERRUN",
	"JPEG_NOLAST",
	"NO_IN_FREQ_SPEC",
	"SINF0",
	"SINF1",
	"CAPTURE0",
	"CAPTURE1",
	"ISR_UNHANDLED",
	"INTERLEAVE_SPOOF",
	"INTERLEAVE_BUF",
	"COORD_OUT_OF_RANGE",
	"ICP_CLOCKING",
	"SENSOR_CLOCKING",
	"SENSOR_NO_IHDR",
	"DIVIDE_BY_ZERO",
	"INT0_UNDERRUN",
	"INT1_UNDERRUN",
	"SCRATCHPAD_TOO_BIG",
	"OTP_RECORD_READ",
	"NO_LSC_IN_OTP",
	"GPIO_INT_LOST",
	"NO_PDAF_DATA",
	"FAR_PDAF_ACCESS_SKIP",
	"PDAF_ERROR",
	"ATM_TVI_BOUNDS",
	"SIPM_0_RTY",
	"SIPM_1_TRY",
	"SIPM_0_NO_ACK",
	"SIPM_1_NO_ACK",
	"SMILE_DIS",
	"DVS_DIS",
	"TEST_DIS",
	"SENSOR_LV2LV",
	"SENSOR_FV2FV",
	"FRAME_LOST",
};

static const char * const ar0822_lane_states[] = {
	"stop_s",
	"hs_req_s",
	"lp_req_s",
	"hs_s",
	"lp_s",
	"esc_req_s",
	"turn_req_s",
	"esc_s",
	"esc_0",
	"esc_1",
	"turn_s",
	"turn_mark",
	"error_s",
};
#endif // DEBUG

#endif /* AR0822_H */
