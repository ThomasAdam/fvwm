#ifndef _IF_HH_
#define _IF_HH_

/*
 * Copyright (C) 2006 Scott Smedley ss@aao.gov.au
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "fvwm.h"

typedef enum {
	EXECUTE_SECTION,
	IGNORE_SECTION,
	IGNORE_TIL_ENDIF
} ParseState;

/*
 * MAX_NESTED_IF_STATEMENTS is the maximum number of nested if statements
 * in a single function/file. If a function containing an if statement
 * invokes another function with an if statement, the nested-if-count
 * gets reset to zero.
 */
#define MAX_NESTED_IF_STATEMENTS 10

typedef struct
{
	unsigned char	a[MAX_NESTED_IF_STATEMENTS];
	unsigned char	n;
}Stack;

void initStack (Stack *s);

#define IfState Stack
#define initIfState(stack) (initStack(stack))


void conditionally_execute_function (
	cond_rc_t *cond_rc,
	const exec_context_t *exc,
	char *line,
	FUNC_FLAGS_TYPE exec_flags,
	char *args[],	/* function args (if any) */
	Bool has_ref_window_moved,
	Stack *stack);
#endif
