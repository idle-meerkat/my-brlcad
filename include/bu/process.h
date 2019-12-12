/*                      P R O C E S S . H
 * BRL-CAD
 *
 * Copyright (c) 2004-2019 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */

/** @ingroup process */
/** @{ */
/** @file include/bu/process.h */
/** @} */
#ifndef BU_PROCESS_H
#define BU_PROCESS_H

#include "common.h"

#include <stdio.h> /* FILE */
#include "bu/defines.h"

__BEGIN_DECLS

/**
 * returns the process ID of the calling process
 */
BU_EXPORT extern int bu_process_id(void);

/**
 * @brief terminate a given process and any children.
 *
 * returns truthfully whether the process could be killed.
 */
BU_EXPORT extern int bu_terminate(int process);


/* Wrappers for using subprocess execution */
struct bu_process;

#define BU_PROCESS_STDIN 0
#define BU_PROCESS_STDOUT 1
#define BU_PROCESS_STDERR 2


/**
 * Open and return a FILE pointer associated with the specified file
 * descriptor for input (0), output (1), or error (2) respectively.
 *
 * Input will be opened write, output and error will be opened
 * read.
 *
 * Caller should not close these FILE pointers directly.  Call
 * bu_process_close() instead.
 *
 * FIXME: misnomer, this does not open a process.  Probably doesn't
 * need to exist; just call fdopen().
 */
BU_EXPORT extern FILE *bu_process_open(struct bu_process *pinfo, int fd);


/**
 * Close any FILE pointers internally opened via bu_process_open().
 *
 * FIXME: misnomer, this does not close a process.  Probably doesn't
 * need to exist; just call fclose().
 */
BU_EXPORT extern void bu_process_close(struct bu_process *pinfo, int fd);


/**
 * Retrieve the pointer to the input (0), output (1), or error (2) file
 * descriptor associated with the process.  To use this in calling code, the
 * caller must cast the supplied pointer to the file handle type of the
 * calling code's specific platform.
 *
 * FIXME: void pointer casting is bad.  this function probably
 * shouldn't exist.
 */
BU_EXPORT void *bu_process_fd(struct bu_process *pinfo, int fd);


/**
 * Return the pid of the subprocess.
 *
 * FIXME: seemingly redundant or combinable with bu_process_id()
 * (perhaps make NULL be equivalent to the current process).
 */
BU_EXPORT int bu_process_pid(struct bu_process *pinfo);


/**
 * Returns argc and argv used to exec the subprocess - argc is
 * returned as the return integer, a pointer to the specified command
 * in the first pointer, and a pointer to the argv array in the second
 * argument. If either cmd or argv are NULL they will be skipped.
 *
 * FIXME: argument ordering and grouping is wrong/inconsistent.
 * unclear who owns the pointers returned.
 */
BU_EXPORT int bu_process_args(const char **cmd, const char * const **argv, struct bu_process *pinfo);


/**
 * Read up to n bytes into buff from a process's specified output
 * channel (fd == 1 for output, fd == 2 for err).
 *
 * FIXME: arg ordering and input/output grouping is wrong.  partially
 * redundant with bu_process_fd() and/or bu_process_open().
 */
BU_EXPORT extern int bu_process_read(char *buff, int *count, struct bu_process *pinfo, int fd, int n);


/**
 * @brief Wrapper for executing a sub-process
 *
 * FIXME: eliminate the last two options so all callers are not
 * exposed to parameters not relevant to them.
 */
BU_EXPORT extern void bu_process_exec(struct bu_process **info, const char *cmd, int argc, const char **argv, int out_eql_err, int hide_window);


/**
 * @brief wait for a sub-process to complete, release all process
 * allocations, and release the process itself.
 *
 * FIXME: 'aborted' argument may be unnecessary (could make function
 * provide return value of the process waited for).  wtime
 * undocumented.
 *
 * FIXME: this doesn't actually release all the file descriptors that
 * were opened after exec.  observed open file exhausture after a
 * couple hundred calls.
 */
 BU_EXPORT extern int bu_process_wait(int *aborted, struct bu_process *pinfo, int wtime);

/** @} */

__END_DECLS

#endif  /* BU_PROCESS_H */

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
