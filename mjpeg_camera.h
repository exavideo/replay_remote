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

#ifndef _MJPEG_CAMERA_H
#define _MJPEG_CAMERA_H

#include "mjpeg_frame.h"
#include <linux/videodev2.h>

class MjpegCamera {
	public:
		MjpegCamera(const char *device = "/dev/video0");
		~MjpegCamera( );

        /* get frame as a new MjpegFrame object */
		MjpegFrame *get_frame( );
        /* put frame into existing MjpegFrame object */
        void get_frame_to(MjpegFrame *target);
        /* read frame from camera, but don't put anywhere */
        void discard_frame( );

		void stream_on( );
		void stream_off( );
		
		void auto_white_balance( );
		void manual_white_balance(int temp);

		void auto_shutter_speed( );
		void shutter_speed(int shutter_speed);
		void print_shutter_speed_range( );

		void gain(int value);

		void auto_focus( );
		void focus(int focus_value);

	private:
		MjpegCamera(const MjpegCamera &cam); 
		int fd;

		void xioctl(int ioc, void *data);
		void check_mjpeg_support( );
		void dump_image_format( );
		void set_image_format( );
		void set_frame_rate( );
		void map_frame_buffers( );
		void queue_buffer(int bufno);
		int dequeue_buffer(size_t &bytesused);
		void queue_all_buffers( );
		void cleanup_frame_buffers( );
		void print_control_range(__u32 id);
		void try_set_control(__u32 id, __s32 value);
		void query_control(__u32 id, struct v4l2_queryctrl &result); 


		struct frame_buffer {
			void *start;
			size_t length;
		} *buffers;

		int n_buffers;
};

#endif
