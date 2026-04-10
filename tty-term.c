/* $OpenBSD$ */

/*
 * Copyright (c) 2008 Nicholas Marriott <nicholas.marriott@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include <fnmatch.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "tmux.h"

static char	*tty_term_strip(const char *);

struct tty_terms tty_terms = LIST_HEAD_INITIALIZER(tty_terms);

enum tty_code_type {
	TTYCODE_NONE = 0,
	TTYCODE_STRING,
	TTYCODE_NUMBER,
	TTYCODE_FLAG,
};

struct tty_code {
	enum tty_code_type	type;
	union {
		char	       *string;
		int		number;
		int		flag;
	} value;
};

struct tty_term_code_entry {
	enum tty_code_type	type;
	const char	       *name;
};

static const struct tty_term_code_entry tty_term_codes[] = {
	[TTYC_ACSC] = { TTYCODE_STRING, "acsc" },
	[TTYC_AM] = { TTYCODE_FLAG, "am" },
	[TTYC_AX] = { TTYCODE_FLAG, "AX" },
	[TTYC_BCE] = { TTYCODE_FLAG, "bce" },
	[TTYC_BEL] = { TTYCODE_STRING, "bel" },
	[TTYC_BIDI] = { TTYCODE_STRING, "Bidi" },
	[TTYC_BLINK] = { TTYCODE_STRING, "blink" },
	[TTYC_BOLD] = { TTYCODE_STRING, "bold" },
	[TTYC_CIVIS] = { TTYCODE_STRING, "civis" },
	[TTYC_CLEAR] = { TTYCODE_STRING, "clear" },
	[TTYC_CLMG] = { TTYCODE_STRING, "Clmg" },
	[TTYC_CMG] = { TTYCODE_STRING, "Cmg" },
	[TTYC_CNORM] = { TTYCODE_STRING, "cnorm" },
	[TTYC_COLORS] = { TTYCODE_NUMBER, "colors" },
	[TTYC_CR] = { TTYCODE_STRING, "Cr" },
	[TTYC_CSR] = { TTYCODE_STRING, "csr" },
	[TTYC_CS] = { TTYCODE_STRING, "Cs" },
	[TTYC_CUB1] = { TTYCODE_STRING, "cub1" },
	[TTYC_CUB] = { TTYCODE_STRING, "cub" },
	[TTYC_CUD1] = { TTYCODE_STRING, "cud1" },
	[TTYC_CUD] = { TTYCODE_STRING, "cud" },
	[TTYC_CUF1] = { TTYCODE_STRING, "cuf1" },
	[TTYC_CUF] = { TTYCODE_STRING, "cuf" },
	[TTYC_CUP] = { TTYCODE_STRING, "cup" },
	[TTYC_CUU1] = { TTYCODE_STRING, "cuu1" },
	[TTYC_CUU] = { TTYCODE_STRING, "cuu" },
	[TTYC_CVVIS] = { TTYCODE_STRING, "cvvis" },
	[TTYC_DCH1] = { TTYCODE_STRING, "dch1" },
	[TTYC_DCH] = { TTYCODE_STRING, "dch" },
	[TTYC_DIM] = { TTYCODE_STRING, "dim" },
	[TTYC_DL1] = { TTYCODE_STRING, "dl1" },
	[TTYC_DL] = { TTYCODE_STRING, "dl" },
	[TTYC_DSEKS] = { TTYCODE_STRING, "Dseks" },
	[TTYC_DSFCS] = { TTYCODE_STRING, "Dsfcs" },
	[TTYC_DSBP] = { TTYCODE_STRING, "Dsbp" },
	[TTYC_DSMG] = { TTYCODE_STRING, "Dsmg" },
	[TTYC_E3] = { TTYCODE_STRING, "E3" },
	[TTYC_ECH] = { TTYCODE_STRING, "ech" },
	[TTYC_ED] = { TTYCODE_STRING, "ed" },
	[TTYC_EL1] = { TTYCODE_STRING, "el1" },
	[TTYC_EL] = { TTYCODE_STRING, "el" },
	[TTYC_ENACS] = { TTYCODE_STRING, "enacs" },
	[TTYC_ENBP] = { TTYCODE_STRING, "Enbp" },
	[TTYC_ENEKS] = { TTYCODE_STRING, "Eneks" },
	[TTYC_ENFCS] = { TTYCODE_STRING, "Enfcs" },
	[TTYC_ENMG] = { TTYCODE_STRING, "Enmg" },
	[TTYC_FSL] = { TTYCODE_STRING, "fsl" },
	[TTYC_HLS] = { TTYCODE_STRING, "Hls" },
	[TTYC_HOME] = { TTYCODE_STRING, "home" },
	[TTYC_HPA] = { TTYCODE_STRING, "hpa" },
	[TTYC_ICH1] = { TTYCODE_STRING, "ich1" },
	[TTYC_ICH] = { TTYCODE_STRING, "ich" },
	[TTYC_IL1] = { TTYCODE_STRING, "il1" },
	[TTYC_IL] = { TTYCODE_STRING, "il" },
	[TTYC_INDN] = { TTYCODE_STRING, "indn" },
	[TTYC_INVIS] = { TTYCODE_STRING, "invis" },
	[TTYC_KCBT] = { TTYCODE_STRING, "kcbt" },
	[TTYC_KCUB1] = { TTYCODE_STRING, "kcub1" },
	[TTYC_KCUD1] = { TTYCODE_STRING, "kcud1" },
	[TTYC_KCUF1] = { TTYCODE_STRING, "kcuf1" },
	[TTYC_KCUU1] = { TTYCODE_STRING, "kcuu1" },
	[TTYC_KDC2] = { TTYCODE_STRING, "kDC" },
	[TTYC_KDC3] = { TTYCODE_STRING, "kDC3" },
	[TTYC_KDC4] = { TTYCODE_STRING, "kDC4" },
	[TTYC_KDC5] = { TTYCODE_STRING, "kDC5" },
	[TTYC_KDC6] = { TTYCODE_STRING, "kDC6" },
	[TTYC_KDC7] = { TTYCODE_STRING, "kDC7" },
	[TTYC_KDCH1] = { TTYCODE_STRING, "kdch1" },
	[TTYC_KDN2] = { TTYCODE_STRING, "kDN" }, /* not kDN2 */
	[TTYC_KDN3] = { TTYCODE_STRING, "kDN3" },
	[TTYC_KDN4] = { TTYCODE_STRING, "kDN4" },
	[TTYC_KDN5] = { TTYCODE_STRING, "kDN5" },
	[TTYC_KDN6] = { TTYCODE_STRING, "kDN6" },
	[TTYC_KDN7] = { TTYCODE_STRING, "kDN7" },
	[TTYC_KEND2] = { TTYCODE_STRING, "kEND" },
	[TTYC_KEND3] = { TTYCODE_STRING, "kEND3" },
	[TTYC_KEND4] = { TTYCODE_STRING, "kEND4" },
	[TTYC_KEND5] = { TTYCODE_STRING, "kEND5" },
	[TTYC_KEND6] = { TTYCODE_STRING, "kEND6" },
	[TTYC_KEND7] = { TTYCODE_STRING, "kEND7" },
	[TTYC_KEND] = { TTYCODE_STRING, "kend" },
	[TTYC_KF10] = { TTYCODE_STRING, "kf10" },
	[TTYC_KF11] = { TTYCODE_STRING, "kf11" },
	[TTYC_KF12] = { TTYCODE_STRING, "kf12" },
	[TTYC_KF13] = { TTYCODE_STRING, "kf13" },
	[TTYC_KF14] = { TTYCODE_STRING, "kf14" },
	[TTYC_KF15] = { TTYCODE_STRING, "kf15" },
	[TTYC_KF16] = { TTYCODE_STRING, "kf16" },
	[TTYC_KF17] = { TTYCODE_STRING, "kf17" },
	[TTYC_KF18] = { TTYCODE_STRING, "kf18" },
	[TTYC_KF19] = { TTYCODE_STRING, "kf19" },
	[TTYC_KF1] = { TTYCODE_STRING, "kf1" },
	[TTYC_KF20] = { TTYCODE_STRING, "kf20" },
	[TTYC_KF21] = { TTYCODE_STRING, "kf21" },
	[TTYC_KF22] = { TTYCODE_STRING, "kf22" },
	[TTYC_KF23] = { TTYCODE_STRING, "kf23" },
	[TTYC_KF24] = { TTYCODE_STRING, "kf24" },
	[TTYC_KF25] = { TTYCODE_STRING, "kf25" },
	[TTYC_KF26] = { TTYCODE_STRING, "kf26" },
	[TTYC_KF27] = { TTYCODE_STRING, "kf27" },
	[TTYC_KF28] = { TTYCODE_STRING, "kf28" },
	[TTYC_KF29] = { TTYCODE_STRING, "kf29" },
	[TTYC_KF2] = { TTYCODE_STRING, "kf2" },
	[TTYC_KF30] = { TTYCODE_STRING, "kf30" },
	[TTYC_KF31] = { TTYCODE_STRING, "kf31" },
	[TTYC_KF32] = { TTYCODE_STRING, "kf32" },
	[TTYC_KF33] = { TTYCODE_STRING, "kf33" },
	[TTYC_KF34] = { TTYCODE_STRING, "kf34" },
	[TTYC_KF35] = { TTYCODE_STRING, "kf35" },
	[TTYC_KF36] = { TTYCODE_STRING, "kf36" },
	[TTYC_KF37] = { TTYCODE_STRING, "kf37" },
	[TTYC_KF38] = { TTYCODE_STRING, "kf38" },
	[TTYC_KF39] = { TTYCODE_STRING, "kf39" },
	[TTYC_KF3] = { TTYCODE_STRING, "kf3" },
	[TTYC_KF40] = { TTYCODE_STRING, "kf40" },
	[TTYC_KF41] = { TTYCODE_STRING, "kf41" },
	[TTYC_KF42] = { TTYCODE_STRING, "kf42" },
	[TTYC_KF43] = { TTYCODE_STRING, "kf43" },
	[TTYC_KF44] = { TTYCODE_STRING, "kf44" },
	[TTYC_KF45] = { TTYCODE_STRING, "kf45" },
	[TTYC_KF46] = { TTYCODE_STRING, "kf46" },
	[TTYC_KF47] = { TTYCODE_STRING, "kf47" },
	[TTYC_KF48] = { TTYCODE_STRING, "kf48" },
	[TTYC_KF49] = { TTYCODE_STRING, "kf49" },
	[TTYC_KF4] = { TTYCODE_STRING, "kf4" },
	[TTYC_KF50] = { TTYCODE_STRING, "kf50" },
	[TTYC_KF51] = { TTYCODE_STRING, "kf51" },
	[TTYC_KF52] = { TTYCODE_STRING, "kf52" },
	[TTYC_KF53] = { TTYCODE_STRING, "kf53" },
	[TTYC_KF54] = { TTYCODE_STRING, "kf54" },
	[TTYC_KF55] = { TTYCODE_STRING, "kf55" },
	[TTYC_KF56] = { TTYCODE_STRING, "kf56" },
	[TTYC_KF57] = { TTYCODE_STRING, "kf57" },
	[TTYC_KF58] = { TTYCODE_STRING, "kf58" },
	[TTYC_KF59] = { TTYCODE_STRING, "kf59" },
	[TTYC_KF5] = { TTYCODE_STRING, "kf5" },
	[TTYC_KF60] = { TTYCODE_STRING, "kf60" },
	[TTYC_KF61] = { TTYCODE_STRING, "kf61" },
	[TTYC_KF62] = { TTYCODE_STRING, "kf62" },
	[TTYC_KF63] = { TTYCODE_STRING, "kf63" },
	[TTYC_KF6] = { TTYCODE_STRING, "kf6" },
	[TTYC_KF7] = { TTYCODE_STRING, "kf7" },
	[TTYC_KF8] = { TTYCODE_STRING, "kf8" },
	[TTYC_KF9] = { TTYCODE_STRING, "kf9" },
	[TTYC_KHOM2] = { TTYCODE_STRING, "kHOM" },
	[TTYC_KHOM3] = { TTYCODE_STRING, "kHOM3" },
	[TTYC_KHOM4] = { TTYCODE_STRING, "kHOM4" },
	[TTYC_KHOM5] = { TTYCODE_STRING, "kHOM5" },
	[TTYC_KHOM6] = { TTYCODE_STRING, "kHOM6" },
	[TTYC_KHOM7] = { TTYCODE_STRING, "kHOM7" },
	[TTYC_KHOME] = { TTYCODE_STRING, "khome" },
	[TTYC_KIC2] = { TTYCODE_STRING, "kIC" },
	[TTYC_KIC3] = { TTYCODE_STRING, "kIC3" },
	[TTYC_KIC4] = { TTYCODE_STRING, "kIC4" },
	[TTYC_KIC5] = { TTYCODE_STRING, "kIC5" },
	[TTYC_KIC6] = { TTYCODE_STRING, "kIC6" },
	[TTYC_KIC7] = { TTYCODE_STRING, "kIC7" },
	[TTYC_KICH1] = { TTYCODE_STRING, "kich1" },
	[TTYC_KIND] = { TTYCODE_STRING, "kind" },
	[TTYC_KLFT2] = { TTYCODE_STRING, "kLFT" },
	[TTYC_KLFT3] = { TTYCODE_STRING, "kLFT3" },
	[TTYC_KLFT4] = { TTYCODE_STRING, "kLFT4" },
	[TTYC_KLFT5] = { TTYCODE_STRING, "kLFT5" },
	[TTYC_KLFT6] = { TTYCODE_STRING, "kLFT6" },
	[TTYC_KLFT7] = { TTYCODE_STRING, "kLFT7" },
	[TTYC_KMOUS] = { TTYCODE_STRING, "kmous" },
	[TTYC_KNP] = { TTYCODE_STRING, "knp" },
	[TTYC_KNXT2] = { TTYCODE_STRING, "kNXT" },
	[TTYC_KNXT3] = { TTYCODE_STRING, "kNXT3" },
	[TTYC_KNXT4] = { TTYCODE_STRING, "kNXT4" },
	[TTYC_KNXT5] = { TTYCODE_STRING, "kNXT5" },
	[TTYC_KNXT6] = { TTYCODE_STRING, "kNXT6" },
	[TTYC_KNXT7] = { TTYCODE_STRING, "kNXT7" },
	[TTYC_KPP] = { TTYCODE_STRING, "kpp" },
	[TTYC_KPRV2] = { TTYCODE_STRING, "kPRV" },
	[TTYC_KPRV3] = { TTYCODE_STRING, "kPRV3" },
	[TTYC_KPRV4] = { TTYCODE_STRING, "kPRV4" },
	[TTYC_KPRV5] = { TTYCODE_STRING, "kPRV5" },
	[TTYC_KPRV6] = { TTYCODE_STRING, "kPRV6" },
	[TTYC_KPRV7] = { TTYCODE_STRING, "kPRV7" },
	[TTYC_KRIT2] = { TTYCODE_STRING, "kRIT" },
	[TTYC_KRIT3] = { TTYCODE_STRING, "kRIT3" },
	[TTYC_KRIT4] = { TTYCODE_STRING, "kRIT4" },
	[TTYC_KRIT5] = { TTYCODE_STRING, "kRIT5" },
	[TTYC_KRIT6] = { TTYCODE_STRING, "kRIT6" },
	[TTYC_KRIT7] = { TTYCODE_STRING, "kRIT7" },
	[TTYC_KRI] = { TTYCODE_STRING, "kri" },
	[TTYC_KUP2] = { TTYCODE_STRING, "kUP" }, /* not kUP2 */
	[TTYC_KUP3] = { TTYCODE_STRING, "kUP3" },
	[TTYC_KUP4] = { TTYCODE_STRING, "kUP4" },
	[TTYC_KUP5] = { TTYCODE_STRING, "kUP5" },
	[TTYC_KUP6] = { TTYCODE_STRING, "kUP6" },
	[TTYC_KUP7] = { TTYCODE_STRING, "kUP7" },
	[TTYC_MS] = { TTYCODE_STRING, "Ms" },
	[TTYC_NOBR] = { TTYCODE_STRING, "Nobr" },
	[TTYC_OL] = { TTYCODE_STRING, "ol" },
	[TTYC_OP] = { TTYCODE_STRING, "op" },
	[TTYC_RECT] = { TTYCODE_STRING, "Rect" },
	[TTYC_REV] = { TTYCODE_STRING, "rev" },
	[TTYC_RGB] = { TTYCODE_FLAG, "RGB" },
	[TTYC_RIN] = { TTYCODE_STRING, "rin" },
	[TTYC_RI] = { TTYCODE_STRING, "ri" },
	[TTYC_RMACS] = { TTYCODE_STRING, "rmacs" },
	[TTYC_RMCUP] = { TTYCODE_STRING, "rmcup" },
	[TTYC_RMKX] = { TTYCODE_STRING, "rmkx" },
	[TTYC_SETAB] = { TTYCODE_STRING, "setab" },
	[TTYC_SETAF] = { TTYCODE_STRING, "setaf" },
	[TTYC_SETAL] = { TTYCODE_STRING, "setal" },
	[TTYC_SETRGBB] = { TTYCODE_STRING, "setrgbb" },
	[TTYC_SETRGBF] = { TTYCODE_STRING, "setrgbf" },
	[TTYC_SETULC] = { TTYCODE_STRING, "Setulc" },
	[TTYC_SETULC1] = { TTYCODE_STRING, "Setulc1" },
	[TTYC_SE] = { TTYCODE_STRING, "Se" },
	[TTYC_SXL] =  { TTYCODE_FLAG, "Sxl" },
	[TTYC_SGR0] = { TTYCODE_STRING, "sgr0" },
	[TTYC_SITM] = { TTYCODE_STRING, "sitm" },
	[TTYC_SMACS] = { TTYCODE_STRING, "smacs" },
	[TTYC_SMCUP] = { TTYCODE_STRING, "smcup" },
	[TTYC_SMKX] = { TTYCODE_STRING, "smkx" },
	[TTYC_SMOL] = { TTYCODE_STRING, "Smol" },
	[TTYC_SMSO] = { TTYCODE_STRING, "smso" },
	[TTYC_SMULX] = { TTYCODE_STRING, "Smulx" },
	[TTYC_SMUL] = { TTYCODE_STRING, "smul" },
	[TTYC_SMXX] =  { TTYCODE_STRING, "smxx" },
	[TTYC_SS] = { TTYCODE_STRING, "Ss" },
	[TTYC_SWD] = { TTYCODE_STRING, "Swd" },
	[TTYC_SYNC] = { TTYCODE_STRING, "Sync" },
	[TTYC_TC] = { TTYCODE_FLAG, "Tc" },
	[TTYC_TSL] = { TTYCODE_STRING, "tsl" },
	[TTYC_U8] = { TTYCODE_NUMBER, "U8" },
	[TTYC_VPA] = { TTYCODE_STRING, "vpa" },
	[TTYC_XT] = { TTYCODE_FLAG, "XT" }
};

u_int
tty_term_ncodes(void)
{
	return (nitems(tty_term_codes));
}

static char *
tty_term_strip(const char *s)
{
	const char     *ptr;
	static char	buf[8192];
	size_t		len;

	/* Ignore strings with no padding. */
	if (strchr(s, '$') == NULL)
		return (xstrdup(s));

	len = 0;
	for (ptr = s; *ptr != '\0'; ptr++) {
		if (*ptr == '$' && *(ptr + 1) == '<') {
			while (*ptr != '\0' && *ptr != '>')
				ptr++;
			if (*ptr == '>')
				ptr++;
			if (*ptr == '\0')
				break;
		}

		buf[len++] = *ptr;
		if (len == (sizeof buf) - 1)
			break;
	}
	buf[len] = '\0';

	return (xstrdup(buf));
}

static char *
tty_term_override_next(const char *s, size_t *offset)
{
	static char	value[8192];
	size_t		n = 0, at = *offset;

	if (s[at] == '\0')
		return (NULL);

	while (s[at] != '\0') {
		if (s[at] == ':') {
			if (s[at + 1] == ':') {
				value[n++] = ':';
				at += 2;
			} else
				break;
		} else {
			value[n++] = s[at];
			at++;
		}
		if (n == (sizeof value) - 1)
			return (NULL);
	}
	if (s[at] != '\0')
		*offset = at + 1;
	else
		*offset = at;
	value[n] = '\0';
	return (value);
}

void
tty_term_apply(struct tty_term *term, const char *capabilities, int quiet)
{
	const struct tty_term_code_entry	*ent;
	struct tty_code				*code;
	size_t                                   offset = 0;
	char					*cp, *value, *s;
	const char				*errstr, *name = term->name;
	u_int					 i;
	int					 n, remove;

	while ((s = tty_term_override_next(capabilities, &offset)) != NULL) {
		if (*s == '\0')
			continue;
		value = NULL;

		remove = 0;
		if ((cp = strchr(s, '=')) != NULL) {
			*cp++ = '\0';
			value = xstrdup(cp);
			if (strunvis(value, cp) == -1) {
				free(value);
				value = xstrdup(cp);
			}
		} else if (s[strlen(s) - 1] == '@') {
			s[strlen(s) - 1] = '\0';
			remove = 1;
		} else
			value = xstrdup("");

		if (!quiet) {
			if (remove)
				log_debug("%s override: %s@", name, s);
			else if (*value == '\0')
				log_debug("%s override: %s", name, s);
			else
				log_debug("%s override: %s=%s", name, s, value);
		}

		for (i = 0; i < tty_term_ncodes(); i++) {
			ent = &tty_term_codes[i];
			if (strcmp(s, ent->name) != 0)
				continue;
			code = &term->codes[i];

			if (remove) {
				code->type = TTYCODE_NONE;
				continue;
			}
			switch (ent->type) {
			case TTYCODE_NONE:
				break;
			case TTYCODE_STRING:
				if (code->type == TTYCODE_STRING)
					free(code->value.string);
				code->value.string = xstrdup(value);
				code->type = ent->type;
				break;
			case TTYCODE_NUMBER:
				n = strtonum(value, 0, INT_MAX, &errstr);
				if (errstr != NULL)
					break;
				code->value.number = n;
				code->type = ent->type;
				break;
			case TTYCODE_FLAG:
				code->value.flag = 1;
				code->type = ent->type;
				break;
			}
		}

		free(value);
	}
}

void
tty_term_apply_overrides(struct tty_term *term)
{
	struct options_entry		*o;
	struct options_array_item	*a;
	union options_value		*ov;
	const char			*s, *acs;
	size_t				 offset;
	char				*first;

	/* Update capabilities from the option. */
	o = options_get_only(global_options, "terminal-overrides");
	a = options_array_first(o);
	while (a != NULL) {
		ov = options_array_item_value(a);
		s = ov->string;

		offset = 0;
		first = tty_term_override_next(s, &offset);
		if (first != NULL && fnmatch(first, term->name, 0) == 0)
			tty_term_apply(term, s + offset, 0);
		a = options_array_next(a);
	}

	/* Log the SIXEL flag. */
	log_debug("SIXEL flag is %d", !!(term->flags & TERM_SIXEL));

	/* Update the RGB flag if the terminal has RGB colours. */
	if (tty_term_has(term, TTYC_SETRGBF) &&
	    tty_term_has(term, TTYC_SETRGBB))
		term->flags |= TERM_RGBCOLOURS;
	else
		term->flags &= ~TERM_RGBCOLOURS;
	log_debug("RGBCOLOURS flag is %d", !!(term->flags & TERM_RGBCOLOURS));

	/*
	 * Set or clear the DECSLRM flag if the terminal has the margin
	 * capabilities.
	 */
	if (tty_term_has(term, TTYC_CMG) && tty_term_has(term, TTYC_CLMG))
		term->flags |= TERM_DECSLRM;
	else
		term->flags &= ~TERM_DECSLRM;
	log_debug("DECSLRM flag is %d", !!(term->flags & TERM_DECSLRM));

	/*
	 * Set or clear the DECFRA flag if the terminal has the rectangle
	 * capability.
	 */
	if (tty_term_has(term, TTYC_RECT))
		term->flags |= TERM_DECFRA;
	else
		term->flags &= ~TERM_DECFRA;
	log_debug("DECFRA flag is %d", !!(term->flags & TERM_DECFRA));

	/*
	 * Terminals without am (auto right margin) wrap at at $COLUMNS - 1
	 * rather than $COLUMNS (the cursor can never be beyond $COLUMNS - 1).
	 *
	 * Terminals without xenl (eat newline glitch) ignore a newline beyond
	 * the right edge of the terminal, but tmux doesn't care about this -
	 * it always uses absolute only moves the cursor with a newline when
	 * also sending a linefeed.
	 *
	 * This is irritating, most notably because it is painful to write to
	 * the very bottom-right of the screen without scrolling.
	 *
	 * Flag the terminal here and apply some workarounds in other places to
	 * do the best possible.
	 */
	if (!tty_term_flag(term, TTYC_AM))
		term->flags |= TERM_NOAM;
	else
		term->flags &= ~TERM_NOAM;
	log_debug("NOAM flag is %d", !!(term->flags & TERM_NOAM));

	/* Generate ACS table. If none is present, use nearest ASCII. */
	memset(term->acs, 0, sizeof term->acs);
	if (tty_term_has(term, TTYC_ACSC))
		acs = tty_term_string(term, TTYC_ACSC);
	else
		acs = "a#j+k+l+m+n+o-p-q-r-s-t+u+v+w+x|y<z>~.";
	for (; acs[0] != '\0' && acs[1] != '\0'; acs += 2)
		term->acs[(u_char) acs[0]][0] = acs[1];
}

struct tty_term *
tty_term_create(struct tty *tty, char *name, char **caps, u_int ncaps,
    int *feat, char **cause)
{
	struct tty_term				*term;
	const struct tty_term_code_entry	*ent;
	struct tty_code				*code;
	struct options_entry			*o;
	struct options_array_item		*a;
	union options_value			*ov;
	u_int					 i, j;
	const char				*s, *value, *errstr;
	size_t					 offset, namelen;
	char					*first;
	int					 n;
	struct environ_entry			*envent;

	log_debug("adding term %s", name);

	term = xcalloc(1, sizeof *term);
	term->tty = tty;
	term->name = xstrdup(name);
	term->codes = xcalloc(tty_term_ncodes(), sizeof *term->codes);
	LIST_INSERT_HEAD(&tty_terms, term, entry);

	/* Fill in codes. */
	for (i = 0; i < ncaps; i++) {
		namelen = strcspn(caps[i], "=");
		if (namelen == 0)
			continue;
		value = caps[i] + namelen + 1;

		for (j = 0; j < tty_term_ncodes(); j++) {
			ent = &tty_term_codes[j];
			if (strncmp(ent->name, caps[i], namelen) != 0)
				continue;
			if (ent->name[namelen] != '\0')
				continue;

			code = &term->codes[j];
			code->type = TTYCODE_NONE;
			switch (ent->type) {
			case TTYCODE_NONE:
				break;
			case TTYCODE_STRING:
				code->type = TTYCODE_STRING;
				code->value.string = tty_term_strip(value);
				break;
			case TTYCODE_NUMBER:
				n = strtonum(value, 0, INT_MAX, &errstr);
				if (errstr != NULL)
					log_debug("%s: %s", ent->name, errstr);
				else {
					code->type = TTYCODE_NUMBER;
					code->value.number = n;
				}
				break;
			case TTYCODE_FLAG:
				code->type = TTYCODE_FLAG;
				code->value.flag = (*value == '1');
				break;
			}
		}
	}

	/* Apply terminal features. */
	o = options_get_only(global_options, "terminal-features");
	a = options_array_first(o);
	while (a != NULL) {
		ov = options_array_item_value(a);
		s = ov->string;

		offset = 0;
		first = tty_term_override_next(s, &offset);
		if (first != NULL && fnmatch(first, term->name, 0) == 0)
			tty_add_features(feat, s + offset, ":");
		a = options_array_next(a);
	}

	/* Check for COLORTERM. */
	envent = environ_find(tty->client->environ, "COLORTERM");
	if (envent != NULL) {
		log_debug("%s COLORTERM=%s", tty->client->name, envent->value);
		if (strcasecmp(envent->value, "truecolor") == 0 ||
		    strcasecmp(envent->value, "24bit") == 0)
			tty_add_features(feat, "RGB", ",");
 		else if (strstr(envent->value, "256") != NULL)
			tty_add_features(feat, "256", ",");
	}

	/* Apply overrides so any capabilities used for features are changed. */
	tty_term_apply_overrides(term);

	/* These are always required. */
	if (!tty_term_has(term, TTYC_CLEAR)) {
		xasprintf(cause, "terminal does not support clear");
		goto error;
	}
	if (!tty_term_has(term, TTYC_CUP)) {
		xasprintf(cause, "terminal does not support cup");
		goto error;
	}

	/*
	 * If TERM has XT or clear starts with CSI then it is safe to assume
	 * the terminal is derived from the VT100. This controls whether device
	 * attributes requests are sent to get more information.
	 *
	 * This is a bit of a hack but there aren't that many alternatives.
	 * Worst case tmux will just fall back to using whatever terminfo(5)
	 * says without trying to correct anything that is missing.
	 *
	 * Also add few features that VT100-like terminals should either
	 * support or safely ignore.
	 */
	s = tty_term_string(term, TTYC_CLEAR);
	if (tty_term_flag(term, TTYC_XT) || strncmp(s, "\033[", 2) == 0) {
		term->flags |= TERM_VT100LIKE;
		tty_add_features(feat, "bpaste,focus,title", ",");
	}

	/* Add RGB feature if terminal has RGB colours. */
	if ((tty_term_flag(term, TTYC_TC) || tty_term_has(term, TTYC_RGB)) &&
	    (!tty_term_has(term, TTYC_SETRGBF) ||
	    !tty_term_has(term, TTYC_SETRGBB)))
		tty_add_features(feat, "RGB", ",");

	/* Apply the features and overrides again. */
	if (tty_apply_features(term, *feat))
		tty_term_apply_overrides(term);

	/* Log the capabilities. */
	for (i = 0; i < tty_term_ncodes(); i++)
		log_debug("%s%s", name, tty_term_describe(term, i));

	return (term);

error:
	tty_term_free(term);
	return (NULL);
}

void
tty_term_free(struct tty_term *term)
{
	u_int	i;

	log_debug("removing term %s", term->name);

	for (i = 0; i < tty_term_ncodes(); i++) {
		if (term->codes[i].type == TTYCODE_STRING)
			free(term->codes[i].value.string);
	}
	free(term->codes);

	LIST_REMOVE(term, entry);
	free(term->name);
	free(term);
}

/*
 * Terminfo parameterized string interpreter, replacing ncurses tparm/tiparm.
 * Implements the full terminfo parameter language (stack machine with
 * arithmetic, conditionals, string params). Returns a static buffer.
 */

static const char *
tparm_skip_to_eb(const char *s)
{
	/* Skip forward to matching %e or %; at depth 0. Returns ptr to the %. */
	int	depth = 0;

	while (*s != '\0') {
		if (*s != '%') {
			s++;
			continue;
		}
		switch (*(s + 1)) {
		case '?':
			depth++;
			s += 2;
			break;
		case 'e':
			if (depth == 0)
				return s;
			s += 2;
			break;
		case ';':
			if (depth == 0)
				return s;
			depth--;
			s += 2;
			break;
		case '{':
			s += 2;
			while (*s != '\0' && *s != '}')
				s++;
			if (*s != '\0')
				s++;
			break;
		case '\'':
			s += 2;
			if (*s != '\0')
				s++;
			if (*s == '\'')
				s++;
			break;
		default:
			s += 2;
			break;
		}
	}
	return s;
}

static const char *
tparm_skip_to_end(const char *s)
{
	/* Skip forward to matching %; at depth 0. Returns ptr to the %. */
	int	depth = 0;

	while (*s != '\0') {
		if (*s != '%') {
			s++;
			continue;
		}
		switch (*(s + 1)) {
		case '?':
			depth++;
			s += 2;
			break;
		case ';':
			if (depth == 0)
				return s;
			depth--;
			s += 2;
			break;
		case '{':
			s += 2;
			while (*s != '\0' && *s != '}')
				s++;
			if (*s != '\0')
				s++;
			break;
		case '\'':
			s += 2;
			if (*s != '\0')
				s++;
			if (*s == '\'')
				s++;
			break;
		default:
			s += 2;
			break;
		}
	}
	return s;
}

static char tparm_buf[4096];

static const char *
tmux_tparm(const char *s, long p1, long p2, long p3, long p4,
    long p5, long p6, long p7, long p8, long p9)
{
	long		params[9] = { p1, p2, p3, p4, p5, p6, p7, p8, p9 };
	long		stack[32];
	int		sp = 0;
	size_t		outlen = 0;
	int		if_stack[16];
	int		if_level = 0;
	const char     *sv;
	int		len, n;
	long		a, b;

#define PUSH(x)  do { if (sp < 32) stack[sp++] = (x); } while (0)
#define POP()    (sp > 0 ? stack[--sp] : 0L)
#define OUTC(c)  do { \
    if (outlen < sizeof tparm_buf - 1) tparm_buf[outlen++] = (char)(c); \
} while (0)

	memset(if_stack, 0, sizeof if_stack);

	while (*s != '\0') {
		if (*s != '%') {
			OUTC(*s++);
			continue;
		}
		s++; /* consume '%' */
		switch (*s++) {
		case '%':
			OUTC('%');
			break;
		case 'd':
			a = POP();
			len = snprintf(tparm_buf + outlen,
			    sizeof tparm_buf - outlen, "%ld", a);
			if (len > 0)
				outlen += (size_t)len;
			break;
		case 'c':
			OUTC(POP());
			break;
		case 's':
			sv = (const char *)(intptr_t)POP();
			if (sv != NULL) {
				while (*sv != '\0' &&
				    outlen < sizeof tparm_buf - 1)
					tparm_buf[outlen++] = *sv++;
			}
			break;
		case 'p':
			n = *s++ - '1';
			PUSH(n >= 0 && n < 9 ? params[n] : 0L);
			break;
		case '\'':
			PUSH((unsigned char)*s);
			if (*s != '\0') s++;
			if (*s == '\'') s++;
			break;
		case '{':
			n = 0;
			while (*s >= '0' && *s <= '9')
				n = n * 10 + (*s++ - '0');
			if (*s == '}') s++;
			PUSH((long)n);
			break;
		case 'l':
			sv = (const char *)(intptr_t)POP();
			PUSH(sv != NULL ? (long)strlen(sv) : 0L);
			break;
		case 'i':
			params[0]++;
			params[1]++;
			break;
		case '+': b = POP(); a = POP(); PUSH(a + b); break;
		case '-': b = POP(); a = POP(); PUSH(a - b); break;
		case '*': b = POP(); a = POP(); PUSH(a * b); break;
		case '/': b = POP(); a = POP(); PUSH(b != 0 ? a / b : 0L); break;
		case 'm': b = POP(); a = POP(); PUSH(b != 0 ? a % b : 0L); break;
		case '&': b = POP(); a = POP(); PUSH(a & b); break;
		case '|': b = POP(); a = POP(); PUSH(a | b); break;
		case '^': b = POP(); a = POP(); PUSH(a ^ b); break;
		case '=': b = POP(); a = POP(); PUSH(a == b ? 1L : 0L); break;
		case '>': b = POP(); a = POP(); PUSH(a > b ? 1L : 0L); break;
		case '<': b = POP(); a = POP(); PUSH(a < b ? 1L : 0L); break;
		case 'A': b = POP(); a = POP(); PUSH(a && b ? 1L : 0L); break;
		case 'O': b = POP(); a = POP(); PUSH(a || b ? 1L : 0L); break;
		case '!': PUSH(!POP() ? 1L : 0L); break;
		case '~': PUSH(~POP()); break;
		case '?':
			/* %? marks start of conditional; push a new if-level */
			if (if_level < 15)
				if_stack[++if_level] = 0;
			break;
		case 't':
			/* %t: pop condition; skip then-part if false */
			a = POP();
			if_stack[if_level] = (int)(a != 0);
			if (!a)
				s = tparm_skip_to_eb(s);
			break;
		case 'e':
			/* %e: skip else-part if then-part was taken */
			if (if_level > 0 && if_stack[if_level])
				s = tparm_skip_to_end(s);
			break;
		case ';':
			/* %; ends the conditional */
			if (if_level > 0)
				if_level--;
			break;
		default:
			break;
		}
	}
	tparm_buf[outlen] = '\0';
	return tparm_buf;

#undef PUSH
#undef POP
#undef OUTC
}

/*
 * Hardcoded terminfo capabilities for tmux-256color.
 * Values are the raw binary escape sequences (same format as tigetstr returns).
 * Parameterized strings keep their %p1/%d/etc. format for tmux_tparm.
 * Only capabilities present in tty_term_codes are included.
 */
static const char *tmux_256color_caps[] = {
	/* flags */
	"am=1",
	/* numbers */
	"colors=256",
	/* strings */
	"acsc=++,,--..00``aaffgghhiijjkkllmmnnooppqqrrssttuuvvwwxxyyzz{{||}}~~",
	"bel=\007",
	"blink=\033[5m",
	"bold=\033[1m",
	"civis=\033[?25l",
	"clear=\033[H\033[J",
	"cnorm=\033[34h\033[?25h",
	"csr=\033[%i%p1%d;%p2%dr",
	"cub1=\010",
	"cub=\033[%p1%dD",
	"cud1=\012",
	"cud=\033[%p1%dB",
	"cuf1=\033[C",
	"cuf=\033[%p1%dC",
	"cup=\033[%i%p1%d;%p2%dH",
	"cuu1=\033M",
	"cuu=\033[%p1%dA",
	"cvvis=\033[34l",
	"dch1=\033[P",
	"dch=\033[%p1%dP",
	"dim=\033[2m",
	"dl1=\033[M",
	"dl=\033[%p1%dM",
	"ed=\033[J",
	"el1=\033[1K",
	"el=\033[K",
	"enacs=\033(B\033)0",
	"fsl=\007",
	"home=\033[H",
	"ich=\033[%p1%d@",
	"il1=\033[L",
	"il=\033[%p1%dL",
	"kcbt=\033[Z",
	"kcub1=\033OD",
	"kcud1=\033OB",
	"kcuf1=\033OC",
	"kcuu1=\033OA",
	"kdch1=\033[3~",
	"kend=\033[4~",
	"kf1=\033OP",
	"kf10=\033[21~",
	"kf11=\033[23~",
	"kf12=\033[24~",
	"kf2=\033OQ",
	"kf3=\033OR",
	"kf4=\033OS",
	"kf5=\033[15~",
	"kf6=\033[17~",
	"kf7=\033[18~",
	"kf8=\033[19~",
	"kf9=\033[20~",
	"khome=\033[1~",
	"kich1=\033[2~",
	"kmous=\033[M",
	"knp=\033[6~",
	"kpp=\033[5~",
	"op=\033[39;49m",
	"rev=\033[7m",
	"ri=\033M",
	"ritm=\033[23m",
	"rmacs=\017",
	"rmcup=\033[?1049l",
	"rmkx=\033[?1l\033>",
	"setab=\033[%?%p1%{8}%<%t4%p1%d%e%p1%{16}%<%t10%p1%{8}%-%d%e48;5;%p1%d%;m",
	"setaf=\033[%?%p1%{8}%<%t3%p1%d%e%p1%{16}%<%t9%p1%{8}%-%d%e38;5;%p1%d%;m",
	"sgr0=\033[m\017",
	"sitm=\033[3m",
	"smacs=\016",
	"smcup=\033[?1049h",
	"smkx=\033[?1h\033=",
	"smso=\033[7m",
	"smul=\033[4m",
	"tsl=\033]0;",
	/* OSC 52 clipboard set: Ms=%p1%s is clip target, %p2%s is base64 data */
	"Ms=\033]52;%p1%s;%p2%s\007",
	NULL
};

static const char *xterm_256color_caps[] = {
	/* flags */
	"am=1", "bce=1", "km=1", "mir=1", "msgr=1", "xenl=1",
	/* numbers */
	"colors=256",
	/* strings */
	"acsc=``aaffggiijjkkllmmnnooppqqrrssttuuvvwwxxyyzz{{||}}~~",
	"bel=\007",
	"blink=\033[5m",
	"bold=\033[1m",
	"civis=\033[?25l",
	"clear=\033[H\033[2J",
	"cnorm=\033[?12l\033[?25h",
	"csr=\033[%i%p1%d;%p2%dr",
	"cub1=\010",
	"cub=\033[%p1%dD",
	"cud1=\012",
	"cud=\033[%p1%dB",
	"cuf1=\033[C",
	"cuf=\033[%p1%dC",
	"cup=\033[%i%p1%d;%p2%dH",
	"cuu1=\033[A",
	"cuu=\033[%p1%dA",
	"cvvis=\033[?12;25h",
	"dch1=\033[P",
	"dch=\033[%p1%dP",
	"dim=\033[2m",
	"dl1=\033[M",
	"dl=\033[%p1%dM",
	"ech=\033[%p1%dX",
	"ed=\033[J",
	"el1=\033[1K",
	"el=\033[K",
	"home=\033[H",
	"hpa=\033[%i%p1%dG",
	"ich=\033[%p1%d@",
	"il1=\033[L",
	"il=\033[%p1%dL",
	"indn=\033[%p1%dS",
	"invis=\033[8m",
	"kDC=\033[3;2~",
	"kEND=\033[1;2F",
	"kHOM=\033[1;2H",
	"kIC=\033[2;2~",
	"kLFT=\033[1;2D",
	"kNXT=\033[6;2~",
	"kPRV=\033[5;2~",
	"kRIT=\033[1;2C",
	"kcbt=\033[Z",
	"kcub1=\033OD",
	"kcud1=\033OB",
	"kcuf1=\033OC",
	"kcuu1=\033OA",
	"kdch1=\033[3~",
	"kend=\033OF",
	"kf1=\033OP",
	"kf10=\033[21~",
	"kf11=\033[23~",
	"kf12=\033[24~",
	"kf13=\033[1;2P",
	"kf14=\033[1;2Q",
	"kf15=\033[1;2R",
	"kf16=\033[1;2S",
	"kf17=\033[15;2~",
	"kf18=\033[17;2~",
	"kf19=\033[18;2~",
	"kf2=\033OQ",
	"kf20=\033[19;2~",
	"kf21=\033[20;2~",
	"kf22=\033[21;2~",
	"kf23=\033[23;2~",
	"kf24=\033[24;2~",
	"kf25=\033[1;5P",
	"kf26=\033[1;5Q",
	"kf27=\033[1;5R",
	"kf28=\033[1;5S",
	"kf29=\033[15;5~",
	"kf3=\033OR",
	"kf30=\033[17;5~",
	"kf31=\033[18;5~",
	"kf32=\033[19;5~",
	"kf33=\033[20;5~",
	"kf34=\033[21;5~",
	"kf35=\033[23;5~",
	"kf36=\033[24;5~",
	"kf37=\033[1;6P",
	"kf38=\033[1;6Q",
	"kf39=\033[1;6R",
	"kf4=\033OS",
	"kf40=\033[1;6S",
	"kf41=\033[15;6~",
	"kf42=\033[17;6~",
	"kf43=\033[18;6~",
	"kf44=\033[19;6~",
	"kf45=\033[20;6~",
	"kf46=\033[21;6~",
	"kf47=\033[23;6~",
	"kf48=\033[24;6~",
	"kf49=\033[1;3P",
	"kf5=\033[15~",
	"kf50=\033[1;3Q",
	"kf51=\033[1;3R",
	"kf52=\033[1;3S",
	"kf53=\033[15;3~",
	"kf54=\033[17;3~",
	"kf55=\033[18;3~",
	"kf56=\033[19;3~",
	"kf57=\033[20;3~",
	"kf58=\033[21;3~",
	"kf59=\033[23;3~",
	"kf6=\033[17~",
	"kf60=\033[24;3~",
	"kf61=\033[1;4P",
	"kf62=\033[1;4Q",
	"kf63=\033[1;4R",
	"kf7=\033[18~",
	"kf8=\033[19~",
	"kf9=\033[20~",
	"khome=\033OH",
	"kich1=\033[2~",
	"kind=\033[1;2B",
	"kmous=\033[M",
	"knp=\033[6~",
	"kpp=\033[5~",
	"kri=\033[1;2A",
	"op=\033[39;49m",
	"rev=\033[7m",
	"ri=\033M",
	"rin=\033[%p1%dT",
	"ritm=\033[23m",
	"rmacs=\033(B",
	"rmcup=\033[?1049l",
	"rmkx=\033[?1l\033>",
	"setab=\033[%?%p1%{8}%<%t4%p1%d%e%p1%{16}%<%t10%p1%{8}%-%d%e48;5;%p1%d%;m",
	"setaf=\033[%?%p1%{8}%<%t3%p1%d%e%p1%{16}%<%t9%p1%{8}%-%d%e38;5;%p1%d%;m",
	"sgr0=\033(B\033[m",
	"sitm=\033[3m",
	"smacs=\033(0",
	"smcup=\033[?1049h",
	"smkx=\033[?1h\033=",
	"smso=\033[7m",
	"smul=\033[4m",
	"vpa=\033[%i%p1%dd",
	/* OSC 52 clipboard set: Ms=%p1%s is clip target, %p2%s is base64 data */
	"Ms=\033]52;%p1%s;%p2%s\007",
	NULL
};

static void
load_caps(const char **src, char ***caps, u_int *ncaps)
{
	u_int	i;

	*ncaps = 0;
	*caps = NULL;
	for (i = 0; src[i] != NULL; i++) {
		*caps = xreallocarray(*caps, (*ncaps) + 1, sizeof **caps);
		(*caps)[*ncaps] = xstrdup(src[i]);
		(*ncaps)++;
	}
}

int
tty_term_read_list(const char *name, int fd, char ***caps, u_int *ncaps,
    char **cause)
{
	(void)fd;

	if (strcmp(name, "tmux-256color") == 0) {
		load_caps(tmux_256color_caps, caps, ncaps);
		return (0);
	}
	if (strcmp(name, "xterm-256color") == 0) {
		load_caps(xterm_256color_caps, caps, ncaps);
		return (0);
	}
	xasprintf(cause, "unsupported terminal: %s "
	    "(only tmux-256color and xterm-256color are supported without ncurses)",
	    name);
	return (-1);
}

void
tty_term_free_list(char **caps, u_int ncaps)
{
	u_int	i;

	for (i = 0; i < ncaps; i++)
		free(caps[i]);
	free(caps);
}

int
tty_term_has(struct tty_term *term, enum tty_code_code code)
{
	return (term->codes[code].type != TTYCODE_NONE);
}

const char *
tty_term_string(struct tty_term *term, enum tty_code_code code)
{
	if (!tty_term_has(term, code))
		return ("");
	if (term->codes[code].type != TTYCODE_STRING)
		fatalx("not a string: %d", code);
	return (term->codes[code].value.string);
}

const char *
tty_term_string_i(struct tty_term *term, enum tty_code_code code, int a)
{
	const char	*x = tty_term_string(term, code);

	return tmux_tparm(x, (long)a, 0, 0, 0, 0, 0, 0, 0, 0);
}

const char *
tty_term_string_ii(struct tty_term *term, enum tty_code_code code, int a, int b)
{
	const char	*x = tty_term_string(term, code);

	return tmux_tparm(x, (long)a, (long)b, 0, 0, 0, 0, 0, 0, 0);
}

const char *
tty_term_string_iii(struct tty_term *term, enum tty_code_code code, int a,
    int b, int c)
{
	const char	*x = tty_term_string(term, code);

	return tmux_tparm(x, (long)a, (long)b, (long)c, 0, 0, 0, 0, 0, 0);
}

const char *
tty_term_string_s(struct tty_term *term, enum tty_code_code code, const char *a)
{
	const char	*x = tty_term_string(term, code);

	return tmux_tparm(x, (long)(intptr_t)a, 0, 0, 0, 0, 0, 0, 0, 0);
}

const char *
tty_term_string_ss(struct tty_term *term, enum tty_code_code code,
    const char *a, const char *b)
{
	const char	*x = tty_term_string(term, code);

	return tmux_tparm(x, (long)(intptr_t)a, (long)(intptr_t)b,
	    0, 0, 0, 0, 0, 0, 0);
}

int
tty_term_number(struct tty_term *term, enum tty_code_code code)
{
	if (!tty_term_has(term, code))
		return (0);
	if (term->codes[code].type != TTYCODE_NUMBER)
		fatalx("not a number: %d", code);
	return (term->codes[code].value.number);
}

int
tty_term_flag(struct tty_term *term, enum tty_code_code code)
{
	if (!tty_term_has(term, code))
		return (0);
	if (term->codes[code].type != TTYCODE_FLAG)
		fatalx("not a flag: %d", code);
	return (term->codes[code].value.flag);
}

const char *
tty_term_describe(struct tty_term *term, enum tty_code_code code)
{
	static char	 s[256];
	char		 out[128];

	switch (term->codes[code].type) {
	case TTYCODE_NONE:
		xsnprintf(s, sizeof s, "%4u: %s: [missing]",
		    code, tty_term_codes[code].name);
		break;
	case TTYCODE_STRING:
		strnvis(out, term->codes[code].value.string, sizeof out,
		    VIS_OCTAL|VIS_CSTYLE|VIS_TAB|VIS_NL);
		xsnprintf(s, sizeof s, "%4u: %s: (string) %s",
		    code, tty_term_codes[code].name,
		    out);
		break;
	case TTYCODE_NUMBER:
		xsnprintf(s, sizeof s, "%4u: %s: (number) %d",
		    code, tty_term_codes[code].name,
		    term->codes[code].value.number);
		break;
	case TTYCODE_FLAG:
		xsnprintf(s, sizeof s, "%4u: %s: (flag) %s",
		    code, tty_term_codes[code].name,
		    term->codes[code].value.flag ? "true" : "false");
		break;
	}
	return (s);
}
