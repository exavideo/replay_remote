#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <linux/uvcvideo.h>
#include <linux/usb/video.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <poll.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <sys/time.h>

#include "posix_error.h"
#include "thread.h"
#include "frame_buffer.h"
#include "mjpeg_camera.h"
#include "mjpeg_frame.h"

int read_stdin_until_newline(char *buf, size_t size) {
	size_t i;
	ssize_t ret;
	buf[size - 1] = 0;
	
	for (i = 0; i < size - 1; i++) {
		ret = read(STDIN_FILENO, buf + i, 1); 
		if (ret < 0) {
			throw POSIXError("read");
		} else if (ret == 0) {
			return 0;
		} else {
			if (buf[i] == '\n') {
				buf[i+1] = '\0';
				return 1;
			}
		}
	}

	return -1;
}

int64_t time_100us( ) {
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return ((int64_t) tv.tv_sec * 10000) + ((int64_t) tv.tv_usec / 100);
}

class ReaderThread : public Thread {
    public:
        ReaderThread(FrameBuffer *buf_, MjpegCamera *cam_) {
            buf = buf_;
            cam = cam_;
            start_thread( );
        }
    protected:
        void run_thread( ) {
            size_t frame_counter = 0;
            for (;;) {
                MjpegFrame *target_frame = buf->get_writable_frame( );
                if (target_frame != NULL) {
                    /* write frame to circular buffer if a slot is free */
                    cam->get_frame_to(target_frame);
                    buf->finish_write( );
                } else {
                    /* nowhere to put frame so drop it on the floor */
                    cam->discard_frame( );
                }
                frame_counter++;
            }
        }

        FrameBuffer *buf;
        MjpegCamera *cam;
};

void dump_buffer(FrameBuffer &buf) {
    size_t i;
    buf.mark(200);  /* hold onto the last 200 frames captured */

    for (i = 0; i < 256; i++) {
        MjpegFrame *fr = NULL;
        /* shitty busy wait loop */
        while (fr == NULL) {
            fprintf(stderr, "busy wait!\n");
            fr = buf.read_frame(i);
        }

        fr->write_to_fd(STDOUT_FILENO);
        fprintf(stderr, "%zd/256\n", i);
    }
}

int main(int argc, char **argv) {
	const char *device;
	if (argc > 1) {
		device = argv[1];
	} else {
		device = "/dev/video0";
	}

	MjpegCamera cam(device);

	char cmd_buf[128];
	char cmd[128];
	int arg, arg2, arg3;

    /* 
     * 256 frames is a bit more than 8.5 s of video, 
     * we're allowing for 512k max per frame.
     * Total buffer to be allocated is 128MB.
     */
    FrameBuffer buf(256);

	cam.stream_on( );
    ReaderThread read(&buf, &cam);

	//cam.manual_white_balance(4400);
	//cam.gain(40);
	//cam.print_shutter_speed_range( );
	//cam.shutter_speed(10); 

	for (;;) {
        if (read_stdin_until_newline(cmd_buf, sizeof(cmd_buf)) <= 0) {
            break;
        }

        sscanf(cmd_buf, "%s%d%d%d", cmd, &arg, &arg2, &arg3);
        if (strcmp(cmd, "AUTOWHITEBAL") == 0) {
            cam.auto_white_balance( );		
        } else if (strcmp(cmd, "WHITEBAL") == 0) {
            cam.manual_white_balance(arg);
        } else if (strcmp(cmd, "GAIN") == 0) {
            cam.gain(arg);
        } else if (strcmp(cmd, "SHUTTER") == 0) {
            cam.shutter_speed(arg);
        } else if (strcmp(cmd, "AUTOSHUTTER") == 0) {
            cam.auto_shutter_speed( );
        } else if (strcmp(cmd, "FOCUS") == 0) {
            cam.focus(arg);
        } else if (strcmp(cmd, "AUTOFOCUS") == 0) {
            cam.auto_focus( );
        } else if (strcmp(cmd, "DUMP") == 0) {
            dump_buffer(buf);
        } else if (strcmp(cmd, "EXIT") == 0) {
            break;
        }
    }
		
	fprintf(stderr, "Goodbye!\n");
	cam.stream_off( );
}
