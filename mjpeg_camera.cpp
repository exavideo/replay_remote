/*
 * Copyright 2012 Exavideo LLC.
 * 
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mjpeg_camera.h"
#include "posix_error.h"

#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

/* open Video4Linux2 device and try to set up certain parameters */
MjpegCamera::MjpegCamera(const char *device) {
	buffers = NULL;
	n_buffers = 0;

	fd = open(device, O_RDWR);
	if (fd == -1) {
		throw POSIXError("open v4l device");
	}

	set_image_format( );
	set_frame_rate( );
	map_frame_buffers( );
	queue_all_buffers( );
	
}

MjpegCamera::MjpegCamera(const MjpegCamera &cam) {
	(void) cam;
	assert(false);
}

MjpegCamera::~MjpegCamera( ) {
	cleanup_frame_buffers( );
	close(fd);
}

void MjpegCamera::xioctl(int ioc, void *data) {
	if (ioctl(fd, ioc, data) == -1) {
		perror("ioctl");
		throw std::runtime_error("ioctl");
	}
}

void MjpegCamera::check_mjpeg_support( ) {
	struct v4l2_fmtdesc format_desc;
	format_desc.index = 0;
	format_desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	while (ioctl(fd, VIDIOC_ENUM_FMT, &format_desc) == 0) {
		if (format_desc.pixelformat == V4L2_PIX_FMT_MJPEG) {
			fprintf(stderr, "camera supports M-JPEG");
			return;
		}
		format_desc.index++;
	}

	throw std::runtime_error("camera does not support M-JPEG");
}

void MjpegCamera::dump_image_format( ) {
	struct v4l2_format format;

	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	xioctl(VIDIOC_G_FMT, &format);

	fprintf(stderr, "current:\n");
	fprintf(stderr, "	width: %d\n", format.fmt.pix.width);
	fprintf(stderr, "	height: %d\n", format.fmt.pix.height);
	fprintf(stderr, "	pixelformat: %d\n", format.fmt.pix.pixelformat);
	fprintf(stderr, "	field: %d\n", format.fmt.pix.field);
	fprintf(stderr, "	bytesperline: %d\n", format.fmt.pix.bytesperline);
}

void MjpegCamera::set_image_format( ) {
	struct v4l2_format format;
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	xioctl(VIDIOC_G_FMT, &format);

	format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	format.fmt.pix.width = 1920;
	format.fmt.pix.height = 1080;

	xioctl(VIDIOC_S_FMT, &format);
}

void MjpegCamera::set_frame_rate( ) {
	struct v4l2_streamparm sp;
	sp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	xioctl(VIDIOC_G_PARM, &sp);

	sp.parm.capture.timeperframe.numerator = 1;
	sp.parm.capture.timeperframe.denominator = 30;

	xioctl(VIDIOC_S_PARM, &sp);

	try_set_control(V4L2_CID_EXPOSURE_AUTO_PRIORITY, 0);
}

void MjpegCamera::map_frame_buffers( ) {
	struct v4l2_requestbuffers reqbuf;
	struct v4l2_buffer buf;
	unsigned int i;

	memset(&reqbuf, 0, sizeof(reqbuf));
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	reqbuf.count = 8;

	xioctl(VIDIOC_REQBUFS, &reqbuf);

	buffers = new frame_buffer[reqbuf.count];
	n_buffers = reqbuf.count;

	for (i = 0; i < reqbuf.count; i++) {
		memset(&buf, 0, sizeof(buf));
		buf.type = reqbuf.type;
		buf.memory = reqbuf.memory;
		buf.index = i;

		xioctl(VIDIOC_QUERYBUF, &buf);
		buffers[i].length = buf.length;
		buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
				MAP_SHARED, fd, buf.m.offset);
	
		if (MAP_FAILED == buffers[i].start) {
            throw POSIXError("mmap");
		}
	}
}

void MjpegCamera::queue_buffer(int bufno) {
	struct v4l2_buffer buf;

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = bufno;

	xioctl(VIDIOC_QBUF, &buf);
}

int MjpegCamera::dequeue_buffer(size_t &bytesused) {
	struct v4l2_buffer buf;

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	xioctl(VIDIOC_DQBUF, &buf);
	bytesused = buf.bytesused;
	return buf.index;
}

void MjpegCamera::queue_all_buffers( ) {
	int i;
	for (i = 0; i < n_buffers; i++) {
		queue_buffer(i);
	}
}

void MjpegCamera::cleanup_frame_buffers(void) {
	int i;
	for (i = 0; i < n_buffers; i++) {
		munmap(buffers[i].start, buffers[i].length);
	}

	delete [] buffers;
	buffers = NULL;
	n_buffers = 0;
}

void MjpegCamera::stream_on( ) {
	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	xioctl(VIDIOC_STREAMON, &type);
}

void MjpegCamera::stream_off( ) {
	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	xioctl(VIDIOC_STREAMOFF, &type);
}

void MjpegCamera::auto_white_balance( ) {
	try_set_control(V4L2_CID_AUTO_WHITE_BALANCE, 1);
}

void MjpegCamera::manual_white_balance(int temp) {
	try_set_control(V4L2_CID_AUTO_WHITE_BALANCE, 0);
	try_set_control(V4L2_CID_WHITE_BALANCE_TEMPERATURE, temp);
}

void MjpegCamera::auto_shutter_speed( ) {
	try_set_control(V4L2_CID_EXPOSURE_AUTO, V4L2_EXPOSURE_AUTO);
}

void MjpegCamera::shutter_speed(int shutter_speed) {
	try_set_control(V4L2_CID_EXPOSURE_AUTO, V4L2_EXPOSURE_MANUAL);
	try_set_control(V4L2_CID_EXPOSURE_ABSOLUTE, shutter_speed);
}

void MjpegCamera::print_shutter_speed_range( ) {
	print_control_range(V4L2_CID_EXPOSURE_ABSOLUTE);
}

void MjpegCamera::print_control_range(__u32 id) {
	struct v4l2_queryctrl queryctrl;
	query_control(id, queryctrl);
	fprintf(stderr, "%s: min=%d max=%d step=%d",
		queryctrl.name, (int) queryctrl.minimum,
		(int) queryctrl.maximum,
		(int) queryctrl.step
	);
}

void MjpegCamera::gain(int gain) {
	struct v4l2_queryctrl queryctrl;
	query_control(V4L2_CID_BRIGHTNESS, queryctrl);

	if (gain < 0) { gain = 0; }
	if (gain > 100) { gain = 100; }

	int new_brightness = gain * (queryctrl.maximum - queryctrl.minimum) / 100 
			+ queryctrl.minimum;

	try_set_control(V4L2_CID_BRIGHTNESS, new_brightness);
}

void MjpegCamera::focus(int focus_value) {
	struct v4l2_queryctrl queryctrl;
	query_control(V4L2_CID_FOCUS_ABSOLUTE, queryctrl);

	if (focus_value < 0) { focus_value = 0; }
	if (focus_value > 100) { focus_value = 100; }

	int new_focus = focus_value * (queryctrl.maximum - queryctrl.minimum) / 100
			+ queryctrl.minimum;

	try_set_control(V4L2_CID_FOCUS_AUTO, 0);
	try_set_control(V4L2_CID_FOCUS_ABSOLUTE, new_focus);
}

void MjpegCamera::auto_focus( ) {
	try_set_control(V4L2_CID_FOCUS_AUTO, 1);
}

void MjpegCamera::query_control(__u32 id, struct v4l2_queryctrl &result) {
	memset(&result, 0, sizeof(result));
	result.id = id;
	if (ioctl(fd, VIDIOC_QUERYCTRL, &result) == -1) {
		if (errno == EINVAL) {
			result.flags |= V4L2_CTRL_FLAG_DISABLED;
		} else {
			throw POSIXError("VIDIOC_QUERYCTRL");
		}
	}
}

void MjpegCamera::try_set_control(__u32 id, __s32 value) {
	struct v4l2_queryctrl queryctrl;
	struct v4l2_control control;

	query_control(id, queryctrl);
	if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
		return; /* control not supported */
	} else {
		memset(&control, 0, sizeof(control));
		control.id = id;
		control.value = value;
		xioctl(VIDIOC_S_CTRL, &control);
	}
}

MjpegFrame *MjpegCamera::get_frame( ) {
	size_t bytesused;
	int bufno = dequeue_buffer(bytesused);
	MjpegFrame *ret = new MjpegFrame(buffers[bufno].start, bytesused);
	queue_buffer(bufno);
	return ret;
}

void MjpegCamera::get_frame_to(MjpegFrame *target) {
    size_t bytesused;
    int bufno = dequeue_buffer(bytesused);
    target->assign(buffers[bufno].start, bytesused);
    queue_buffer(bufno);
}

void MjpegCamera::discard_frame( ) {
    size_t bytesused; /* dummy here */
    int bufno = dequeue_buffer(bytesused);
    queue_buffer(bufno);
}
