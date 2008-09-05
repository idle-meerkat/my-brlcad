/*                         F I N D . H
 * BRL-CAD
 *
 * Copyright (c) 2008 United States Government as represented by
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
 *
 * Includes code from OpenBSD's find command:
 *
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Cimarron D. Taylor of the University of California, Berkeley.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */


#include <sys/cdefs.h>

/* node type */
enum ntype {
	N_AND = 1, 				/* must start > 0 */
	N_CLOSEPAREN, N_DEPTH, N_EMPTY, N_EXEC, N_EXECDIR, N_EXPR,
	N_FLAGS, N_INAME, N_LS, N_MAXDEPTH,
	N_MINDEPTH, N_NAME, N_NOT, N_OK, N_OPENPAREN, N_OR, N_PATH, 
	N_PRINT, N_PRINT0, N_PRUNE, N_TYPE
};


/* node definition */
typedef struct _plandata {
	struct _plandata *next;			/* next node */
	int (*eval)(struct _plandata *, struct db_full_path *);
									/* node evaluation function */
#define	F_EQUAL		1			/* [acm]time inum links size */
#define	F_LESSTHAN	2
#define	F_GREATER	3
#define	F_NEEDOK	1			/* exec ok */
#define	F_MTFLAG	1			/* fstype */
#define	F_MTTYPE	2
#define	F_ATLEAST	1			/* perm */
	int flags;				/* private flags */
	enum ntype type;			/* plan node type */
	union {
		gid_t _g_data;			/* gid */
		struct {
			u_int _f_flags;
			u_int _f_mask;
		} fl;
		struct _plandata *_p_data[2];	/* PLAN trees */
		struct _ex {
			char **_e_argv;		/* argv array */
			char **_e_orig;		/* original strings */
			int *_e_len;		/* allocated length */
		} ex;
		char *_a_data[2];		/* array of char pointers */
		char *_c_data;			/* char pointer */
		int _max_data;			/* tree depth */
		int _min_data;			/* tree depth */
	} p_un;
} PLAN;
#define	a_data		p_un._a_data
#define	c_data		p_un._c_data
#define fl_flags	p_un.fl._f_flags
#define fl_mask		p_un.fl._f_mask
#define	g_data		p_un._g_data
#define	max_data	p_un._max_data
#define	min_data	p_un._min_data
#define	p_data		p_un._p_data
#define	e_argv		p_un.ex._e_argv
#define	e_orig		p_un.ex._e_orig
#define	e_len		p_un.ex._e_len

typedef struct _option {
	char *name;				/* option name */
	enum ntype token;			/* token type */
	int (*create)(char *, char ***, int, PLAN **);	/* create function */
#define	O_NONE		0x01			/* no call required */
#define	O_ZERO		0x02			/* pass: nothing */
#define	O_ARGV		0x04			/* pass: argv, increment argv */
#define	O_ARGVP		0x08		/* pass: *argv, N_OK || N_EXEC || N_EXECDIR */
	int flags;
} OPTION;


void	 brace_subst(char *, char **, char *, int);
int	find_create(char ***, PLAN **);
void	 find_execute(PLAN *, struct db_full_path *, struct rt_wdb *);
int	find_formplan(char **, PLAN **);
int	not_squish(PLAN *, PLAN **);
OPTION	*option(char *);
int	or_squish(PLAN *, PLAN **);
int	paren_squish(PLAN *, PLAN **);
struct stat;
void	 printlong(char *, char *, struct stat *);
int	     queryuser(char **);
void	 show_path(int);

PLAN	*c_depth(char *, char ***, int);
PLAN	*c_empty(char *, char ***, int);
PLAN	*c_exec(char *, char ***, int);
PLAN	*c_execdir(char *, char ***, int);
PLAN	*c_flags(char *, char ***, int);
PLAN	*c_iname(char *, char ***, int);
PLAN	*c_ls(char *, char ***, int);
PLAN	*c_maxdepth(char *, char ***, int);
PLAN	*c_mindepth(char *, char ***, int);
int	c_name(char *, char ***, int, PLAN **);
PLAN	*c_path(char *, char ***, int);
int	c_print(char *, char ***, int, PLAN **);
int	c_print0(char *, char ***, int, PLAN **);
PLAN	*c_prune(char *, char ***, int);
PLAN	*c_type(char *, char ***, int);
int	c_openparen(char *, char ***, int, PLAN **);
int	c_closeparen(char *, char ***, int, PLAN **);
int	c_not(char *, char ***, int, PLAN **);
int	c_or(char *, char ***, int, PLAN **);

extern int isdepth, isoutput;
