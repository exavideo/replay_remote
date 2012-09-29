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

#ifndef _FRAME_BUFFER_H
#define _FRAME_BUFFER_H

#include "mutex.h"

class MjpegFrame;

class FrameBuffer {
	public:
		FrameBuffer(size_t nframes);
		~FrameBuffer( );
	
		/* 
		 * return the next frame available for writing, 
		 * or NULL if none available
		 */
		MjpegFrame *get_writable_frame( );

		/*
		 * advance write pointer after a successful write
		 * NOTE: do not call if get_writable_frame( ) returned NULL
		 * on the most recent call. Results are unpredictable.
		 */
		void finish_write( );

		/* mark frame nframes in the past as being read start point */
		void mark(size_t nframes);

		/* delete mark (allow writing to resume) */
		void unmark( );

		/* read frame (offset starts from mark) */
		MjpegFrame *read_frame(size_t frame_no);		 
	protected:
		MjpegFrame *frames_;
		size_t nframes_;

		size_t write_pointer;
		size_t markpos;
		bool mark_set;

		Mutex m;
};

#endif
