#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/v4l2-mediabus.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>


enum fpga_fps_e
{
	FPS_30 = 0,
	FPS_60
};
static int fpga_fps[] = {
	[FPS_30] = 30,
	[FPS_60] = 60,
};

struct fpga_resolution
{
	int width;
	int height;
	int fps;
};
struct fpga_resolution fpga_valid_res[] = {
	[0] = {640, 512},
};
#define MAX_NUMBER_RES ARRAY_SIZE(fpga_valid_res)
struct fpga_datafmt {
	u32	code;
	enum v4l2_colorspace		colorspace;
};
#define MEDIA_BUS_FMT_META_8 0x8001
static const struct fpga_datafmt fpga_colour_fmts[] = {
	{MEDIA_BUS_FMT_META_8, V4L2_COLORSPACE_JPEG},
	{},
};
struct fpga_csi {
	struct v4l2_subdev			subdev;
	struct v4l2_pix_format 		pix;
	const struct fpga_datafmt	*fmt;
	struct v4l2_captureparm 	streamcap;
	struct device * 			dev;
};

static const struct of_device_id fpga_of_match[] = {
	{ .compatible = "raw,4-line", },
	{ },
};

static const struct fpga_datafmt
			*fpga_find_datafmt(u32 code)
{
	printk("AAAA fpga_find_datafmt\r\n");
	int i;

	for (i = 0; i < ARRAY_SIZE(fpga_colour_fmts); i++)
	{
		if (fpga_colour_fmts[i].code == code)
		{
			return fpga_colour_fmts + i;
		}
	}

	return NULL;
}
static int get_capturemode(int width, int height)
{
	printk("AAAA get_capturemode\r\n");
	int i;
	for (i = 0; i < ARRAY_SIZE(fpga_valid_res); i++) {
		if ((fpga_valid_res[i].width == width) &&
		     (fpga_valid_res[i].height == height))
			 {
				return i;
			 }			
	}
	return -1;
}
static int fpga_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *a)
{
	printk("AAAA fpga_g_parm\r\n");
	struct fpga_csi *fpga = v4l2_get_subdevdata(sd);
	struct v4l2_captureparm *cparm = &a->parm.capture;
	int ret = 0;
	switch (a->type) {
	/* This is the only case currently handled. */
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		memset(a, 0, sizeof(*a));
		a->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		cparm->capability 	= fpga->streamcap.capability;
		cparm->timeperframe = fpga->streamcap.timeperframe;
		cparm->capturemode 	= fpga->streamcap.capturemode;
		ret = 0;
		break;

	/* These are all the possible cases. */
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	case V4L2_BUF_TYPE_VBI_CAPTURE:
	case V4L2_BUF_TYPE_VBI_OUTPUT:
	case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
	case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
		ret = -EINVAL;
		break;

	default:
		printk("Unknown type %d\n", a->type);
		ret = -EINVAL;
		break;
	}

	return ret;
}
static int fpga_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *a)
{
	printk("AAAA fpga_s_parm\r\n");
	struct fpga_csi *fpga = v4l2_get_subdevdata(sd);
	struct v4l2_fract *timeperframe = &a->parm.capture.timeperframe;
	u32 tgt_fps;	/* target frames per secound */
	int ret = 0;
	int max_fps = fpga_fps[FPS_60];
	int min_fps = fpga_fps[FPS_30];
	switch (a->type) {
	/* This is the only case currently handled. */
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		/* Check that the new frame rate is allowed. */
		if ((timeperframe->numerator == 0) ||
		    (timeperframe->denominator == 0)) {
			timeperframe->denominator = fpga_fps[FPS_60];
			timeperframe->numerator = 1;
		}

		tgt_fps = timeperframe->denominator /
			  timeperframe->numerator;

		if (tgt_fps > max_fps) {
			timeperframe->denominator = max_fps;
			timeperframe->numerator = 1;
		} else if (tgt_fps < min_fps) {
			timeperframe->denominator = min_fps;
			timeperframe->numerator = 1;
		}

		fpga->streamcap.timeperframe = *timeperframe;
		fpga->streamcap.capturemode =
				(u32)a->parm.capture.capturemode;

		break;

	/* These are all the possible cases. */
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	case V4L2_BUF_TYPE_VBI_CAPTURE:
	case V4L2_BUF_TYPE_VBI_OUTPUT:
	case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
	case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
		printk( "Type is not V4L2_BUF_TYPE_VIDEO_CAPTURE but %d\n",a->type);
		ret = -EINVAL;
		break;

	default:
		printk("Type is unknown - %d\n", a->type);
		ret = -EINVAL;
		break;
	}

	return ret;
}
static int fpga_s_stream(struct v4l2_subdev *sd, int enable)
{
	printk("AAAA fpga_s_stream\r\n");
	struct fpga_csi *fpga = v4l2_get_subdevdata(sd);	
	return 0;
}
static struct v4l2_subdev_video_ops fpga_subdev_video_ops = {
	.s_stream = fpga_s_stream,
};
static int fpga_enum_framesizes(struct v4l2_subdev *sd,
			       struct v4l2_subdev_state *sd_state,
			       struct v4l2_subdev_frame_size_enum *fse)
{
	printk("AAAA fpga_enum_framesizes\r\n");
	if (fse->index >= MAX_NUMBER_RES)
		return -EINVAL;
	printk("Call %s\r\n",__func__);
	fse->max_width =fpga_valid_res[fse->index].width;			
	fse->min_width = fse->max_width;

	fse->max_height =fpga_valid_res[fse->index].height;
	fse->min_height = fse->max_height;
	return 0;
}
static int fpga_enum_frameintervals(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_frame_interval_enum *fie)
{
	printk("AAAA fpga_enum_frameintervals\r\n");
	struct fpga_csi *fpga = v4l2_get_subdevdata(sd);
	int i, j, count = 0;
	if (fie->index >= MAX_NUMBER_RES)
		return -EINVAL;

	if (fie->width == 0 || fie->height == 0 ||
	    fie->code == 0) {
		printk("Please assign pixel format, width and height\n");
		return -EINVAL;
	}

	fie->interval.numerator = 1;

	count = 0;
	for (i = 0; i < MAX_NUMBER_RES; i++) {
			if (fie->width == fpga_valid_res[i].width
			 && fie->height == fpga_valid_res[i].height){
				count++;
			}
			if (fie->index == (count - 1)) {
				fie->interval.denominator = fpga_fps[FPS_60];
				return 0;
			}
	}

	return -EINVAL;
}
static int fpga_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *sd_state,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	printk("AAAA fpga_enum_mbus_code\r\n");
	if (code->pad || code->index >= ARRAY_SIZE(fpga_colour_fmts))
		return -EINVAL;
	code->code = fpga_colour_fmts[code->index].code;
	return 0;
}
static int fpga_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_state *sd_state,
			struct v4l2_subdev_format *format)
{
	printk("AAAA fpga_set_fmt\r\n");
	struct v4l2_mbus_framefmt *mf = &format->format;
	const struct fpga_datafmt *fmt = fpga_find_datafmt(mf->code);
	struct fpga_csi *fpga = v4l2_get_subdevdata(sd);
	int capturemode;
	if (!fmt) {
		mf->code		= fpga_colour_fmts[0].code;
		mf->colorspace	= fpga_colour_fmts[0].colorspace;
		fmt				= &fpga_colour_fmts[0];
	}
	mf->code		= fpga_colour_fmts[0].code;
	mf->colorspace	= fpga_colour_fmts[0].colorspace;
	mf->field		= V4L2_FIELD_NONE;

	if (format->which == V4L2_SUBDEV_FORMAT_TRY)
		return 0;

	fpga->fmt = fmt;

	capturemode = get_capturemode(mf->width, mf->height);
	if (capturemode >= 0) {
		fpga->streamcap.capturemode = capturemode;
		fpga->pix.width = mf->width;
		fpga->pix.height = mf->height;
		return 0;
	}

	return -EINVAL;
}
static int fpga_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *sd_state,
			  struct v4l2_subdev_format *format)
{
	printk("AAAA fpga_get_fmt\r\n");
	struct v4l2_mbus_framefmt *mf = &format->format;
	struct fpga_csi *fpga = v4l2_get_subdevdata(sd);
	const struct fpga_datafmt *fmt = fpga->fmt;
	if (format->pad)
		return -EINVAL;
	
	mf->code		= fmt->code;
	mf->colorspace	= fmt->colorspace;
	mf->field		= V4L2_FIELD_NONE;

	mf->width	= fpga->pix.width;
	mf->height	= fpga->pix.height;
	
	return 0;
}

#define TAX_NATIVE_WIDTH 		640U
#define TAX_NATIVE_HEIGHT 		512U
#define TAX_PIXEL_ARRAY_LEFT 	8U
#define TAX_PIXEL_ARRAY_TOP 	16U // TODO: ?
#define TAX_PIXEL_ARRAY_WIDTH 	4056U
#define TAX_PIXEL_ARRAY_HEIGHT 	3040U

static int tax_get_selection(struct v4l2_subdev *sd,
				struct v4l2_subdev_state *sd_state,
				struct v4l2_subdev_selection *sel)
{
	printk("TAX GET SELECTION!!!!\r\n");
	// switch (sel->target) {
	// case V4L2_SEL_TGT_CROP: {
	// 	struct imx500 *imx500 = to_imx500(sd);

	// 	mutex_lock(&imx500->mutex);
	// 	sel->r = *__imx500_get_pad_crop(imx500, sd_state, sel->pad,
	// 					sel->which);
	// 	mutex_unlock(&imx500->mutex);

	// 	return 0;
	// }

	// case V4L2_SEL_TGT_NATIVE_SIZE:
	// 	sel->r.left = 0;
	// 	sel->r.top = 0;
	// 	sel->r.width = IMX500_NATIVE_WIDTH;
	// 	sel->r.height = IMX500_NATIVE_HEIGHT;

	// 	return 0;

	// case V4L2_SEL_TGT_CROP_DEFAULT:
	// case V4L2_SEL_TGT_CROP_BOUNDS:
	// 	sel->r.left = IMX500_PIXEL_ARRAY_LEFT;
	// 	sel->r.top = IMX500_PIXEL_ARRAY_TOP;
	// 	sel->r.width = IMX500_PIXEL_ARRAY_WIDTH;
	// 	sel->r.height = IMX500_PIXEL_ARRAY_HEIGHT;

	// 	return 0;
	// }

	return 0;
}
static const struct v4l2_subdev_pad_ops fpga_subdev_pad_ops = {
	.enum_mbus_code		   	= fpga_enum_mbus_code,
	.set_fmt               	= fpga_set_fmt,
	.get_fmt               	= fpga_get_fmt,
	.get_selection 			= tax_get_selection,
	.enum_frame_size       	= fpga_enum_framesizes,
};
static int fpga_s_power(struct v4l2_subdev *sd, int on)
{
	struct fpga_csi *fpga = v4l2_get_subdevdata(sd);

	printk("fpga s_power: %d\n", on);
	return 0;
}
static struct v4l2_subdev_core_ops fpga_subdev_core_ops = {
	.s_power	= fpga_s_power,
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
};

static struct v4l2_subdev_ops fpga_subdev_ops = {
	.core	= &fpga_subdev_core_ops,
	.video	= &fpga_subdev_video_ops,
	.pad	= &fpga_subdev_pad_ops,
};
static int fpga_init_subdev(struct fpga_csi *fpga,struct device	*dev)
{	
	printk("INIT TAX 1\r\n");
	struct v4l2_subdev *sd = &fpga->subdev;
	v4l2_subdev_init(sd, &fpga_subdev_ops);
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	sd->dev = dev;
	sd->name[0] = "fpga_mipi\0";
	sd->owner = THIS_MODULE;
	v4l2_set_subdevdata(sd, fpga);
	sd->grp_id = 678;
	int retval = v4l2_async_register_subdev(sd);
	return retval;
}
static int fpga_probe(struct platform_device *pdev)
{	
	struct device	*dev = &pdev->dev;
	struct fpga_csi *fpga;
	int retval = 0;
	fpga = devm_kzalloc(dev, sizeof(*fpga), GFP_KERNEL);
	if (!fpga)
		return -ENOMEM;
	platform_set_drvdata(pdev, fpga);
	fpga->dev = dev;
	fpga->pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fpga->pix.width = 800;
	fpga->pix.height = 600;
	fpga->streamcap.capability = V4L2_MODE_HIGHQUALITY |
					   V4L2_CAP_TIMEPERFRAME;
	fpga->streamcap.capturemode = 0;
	fpga->streamcap.timeperframe.denominator = fpga_fps[FPS_60];
	fpga->streamcap.timeperframe.numerator = 1;

	retval =fpga_init_subdev(fpga,dev);
	if (retval < 0)
		printk("Async register failed, ret=%d\n",retval);
	printk ("V4l reg succeful Fpga probe\r\n");
	return retval;
}

static void fpga_remove(struct platform_device *pdev)
{
	struct fpga_csi *fpga= platform_get_drvdata(pdev);
	struct v4l2_subdev *sd = &fpga->subdev;

	v4l2_async_unregister_subdev(sd);
	kfree(fpga);
}
MODULE_DEVICE_TABLE(of, fpga_of_match);

static struct platform_driver fpga_csi_driver = {
	.driver = {
		.name = "raw_mipi",
		.of_match_table = fpga_of_match,
	},
	.probe = fpga_probe,
	.remove = fpga_remove,
};

module_platform_driver(fpga_csi_driver);

MODULE_AUTHOR("Aliaksei Yablonski aliaksei.y@noctavis.com");
MODULE_DESCRIPTION("Simple mipi driver for fpga driver");
MODULE_LICENSE("GPL v2");