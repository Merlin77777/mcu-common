/*
 * This file is part of MCU-Common.
 *
 * Copyright (C) 2017 Adam Heinrich <adam@adamh.cz>
 *
 * MCU-Common is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MCU-Common is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MCU-Common.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, the copyright holders of this library give
 * you permission to link this library with independent modules to
 * produce an executable, regardless of the license terms of these
 * independent modules, and to copy and distribute the resulting
 * executable under terms of your choice, provided that you also meet,
 * for each linked independent module, the terms and conditions of the
 * license of that module.  An independent module is a module which is
 * not derived from or based on this library.  If you modify this
 * library, you may extend this exception to your version of the
 * library, but you are not obligated to do so.  If you do not wish to
 * do so, delete this exception statement from your version.
 */

#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <mcu-common/logger.h>

static ssize_t logger_write(void *cookie, const char *buf, size_t size);
static int flprintf(FILE *fp, const struct logger_entry *e);

bool logger_init(struct logger *log, bool buffered)
{
	assert(log != NULL);
	assert(log->write_cb != NULL);

	log->initialized = false;
	cookie_io_functions_t logger_io_fns = {
		.read  = NULL,
		.write = &logger_write,
		.seek  = NULL,
		.close = NULL,
	};

	if (!fifo_init(log->fifo))
		return true;

	log->fp = fopencookie(log, "w", logger_io_fns);
	if (log->fp == NULL)
		return false;

	if (buffered)
		setlinebuf(log->fp);
	else
		setbuf(log->fp, NULL);

	log->initialized = true;

	return true;
}

bool logger_put(const struct logger *log, int argc, const char *fmt, ...)
{
	assert(log != NULL);
	assert(fmt != NULL);
	assert(argc >= 0 && argc <= LOGGER_MAX_ARGC);

	if (!log->initialized)
		return false;

	struct logger_entry entry;
	va_list args;

	entry.fmt = fmt;
	entry.argc = (argc > LOGGER_MAX_ARGC) ? LOGGER_MAX_ARGC : argc;

	va_start(args, fmt);
	for (int i = 0; i < entry.argc; i++)
		entry.argv[i] = va_arg(args, unsigned int);
	va_end(args);

	return (fifo_write(log->fifo, &entry, 1) == 1);
}

bool logger_process(const struct logger *log)
{
	assert(log != NULL);

	if (!log->initialized)
		return false;

	struct logger_entry entry;
	if (fifo_read(log->fifo, &entry, 1) == 1) {
		flprintf(log->fp, &entry);
		return true;
	}

	return false;
}

static ssize_t logger_write(void *cookie, const char *buf, size_t size)
{
	assert(cookie != NULL);
	assert(buf != NULL);
	assert(((struct logger *)cookie)->write_cb != NULL);

	return ((struct logger *)cookie)->write_cb(buf, size);
}

static int flprintf(FILE *fp, const struct logger_entry *e)
{
	assert(fp != NULL);
	assert(e != NULL);

	const char *fm = e->fmt;
	const unsigned int *a = e->argv;

	switch (e->argc) {
	case 0:
		return fprintf(fp, fm);
	case 1:
		return fprintf(fp, fm, a[0]);
	case 2:
		return fprintf(fp, fm, a[0], a[1]);
	case 3:
		return fprintf(fp, fm, a[0], a[1], a[2]);
	case 4:
		return fprintf(fp, fm, a[0], a[1], a[2], a[3]);
	case 5:
		return fprintf(fp, fm, a[0], a[1], a[2], a[3], a[4]);
	case 6:
		return fprintf(fp, fm, a[0], a[1], a[2], a[3], a[4], a[5]);
#if (LOGGER_MAX_ARGC > 6)
	#error "Please define more cases for the given value of LOGGER_MAX_ARGC"
#endif
	default:
		return -1;
	}
}