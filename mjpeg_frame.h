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

#ifndef _MJPEG_FRAME_H
#define _MJPEG_FRAME_H

#include <stdio.h>
#include <stdint.h>

class MjpegFrame {
	public:
        MjpegFrame(size_t max_size = 524288);
		MjpegFrame(const void *data, size_t size);
		~MjpegFrame();
        void assign(const void *data, size_t size);
		void write(FILE *file);
		void write_to_fd(int fd);
        size_t size( ) { return jpegsize; }

	protected:
        void alloc(size_t max_size);
		uint8_t *jpegdata;
		size_t jpegsize;
		size_t dht_pos;
        size_t max_size_;
};

#endif
