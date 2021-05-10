/*
 * v4l2loopback.c  --  video4linux2 loopback driver
 *
 * Copyright (C) 2014 Nicolas Dufresne
 */
// https://www.cnblogs.com/kevin-heyongyuan/articles/11070935.html

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

#include <stdlib.h>
#include <unistd.h>

#define COUNT 4
#define sysfail(msg) { printf ("%d %s failed: %s\n", __LINE__, (msg), strerror (errno)); return -1; }

void
usage(const char*progname)
{
  printf("usage: %s <videodevice>\n", progname);
  exit(1);
}

void write_frame(char *ext, int count, void *data, int len)
{
	int file;
	char filename[100];
	sprintf(filename, "%s%d.%s",  "frame", count, ext);
	if((file = open(filename, O_WRONLY | O_CREAT, 0660)) < 0){
		perror("open");
		exit(1);
	}
 
	write(file, data, len);
	close(file);
}

int
main (int argc, char **argv)
{
  struct v4l2_format fmt = { 0 };
  struct v4l2_requestbuffers breq = { 0 };
  void *data[COUNT] = { 0 };
  int fd;
  int i;

  if(argc<2) usage(argv[0]);

  fd = open (argv[1], O_RDWR);
  if (fd < 0)
    sysfail("open");

  struct v4l2_capability cap;
  if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
	  fprintf(stderr, "Error: cam_info: can't open device: %s\n", argv[1]);
	  return -1;
  }
  if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
	  fprintf(stderr, "The device does not handle single-planar video capture.\n");
	  exit(1);
  }



  // fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;;
  fmt.fmt.pix.width = 320;
  fmt.fmt.pix.height = 240;
  // fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB32;
  // fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;//v4l2_fourcc('Y', 'U', 'Y', 'V') /* 16  YUV 4:2:2     */;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPG;

  if (ioctl (fd, VIDIOC_S_FMT, &fmt) < 0)
    sysfail ("S_FMT");

  breq.count = COUNT;
  // breq.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
  breq.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  breq.memory = V4L2_MEMORY_MMAP;

  if (ioctl (fd, VIDIOC_REQBUFS, &breq) < 0)
    sysfail ("REQBUFS");

  assert (breq.count == COUNT);

#if 1
  struct v4l2_buffer bufs[COUNT];
  memset (bufs, 0, sizeof (bufs));

  for (i = 0; i < COUNT; i++) {
    int p;

    bufs[i].index = i;
    bufs[i].type = breq.type;
    bufs[i].memory = breq.memory;

    if (ioctl (fd, VIDIOC_QUERYBUF, &bufs[i]) < 0)
      sysfail ("QUERYBUF");

    // data[i] = mmap (NULL, bufs[i].length, PROT_WRITE, MAP_SHARED, fd, bufs[i].m.offset);
    data[i] = mmap (NULL, bufs[i].length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, bufs[i].m.offset);
    if (data[i] == MAP_FAILED)
      sysfail ("mmap");

    for (p = 0; p < (bufs[i].bytesused >> 2); p++)
      ((unsigned int*)data[i])[p] = 0xFF00FF00;
  } 

  if (ioctl (fd, VIDIOC_QBUF, &bufs[0]) < 0)
    sysfail ("QBUF");

  if ((bufs[0].flags & V4L2_BUF_FLAG_QUEUED) == 0) {
    printf ("BUG #1: Driver should set the QUEUED flag before returning from QBUF\n");
    bufs[0].flags |= V4L2_BUF_FLAG_QUEUED;
  }

  if (ioctl (fd, VIDIOC_STREAMON, &fmt.type) < 0)
    sysfail ("STREAMON");

  i = 1;
  int count = 0;

  // while (1) {
  while (1) {
    struct v4l2_buffer buf = { 0 };

    if (ioctl (fd, VIDIOC_QBUF, &bufs[i]) < 0)
      sysfail ("QBUF");


    printf ("\tQUEUED=%d\tDONE=%d\n", 
	    bufs[i].flags & V4L2_BUF_FLAG_QUEUED,
	    bufs[i].flags & V4L2_BUF_FLAG_DONE);

    if ((bufs[i].flags & V4L2_BUF_FLAG_QUEUED) == 0) {
      printf ("BUG #1: Driver should set the QUEUED flag before returning from QBUF\n");
      bufs[i].flags |= V4L2_BUF_FLAG_QUEUED;
    }

    buf.type = breq.type;
    buf.memory = breq.memory;

    if (ioctl (fd, VIDIOC_DQBUF, &buf) < 0)
      sysfail ("DBBUF");

    i = buf.index;

    if ((bufs[i].flags & V4L2_BUF_FLAG_QUEUED) == 0) {
      printf ("BUG #2: Driver should not dequeue a buffer that was not intially queued\n");
    }

#if 0
    assert (bufs[i].flags & V4L2_BUF_FLAG_QUEUED);
    assert (!(buf.flags & (V4L2_BUF_FLAG_QUEUED | V4L2_BUF_FLAG_DONE)));
#endif
    bufs[i] = buf;

	// write_frame("yuv", count, data[i], buf.length);
	write_frame("jpg", count, data[i], buf.length);
	// 停止
	if(count++ > 2)break;
  }
#else
	

#endif

  return 0;
}
