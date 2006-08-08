/* Copyright (c) 2003,2004,2005,2006 David Lichteblau
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
#include "common.h"

t_print_binary_mode print_binary_mode = PRINT_UTF8;

static void
write_backslashed(FILE *s, char *ptr, int n)
{
	int i;
	for (i = 0; i < n; i++) {
		char c = ptr[i];
		if (c == '\n' || c == '\\') fputc('\\', s);
		fputc(c, s);
	}
	if (ferror(s)) syserr();
}

static int
utf8_string_p(unsigned char *str, int n)
{
	int i = 0;
	while (i < n) {
		unsigned char c = str[i++];
		if (c >= 0xfe)
			return 0;
		if (c >= 0xfc) {
			unsigned char d;
			if ((n - i < 5)
			    || ((d=str[i++]) ^ 0x80) >= 0x40
			    || (str[i++] ^ 0x80) >= 0x40
			    || (str[i++] ^ 0x80) >= 0x40
			    || (str[i++] ^ 0x80) >= 0x40
			    || (str[i++] ^ 0x80) >= 0x40
			    || (c < 0xfd && d < 0x84))
				return 0;
		} else if (c >= 0xf8) {
			unsigned char d;
			if ((n - i < 4)
			    || ((d=str[i++]) ^ 0x80) >= 0x40
			    || (str[i++] ^ 0x80) >= 0x40
			    || (str[i++] ^ 0x80) >= 0x40
			    || (str[i++] ^ 0x80) >= 0x40
			    || (c < 0xf9 && d < 0x88))
				return 0;
		} else if (c >= 0xf0) {
			unsigned char d;
			if ((n - i < 3)
			    || ((d=str[i++]) ^ 0x80) >= 0x40
			    || (str[i++] ^ 0x80) >= 0x40
			    || (str[i++] ^ 0x80) >= 0x40
			    || (c < 0xf1 && d < 0x90))
				return 0;
		} else if (c >= 0xe0) {
			unsigned char d, e;
			unsigned code;
			if ((n - i < 2)
			    || ((d=str[i++]) ^ 0x80) >= 0x40
			    || ((e=str[i++]) ^ 0x80) >= 0x40
			    || (c < 0xe1 && d < 0xa0))
				return 0;
			code = ((int) c & 0x0f) << 12
				| ((int) d ^ 0x80) << 6
				| ((int) e ^ 0x80);
			if ((0xd800 <= code) && (code <= 0xdfff)
			    || code == 0xfffe || code == 0xffff)
				return 0;
		} else if (c >= 0x80) {
			unsigned char d;
			if ((n - i < 1)
			    || ((d=str[i++]) ^ 0x80) >= 0x40
			    || (c < 0xc2))
				return 0;
		} else if (c == 0)
			return 0;
	}
	return 1;
}

static int
readable_string_p(char *str, int n)
{
	int i;
	for (i = 0; i < n; i++) {
		char c = str[i];
		if (c < 32 && c != '\n' && c != '\t')
			return 0;
	}
	return 1;
}

static int
safe_string_p(char *str, int n)
{
	unsigned char c;
	int i;

	if (n == 0) return 1;
		
	c = str[0];
	if ((c == ' ') || (c == ':') || (c == '<'))
		return 0;
	
	for (i = 0; i < n; i++) {
		c = str[i];
		if ((c == '\0') || (c == '\r') || (c == '\n') || (c >= 0x80))
			return 0;
	}
	return 1;
}

void
print_attrval(FILE *s, char *str, int len)
{
	int readablep;
	switch (print_binary_mode) {
	case PRINT_ASCII:
		readablep = readable_string_p(str, len);
		break;
	case PRINT_UTF8:
		readablep = utf8_string_p((unsigned char *) str, len);
		break;
	case PRINT_JUNK:
		readablep = 1;
		break;
	default:
		abort();
	}

	if (!readablep) {
		fputs(":: ", s);
		print_base64((unsigned char *) str, len, s);
	} else if (!safe_string_p(str, len)) {
		fputs(":; ", s);
		write_backslashed(s, str, len);
	} else {
		fputs(": ", s);
		fwrite(str, 1, len, s);
	}
}

static void
print_attribute(FILE *s, tattribute *attribute)
{
	GPtrArray *values = attribute_values(attribute);
	int j;
	
	for (j = 0; j < values->len; j++) {
		GArray *av = g_ptr_array_index(values, j);
		fputs(attribute_ad(attribute), s);
		print_attrval(s, av->data, av->len);
		fputc('\n', s);
	}
	if (ferror(s)) syserr();
}

void
print_entry_object(FILE *s, tentry *entry, char *key)
{
	GPtrArray *attributes = entry_attributes(entry);
	int i;

	fputc('\n', s);
	fputs(key ? key : "entry", s);
	fputc(' ', s);
	fputs(entry_dn(entry), s);
	fputc('\n', s);
	if (ferror(s)) syserr();

	for (i = 0; i < attributes->len; i++) {
		tattribute *attribute = g_ptr_array_index(attributes, i);
		print_attribute(s, attribute);
	}
}

static void
print_ldapvi_ldapmod(FILE *s, LDAPMod *mod)
{
	struct berval **values = mod->mod_bvalues;

	switch (mod->mod_op & ~LDAP_MOD_BVALUES) {
	case LDAP_MOD_ADD: fputs("add", s); break;
	case LDAP_MOD_DELETE: fputs("delete", s); break;
	case LDAP_MOD_REPLACE: fputs("replace", s); break;
	default: abort();
	}
	print_attrval(s, mod->mod_type, strlen(mod->mod_type));
	fputc('\n', s);
	for (; *values; values++) {
		struct berval *value = *values;
		print_attrval(s, value->bv_val, value->bv_len);
		fputc('\n', s);
	}
	if (ferror(s)) syserr();
}

void
print_ldapvi_modify(FILE *s, char *dn, LDAPMod **mods)
{
	fputs("\nmodify", s);
	print_attrval(s, dn, strlen(dn));
	fputc('\n', s);

	for (; *mods; mods++)
		print_ldapvi_ldapmod(s, *mods);
	if (ferror(s)) syserr();
}

void
print_ldapvi_rename(FILE *s, char *olddn, char *newdn, int deleteoldrdn)
{
	fputs("\nrename", s);
	print_attrval(s, olddn, strlen(olddn));
	fputs(deleteoldrdn ? "\nreplace" : "\nadd", s);
	print_attrval(s, newdn, strlen(newdn));
	fputc('\n', s);
	if (ferror(s)) syserr();
}

void
print_ldapvi_add(FILE *s, char *dn, LDAPMod **mods)
{
	fputs("\nadd", s);
	print_attrval(s, dn, strlen(dn));
	fputc('\n', s);

	for (; *mods; mods++) {
		LDAPMod *mod = *mods;
		struct berval **values = mod->mod_bvalues;
		for (; *values; values++) {
			struct berval *value = *values;
			fputs(mod->mod_type, s);
			print_attrval(s, value->bv_val, value->bv_len);
			fputc('\n', s);
		}
	}
	if (ferror(s)) syserr();
}

void
print_ldapvi_delete(FILE *s, char *dn)
{
	fputs("\ndelete", s);
	print_attrval(s, dn, strlen(dn));
	fputc('\n', s);
	if (ferror(s)) syserr();
}

static void
print_ldif_ldapmod(FILE *s, LDAPMod *mod)
{
	struct berval **values = mod->mod_bvalues;
	for (; *values; values++) {
		struct berval *value = *values;
		fputs(mod->mod_type, s);
		if (safe_string_p(value->bv_val, value->bv_len)) {
			fputs(": ", s);
			fwrite(value->bv_val, value->bv_len, 1, s);
		} else {
			fputs(":: ", s);
			print_base64((unsigned char *) value->bv_val,
				     value->bv_len,
				     s);
		}
		fputs("\n", s);
	}
	if (ferror(s)) syserr();
}

void
print_ldif_modify(FILE *s, char *dn, LDAPMod **mods)
{
	fputs("\ndn: ", s);
	fputs(dn, s);
	fputs("\nchangetype: modify\n", s);

	for (; *mods; mods++) {
		LDAPMod *mod = *mods;

		switch (mod->mod_op & ~LDAP_MOD_BVALUES) {
		case LDAP_MOD_ADD: fputs("add: ", s); break;
		case LDAP_MOD_DELETE: fputs("delete: ", s); break;
		case LDAP_MOD_REPLACE: fputs("replace: ", s); break;
		default: abort();
		}
		fputs(mod->mod_type, s);
		fputc('\n', s);

		print_ldif_ldapmod(s, mod);
		fputs("-\n", s);
	}
	if (ferror(s)) syserr();
}

void
print_ldif_add(FILE *s, char *dn, LDAPMod **mods)
{
	fputs("\ndn: ", s);
	fputs(dn, s);
	fputs("\nchangetype: add\n", s);

	for (; *mods; mods++)
		print_ldif_ldapmod(s, *mods);
	if (ferror(s)) syserr();
}

void
print_ldif_rename(FILE *s, char *olddn, char *newdn, int deleteoldrdn)
{
	char **newrdns = ldap_explode_dn(newdn, 0); 
	char **ptr = newrdns;
	
	fputs("\ndn: ", s);
	fputs(olddn, s);
	fputs("\nchangetype: modrdn\nnewrdn: ", s);
	fputs(*ptr, s);  /* non-null (checked in validate_rename) */
	fprintf(s, "\ndeleteoldrdn: %d\nnewsuperior: ", !!deleteoldrdn);
	ptr++;
	if (*ptr)
		fputs(*ptr, s);
	ptr++;
	for (; *ptr; ptr++) {
		fputc(',', s);
		fputs(*ptr, s);
	}
	fputc('\n', s);
	if (ferror(s)) syserr();
	ldap_value_free(newrdns);
}

void
print_ldif_delete(FILE *s, char *dn)
{
	fputs("\ndn: ", s);
	fputs(dn, s);
	fputs("\nchangetype: delete\n", s);
	if (ferror(s)) syserr();
}
