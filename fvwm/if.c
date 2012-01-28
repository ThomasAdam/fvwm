
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
#include "libs/Parse.h"
#include "fvwm.h"
#include "misc.h"	/* fvwm_msg() */
#include "functions.h"	/* execute_function_with_args() */
#include "if.h"

#include <stdio.h>
#include <strings.h>	/* strcasecmp() */
#include <stdlib.h>	/* system() */


void initStack (Stack *s)
{
	s->n = 0;
}

static void pushStack (Stack *s, ParseState state)
{
	if (s->n >= MAX_NESTED_IF_STATEMENTS)
		fvwm_msg(ERR, "pushStack", "Too many nested if-statements.");

	s->a[s->n++] = state;
}

static void popStack (Stack *s)
{
	if (s->n <= 0)
		fvwm_msg(ERR, "popStack", "Error trying to pop empty stack.");
	if (s->n > MAX_NESTED_IF_STATEMENTS)
		fvwm_msg(ERR, "popStack", "Logic error!");
	s->n--;
}

static Bool stackEmpty (Stack *s)
{
	return (s->n == 0);
}

static ParseState peekStack (Stack *s)
{
	return (s->a[s->n - 1]);
}

static void editTopOfStack (Stack *s, ParseState state)
{
	if (s->n <= 0)
		fvwm_msg(ERR, "editTopOfStack", "Logic error");

	s->a[s->n - 1] = state;
}

/*
 * We use a stack to remember the state, however, we try to keep that
 * implementation detail transparent outside of this module. Hence the
 * use of the #defines in the "if.h" header file.
 */
#define currentState() (peekStack(stack))
#define setCurrentState(state) (editTopOfStack(stack, state))
#define inIf() (!stackEmpty(stack))


static Bool conditionTrue (
	cond_rc_t *cond_rc,
	const exec_context_t *exc,
	char *cond,
	char *args[],
	Bool has_ref_window_moved)
{
	Bool bNegate = False;

	/* Handle preceeding '!'s to negate return code. */
	while (cond[0] == '!')
	{
		bNegate = !bNegate;
		cond++;
		if (cond[0] == '\0')
		{
			fvwm_msg(ERR, "conditionTrue", "No condition specified");
			return False;
		}
	}

	execute_function_with_args(cond_rc, exc, cond, FUNC_DONT_DEFER, args, has_ref_window_moved);

	return (((cond_rc->rc == COND_RC_OK) == !bNegate) ? True : False);
}


/*
 * preconditions:
 *	cond_rc != NULL
 */
void conditionally_execute_function (
	cond_rc_t *cond_rc,
	const exec_context_t *exc,
	char *line,
	FUNC_FLAGS_TYPE exec_flags,
	char *args[],	/* function args (if any) */
	Bool has_ref_window_moved,
	Stack *stack)
{
	char *tmp = line, *cond, *fn = "conditionally_execute_function";

	if (line == NULL || line[0] == '\0' || line[0] == '#')
		return;

	tmp = PeekToken(tmp, &cond);

	if (strcasecmp(tmp, "if") == 0)
	{
		if (inIf() && currentState() != EXECUTE_SECTION)
			pushStack(stack, IGNORE_TIL_ENDIF);
		else
		{
			if (cond == NULL || cond[0] == '\0')
			{
				fvwm_msg(ERR, fn, "No condition specified in \"if\"");
				return;
			}

			pushStack(stack,
				conditionTrue(cond_rc, exc, cond, args, has_ref_window_moved) == True ?
				EXECUTE_SECTION : IGNORE_SECTION);
		}
	}
	else if (strcasecmp(tmp, "elsif") == 0)
	{
		if (!inIf())
			fvwm_msg(ERR, fn, "\"elsif\" not within if-statement");
		else if (currentState() == EXECUTE_SECTION)
			setCurrentState(IGNORE_TIL_ENDIF);
		else if (currentState() == IGNORE_SECTION)
		{
			if (cond == NULL || cond[0] == '\0')
			{
				fvwm_msg(ERR, fn, "No condition specified in \"elsif\"");
				return;
			}
			if (conditionTrue(cond_rc, exc, cond, args, has_ref_window_moved) == True)
				setCurrentState(EXECUTE_SECTION);
		}
	}
	else if (strcasecmp(tmp, "else") == 0)
	{
		if (!inIf())
			fvwm_msg(ERR, fn, "\"else\" not within if-statement");
		else if (currentState() == EXECUTE_SECTION)
			setCurrentState(IGNORE_TIL_ENDIF);
		else if (currentState() == IGNORE_SECTION)
			setCurrentState(EXECUTE_SECTION);
	}
	else if (strcasecmp(tmp, "endif") == 0)
	{
		if (!inIf())
			fvwm_msg(ERR, fn, "\"endif\" not within if-statement");
		else
			popStack(stack);
	}
	else
	{
		if (!inIf() || currentState() == EXECUTE_SECTION)
			execute_function_with_args(cond_rc, exc, line,
				exec_flags, args, has_ref_window_moved);
		// else: ignore it.
	}
}
