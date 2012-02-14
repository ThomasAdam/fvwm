/* -*-c-*- */
/* This program is free software; you can redistribute it and/or modify
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

/*
 * This module was all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 */

/* code for parsing the fvwm style command */

/* ---------------------------- included header files ---------------------- */
#include "config.h"
#include <stdio.h>

#include "fvwm.h"
#include "update.h"
#include "misc.h"
#include "style.h"
#include "style-opt.h"

static void style_activeplacement(struct style_args *);
static void style_activeplacementhonorsstartsonpage(struct style_args *);
static void style_activeplacementignoresstartsonpage(struct style_args *);
static void style_allowrestack(struct style_args *);
static void style_allowmaximizefixedsize(struct style_args *);

struct style_opt_struct styles_opt_a[] = {
	{
		"ActivePlacement",
		style_activeplacement,
		NULL
	},
	{
		"ActivePlacementHonorsStartsOnPage",
		style_activeplacementhonorsstartsonpage,
		NULL
	},
	{
		"ActivePlacementIgnoresStartsOnPage",
		style_activeplacementignoresstartsonpage,
		NULL
	},
	{
		"AllowRestack",
		style_allowrestack,
		NULL
	},
	{
		"AllowMaximizeFixedSize",
		style_allowmaximizefixedsize,
		NULL
	},
	{
		NULL,
		NULL,
		NULL,
	}
};

void parse_style_opt_a(struct style_args *sargs)
{
	struct style_opt_struct *sos;

	sos = find_specific_opt(styles_opt_a, sargs->style_name);

	if (!sos) {
		fvwm_msg(INFO, "parse_style_opt_a", "Couldn't find style '%s'",
			 sargs->style_name);
		return;
	}

	sos->callback(sargs);
}

void style_activeplacement(struct style_args *sa)
{
	sa->ps->flags.placement_mode &= (~PLACE_RANDOM);
	sa->ps->flag_mask.placement_mode |= PLACE_RANDOM;
	sa->ps->change_mask.placement_mode |= PLACE_RANDOM;
}

void style_activeplacementhonorsstartsonpage(struct style_args *sa)
{
	sa->ps->flags.manual_placement_honors_starts_on_page = sa->is_negated;
	sa->ps->flag_mask.manual_placement_honors_starts_on_page = 1;
	sa->ps->change_mask.manual_placement_honors_starts_on_page = 1;
}

void style_activeplacementignoresstartsonpage(struct style_args *sa)
{
	sa->ps->flags.manual_placement_honors_starts_on_page = !sa->is_negated;
	sa->ps->flag_mask.manual_placement_honors_starts_on_page = 1;
	sa->ps->change_mask.manual_placement_honors_starts_on_page = 1;
}

void style_allowrestack(struct style_args *sa)
{
	S_SET_DO_IGNORE_RESTACK(SCF(*sa->ps), !sa->is_negated);
	S_SET_DO_IGNORE_RESTACK(SCM(*sa->ps), 1);
	S_SET_DO_IGNORE_RESTACK(SCC(*sa->ps), 1);
}

void style_allowmaximizefixedsize(struct style_args *sa)
{
	S_SET_MAXIMIZE_FIXED_SIZE_DISALLOWED(SCF(*sa->ps), !sa->is_negated);
	S_SET_MAXIMIZE_FIXED_SIZE_DISALLOWED(SCM(*sa->ps), 1);
	S_SET_MAXIMIZE_FIXED_SIZE_DISALLOWED(SCC(*sa->ps), 1);
}
