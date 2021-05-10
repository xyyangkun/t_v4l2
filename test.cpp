/******************************************************************************
 *
 *       Filename:  test.c
 *
 *    Description:  test v4l2
 *    get --list-formats
 *    --list-formats-ext  
 *    set formats
 *    get datas
 *     获取支持分辨率帧率
 *
 *
 *        Version:  1.0
 *        Created:  2021年05月10日 15时23分13秒
 *       Revision:  none
 *       Compiler:  gcc
		https://www.kernel.org/doc/html/v4.14/media/uapi/v4l/vidioc-enum-fmt.html#c.v4l2_fmtdesc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <map>

#include <stdlib.h>
#include <unistd.h>

#define sysfail(msg) { printf ("%s failed: %s\n", (msg), strerror (errno)); return -1; }
void
usage(const char*progname)
{
	  printf("usage: %s <videodevice>\n", progname);
	    exit(1);
}

//static const __u32 v4l2_pixel_format_map[4] = {875967048, 0, 1196444237, 1448695129};
static const __u32 v4l2_pixel_format_map[] = {V4L2_PIX_FMT_H264, 0, V4L2_PIX_FMT_MJPEG, V4L2_PIX_FMT_YUYV};
 
int v4l2_is_v4l_dev(const char *name)
{
	return !strncmp(name, "video", 5) ||
		!strncmp(name, "radio", 5) ||
		!strncmp(name, "vbi", 3) ||
		!strncmp(name, "v4l-subdev", 10);
}


int test_v4l2_get_video_device_info()
{
	std::map<std::string, std::string> device_list;
	//test_v4l2_get_device_list(device_list);
}

#define v4l2_fourcc(a,b,c,d) (((__u32)(a)<<0)|((__u32)(b)<<8)|((__u32)(c)<<16)|((__u32)(d)<<24))
// https://stackoverflow.com/questions/15112134/how-to-get-list-of-supported-frame-size-and-frame-interval-of-webcam-device-usin
void printFrameInterval(int fd, unsigned int fmt, unsigned int width, unsigned int height)
{
    struct v4l2_frmivalenum frmival;
    memset(&frmival,0,sizeof(frmival));
    frmival.pixel_format = fmt;
    frmival.width = width;
    frmival.height = height;
    while (ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) == 0) 
    {
        if (frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE) 
            printf("[%dx%d] %f fps\n", width, height, 1.0*frmival.discrete.denominator/frmival.discrete.numerator);
        else
            printf("[%dx%d] [%f,%f] fps\n", width, height, 1.0*frmival.stepwise.max.denominator/frmival.stepwise.max.numerator, 1.0*frmival.stepwise.min.denominator/frmival.stepwise.min.numerator);
        frmival.index++;    
    }
}

void printFrameSize(int fd, int fmt)
{
	// 枚举帧率
	struct v4l2_frmsizeenum frmsize;
	// frmsize.pixel_format = v4l2_fourcc('Y', 'U', 'Y', 'V');
	frmsize.pixel_format = fmt;
	frmsize.index = 0;
	unsigned int width=0, height=0;
	while(!ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize)) {
		printf("width=%d, height:%d\n", frmsize.discrete.width, frmsize.discrete.height);	
		if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) 
		{
			printFrameInterval(fd, frmsize.pixel_format, frmsize.discrete.width, frmsize.discrete.height);
		}
		else
		{
			for (width=frmsize.stepwise.min_width; width< frmsize.stepwise.max_width; width+=frmsize.stepwise.step_width)
				for (height=frmsize.stepwise.min_height; height< frmsize.stepwise.max_height; height+=frmsize.stepwise.step_height)
					printFrameInterval(fd, frmsize.pixel_format, width, height);

		}
		frmsize.index++;
	}
}
int main(int argc, char *argv[])
{
	int fd;

	if(argc<2) usage(argv[0]);
	fd = open (argv[1], O_RDWR);
	if (fd < 0)
		sysfail("open");
	struct v4l2_capability cap;
	if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
		fprintf(stderr, "Error: cam_info: can't open device: %s\n", argv[1]);
		return -1;
	}

	struct v4l2_fmtdesc vfd;
	vfd.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	vfd.index = 0;
	char format[5]= {0};

	// 枚举格式
	while(!ioctl(fd, VIDIOC_ENUM_FMT, &vfd)) {
		// enum AVCodecID codec_id = ff_fmt_v4l2codec(vfd.pixelformat);
		// enum AVPixelFormat pix_fmt = ff_fmt_v4l2ff(vfd.pixelformat, codec_id);
		// printf("index= %d, vfd.pixelformat:%d descript:%s\n", vfd.index, vfd.pixelformat, vfd.description);
		// 格式转换
		memcpy(&format[0], (void *)&vfd.pixelformat, 4);
		printf("index= %d, vfd.pixelformat:%s descript:%s\n", vfd.index, format, vfd.description);
 
		vfd.index++;
	}



	int fmt = v4l2_fourcc('Y', 'U', 'Y', 'V');
	printf("yuv:\n");
	printFrameSize(fd, fmt);
	printf("jpg:\n");
	// fmt = V4L2_PIX_FMT_JPEG;
	fmt = v4l2_fourcc('M', 'J', 'P', 'G');
	printFrameSize(fd, fmt);

	

	close(fd);
}
