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

#ifndef _POSIX_ERROR_H
#define _POSIX_ERROR_H

#include <stdexcept>
#include <errno.h>
#include <string.h>

class POSIXError : public std::runtime_error {
	public:
		POSIXError(const std::string& arg) 
			: std::runtime_error(arg + ": " + strerror(errno)) {}
	protected:
		int my_errno;
};

#endif
