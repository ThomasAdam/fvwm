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

#include "libs/Strings.h"
#include "fvwm.h"
#include "update.h"
#include "misc.h"
#include "style.h"
#include "style-opt.h"

struct style_opt_struct *find_specific_opt(struct style_opt_struct *sos,
			const char *style_name)
{
	if (!sos)
		return NULL;

	for(; sos != NULL; ++sos)
	{
		if (StrEquals(sos->style_name, style_name))
			return sos;
	}

	return NULL;
}
