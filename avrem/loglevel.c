/*
  XTulator: A portable, open-source 80186 PC emulator.
  Copyright (C)2020 Mike Chambers

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "config.h"
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include "loglevel.h"

int log_level = LOG_ERROR;

void printl(uint8_t level, char* format, ...) {
	va_list argptr;
	va_start(argptr, format);
	if (level > log_level) {
		va_end(argptr);
		return;
	}
	vfprintf(stderr, format, argptr);
	fflush(stderr);
	va_end(argptr);
}

void log_setLevel(uint8_t level) {
	if (level > LOG_DETAIL2) {
		return;
	}
	log_level = level;
}

void log_init() {
	//TODO: Maybe allow initializing this with file output rather than always using stderr. Or maybe remove this, I don't know...
}
