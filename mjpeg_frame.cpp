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

#include "posix_error.h"
#include "mjpeg_frame.h"
#include "huffman.h"

MjpegFrame::MjpegFrame(size_t max_size) {
	alloc(max_size);
}

MjpegFrame::MjpegFrame(const void *data, size_t size) {
	alloc(size);
	assign(data, size);
}

MjpegFrame::~MjpegFrame( ) {
	delete [] jpegdata;
}

void MjpegFrame::alloc(size_t max_size) {
	if (jpegdata) {
		delete [] jpegdata;
	}

	jpegdata = new uint8_t[max_size];
	jpegsize = 0;
	max_size_ = max_size;
}

void MjpegFrame::assign(const void *data_, size_t size) {
    const uint8_t *data = (const uint8_t *)data_;

	jpegsize = size;
	memcpy(jpegdata, data, size);

	/* find insertion point for Huffman table */
	if (data[0] == 0xff && data[1] == 0xd8) {
		dht_pos = 2; /* we can insert the tables right after SOI marker */
	} else {
		throw std::runtime_error("corrupt JPEG received");
	}
}

void MjpegFrame::write(FILE *file) {
	fwrite(jpegdata, dht_pos, 1, file);
	fwrite(dht_data, DHT_SIZE, 1, file);
	fwrite(jpegdata + dht_pos, jpegsize - dht_pos, 1, file);
}

static void write_all(int fd, void *data, size_t size) {
	uint8_t *data_ptr = (uint8_t *) data;
	size_t remaining = size;
	ssize_t ret;

	while (remaining > 0) {
		ret = write(fd, data_ptr, remaining);
		if (ret == -1) {
			throw POSIXError("write");
		} else {
			remaining -= ret;
			data_ptr += ret;
		}
	}
}

void MjpegFrame::write_to_fd(int fd) {
	write_all(fd, jpegdata, dht_pos);
	write_all(fd, dht_data, DHT_SIZE);
	write_all(fd, jpegdata + dht_pos, jpegsize - dht_pos);
}

