/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
/*
 * Copyright (c) 1996, 1998 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */
/*
 * Portions Copyright (c) 1995 by International Business Machines, Inc.
 *
 * International Business Machines, Inc. (hereinafter called IBM) grants
 * permission under its copyrights to use, copy, modify, and distribute this
 * Software with or without fee, provided that the above copyright notice and
 * all paragraphs of this notice appear in all copies, and that the name of IBM
 * not be used in connection with the marketing of any product incorporating
 * the Software or modifications thereof, without specific, written prior
 * permission.
 *
 * To the extent it has a right to do so, IBM grants an immunity from suit
 * under its patents, if any, for the use, sale or manufacture of products to
 * the extent that such products are used for performing Domain Name System
 * dynamic updates in TCP/IP networks by means of the Software.  No immunity is
 * granted for any product per se or for any other function of any product.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", AND IBM DISCLAIMS ALL WARRANTIES,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  IN NO EVENT SHALL IBM BE LIABLE FOR ANY SPECIAL,
 * DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE, EVEN
 * IF IBM IS APPRISED OF THE POSSIBILITY OF SUCH DAMAGES.
 */
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "common.h"

static const char Base64[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char Pad64 = '=';

void
print_base64(
	unsigned char const *src,
	size_t srclength,
	FILE *s)
{
	unsigned char input[3];
	unsigned char output[4];
	size_t i;
	int col = 0;

	while (2 < srclength) {
		input[0] = *src++;
		input[1] = *src++;
		input[2] = *src++;
		srclength -= 3;

		output[0] = input[0] >> 2;
		output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
		output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
		output[3] = input[2] & 0x3f;

		if (col >= 76) {
			fputs("\n ", s);
			col = 0;
		}
		col += 4;

		fputc(Base64[output[0]], s);
		fputc(Base64[output[1]], s);
		fputc(Base64[output[2]], s);
		fputc(Base64[output[3]], s);
	}
    
	/* Now we worry about padding. */
	if (0 != srclength) {
		/* Get what's left. */
		input[0] = input[1] = input[2] = '\0';
		for (i = 0; i < srclength; i++)
			input[i] = *src++;
	
		output[0] = input[0] >> 2;
		output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
		output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);

		fputc(Base64[output[0]], s);
		fputc(Base64[output[1]], s);
		if (srclength == 1)
			fputc(Pad64, s);
		else
			fputc(Base64[output[2]], s);
		fputc(Pad64, s);
	}
}

/* na toll, copy und paste von oben */
void
g_string_append_base64(
	GString *string, unsigned char const *src, size_t srclength)
{
	unsigned char input[3];
	unsigned char output[4];
	size_t i;
	int col = 0;

	while (2 < srclength) {
		input[0] = *src++;
		input[1] = *src++;
		input[2] = *src++;
		srclength -= 3;

		output[0] = input[0] >> 2;
		output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
		output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
		output[3] = input[2] & 0x3f;

		if (col >= 76) {
			g_string_append(string, "\n ");
			col = 0;
		}
		col += 4;

		g_string_append_c(string, Base64[output[0]]);
		g_string_append_c(string, Base64[output[1]]);
		g_string_append_c(string, Base64[output[2]]);
		g_string_append_c(string, Base64[output[3]]);
	}
    
	/* Now we worry about padding. */
	if (0 != srclength) {
		/* Get what's left. */
		input[0] = input[1] = input[2] = '\0';
		for (i = 0; i < srclength; i++)
			input[i] = *src++;
	
		output[0] = input[0] >> 2;
		output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
		output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);

		g_string_append_c(string, Base64[output[0]]);
		g_string_append_c(string, Base64[output[1]]);
		if (srclength == 1)
			g_string_append_c(string, Pad64);
		else
			g_string_append_c(string, Base64[output[2]]);
		g_string_append_c(string, Pad64);
	}
}

int
read_base64(
	char const *src,
	unsigned char *target, 
	size_t targsize)
{
	int tarindex, state, ch;
	char *pos;

	state = 0;
	tarindex = 0;

	while ((ch = *src++) != '\0') {
		if (isascii(ch) && isspace(ch))	/* Skip whitespace anywhere. */
			continue;

		if (ch == Pad64)
			break;

		pos = strchr(Base64, ch);
		if (pos == 0) 		/* A non-base64 character. */
			return (-1);

		switch (state) {
		case 0:
			if (target) {
				if ((size_t)tarindex >= targsize)
					return (-1);
				target[tarindex] = (pos - Base64) << 2;
			}
			state = 1;
			break;
		case 1:
			if (target) {
				if ((size_t)tarindex + 1 >= targsize)
					return (-1);
				target[tarindex]   |=  (pos - Base64) >> 4;
				target[tarindex+1]  = ((pos - Base64) & 0x0f)
							<< 4 ;
			}
			tarindex++;
			state = 2;
			break;
		case 2:
			if (target) {
				if ((size_t)tarindex + 1 >= targsize)
					return (-1);
				target[tarindex]   |=  (pos - Base64) >> 2;
				target[tarindex+1]  = ((pos - Base64) & 0x03)
							<< 6;
			}
			tarindex++;
			state = 3;
			break;
		case 3:
			if (target) {
				if ((size_t)tarindex >= targsize)
					return (-1);
				target[tarindex] |= (pos - Base64);
			}
			tarindex++;
			state = 0;
			break;
		default:
			abort();
		}
	}

	/*
	 * We are done decoding Base-64 chars.  Let's see if we ended
	 * on a byte boundary, and/or with erroneous trailing characters.
	 */

	if (ch == Pad64) {		/* We got a pad char. */
		ch = *src++;		/* Skip it, get next. */
		switch (state) {
		case 0:		/* Invalid = in first position */
		case 1:		/* Invalid = in second position */
			return (-1);

		case 2:		/* Valid, means one byte of info */
			/* Skip any number of spaces. */
			for ((void)NULL; ch != '\0'; ch = *src++)
				if (! (isascii(ch) && isspace(ch)))
					break;
			/* Make sure there is another trailing = sign. */
			if (ch != Pad64)
				return (-1);
			ch = *src++;		/* Skip the = */
			/* Fall through to "single trailing =" case. */
			/* FALLTHROUGH */

		case 3:		/* Valid, means two bytes of info */
			/*
			 * We know this char is an =.  Is there anything but
			 * whitespace after it?
			 */
			for ((void)NULL; ch != '\0'; ch = *src++)
				if (! (isascii(ch) && isspace(ch)))
					return (-1);

			/*
			 * Now make sure for cases 2 and 3 that the "extra"
			 * bits that slopped past the last full byte were
			 * zeros.  If we don't check them, they become a
			 * subliminal channel.
			 */
			if (target && target[tarindex] != 0)
				return (-1);
		}
	} else {
		/*
		 * We ended by seeing the end of the string.  Make sure we
		 * have no partial bytes lying around.
		 */
		if (state != 0)
			return (-1);
	}

	return (tarindex);
}
