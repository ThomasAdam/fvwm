/* -*-c-*- */

#ifndef _STYLE_OPT_
#define _STYLE_OPT_

/* Data surrounding a specific style entry -- such as an options passed to a
 * style command, whether it's negated, and the name of the style command
 * itself.  This is the raw data FVWM finds in its configs, from modules, etc.
 */
struct style_args
{
	const char *style_name;
	void *options;
	window_style *ps;
	int is_negated;
};

/* If a given style option is deprecated, suggest its new option, and provide
 * a deprecation message.  When looking up a style command, this will be
 * printed to stderr.
 */
struct style_opt_deprecated
{
	char new_option[1024];
	char deprecation_msg[1024][1024];
};

/* Holds actions needed to be performed when a style with "style_name" is
 * found in struct style_args.
 */
struct style_opt_struct
{
	const char *style_name;
	void (*callback)(struct style_args *);
	struct style_opt_deprecated *deprecated;
};

/* TA:  FIXME - maybe generate with a macro? */
void parse_style_opt_a(struct style_args *);
void parse_style_opt_b(struct style_args *);
void parse_style_opt_c(struct style_args *);
void parse_style_opt_d(struct style_args *);
void parse_style_opt_e(struct style_args *);
void parse_style_opt_f(struct style_args *);
void parse_style_opt_g(struct style_args *);
void parse_style_opt_h(struct style_args *);
void parse_style_opt_i(struct style_args *);
void parse_style_opt_j(struct style_args *);
void parse_style_opt_k(struct style_args *);
void parse_style_opt_l(struct style_args *);
void parse_style_opt_m(struct style_args *);
void parse_style_opt_n(struct style_args *);
void parse_style_opt_o(struct style_args *);
void parse_style_opt_p(struct style_args *);
void parse_style_opt_q(struct style_args *);
void parse_style_opt_r(struct style_args *);
void parse_style_opt_s(struct style_args *);
void parse_style_opt_t(struct style_args *);
void parse_style_opt_u(struct style_args *);
void parse_style_opt_v(struct style_args *);
void parse_style_opt_w(struct style_args *);
void parse_style_opt_x(struct style_args *);
void parse_style_opt_y(struct style_args *);
void parse_style_opt_z(struct style_args *);

struct style_opt_struct *find_specific_opt(struct style_opt_struct *,
					   const char *);
#endif
