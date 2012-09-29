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

#include "frame_buffer.h"
#include "mjpeg_frame.h"
#include <stdexcept>

FrameBuffer::FrameBuffer(size_t nframes) {
	frames_ = new MjpegFrame[nframes];
	nframes_ = nframes;
	write_pointer = 0;
	mark_set = false;

}

FrameBuffer::~FrameBuffer( ) {
	delete [] frames_;
}

/* Writing */
MjpegFrame *FrameBuffer::get_writable_frame( ) {
	MutexLock l(m);

	if (mark_set && write_pointer == markpos) {
		return NULL;
	} else {
		return &frames_[write_pointer];
	}
}

void FrameBuffer::finish_write( ) {
	MutexLock l(m);
	write_pointer = (write_pointer + 1) % nframes_;
}

/* Reading/Marking */
MjpegFrame *FrameBuffer::read_frame(size_t frame_no) {
	MutexLock l(m);

	/* FIXME: this should really use a condition variable, not busy wait */
	size_t read_ptr = (markpos + frame_no) % nframes_;

	if (!mark_set) {
		throw std::runtime_error("tried to read frame when none marked");
	}

	/* check that read_ptr is between mark and write_ptr */
	if (
		(write_pointer <= markpos && markpos <= read_ptr)			/* ***w***m**r* */
		|| (markpos <= read_ptr && read_ptr <= write_pointer)	/* **m**r**w*** */
		|| (read_ptr <= write_pointer && write_pointer <= markpos)	/* **r**w**m*** */
	) {
		/* these are all acceptable frames to read */
		return &frames_[read_ptr];
	} else {
		/* this frame has not yet been written */
		return NULL;
	}
}

void FrameBuffer::mark(size_t nframes) {
	MutexLock l(m);

	markpos = (write_pointer + (nframes_ - nframes)) % nframes_;
	mark_set = true;
}

void FrameBuffer::unmark( ) {
	MutexLock l(m);
	mark_set = false;
}
