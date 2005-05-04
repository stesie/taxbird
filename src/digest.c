/* Copyright(C) 2005 Stefan Siegl <ssiegl@gmx.de>
 * taxbird - free program to interface with German IRO's Elster/Coala
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <ctype.h>
#include <mhash.h>

#include "guile.h"



/* convert the md5hash as stored in the .sig file (32char hexdump notation)
 * to binary (internal) notation
 *
 * return ptr to static char array
 */
static const char *
taxbird_digest_hexdecode(const char *in)
{
  static char md5hash[16];
  int i;
  
#define unhex(i) ((i) > '9' ? (((i) & ~32) - 'A' + 10) : ((i) - '0'))
  for(i = 0; i < 16; i ++) {
    md5hash[i] = (unhex(in[0]) << 4) + unhex(in[1]);
    in += 2;
  }
   
  return md5hash;
}



static int
taxbird_digest_verify_file(const char *filename, const char *sighash) 
{
  MHASH ctx;
  unsigned int i;
  char buf[512];
  char *lookup_fn = taxbird_guile_dirlist_lookup(filename);
  FILE *handle = fopen(lookup_fn, "r");

  g_free(lookup_fn);
  if(! handle) return 1; /* error */
  
  ctx = mhash_init(MHASH_MD5);
  if(ctx == MHASH_FAILED) {
    fclose(handle);
    return 1; /* error */
  }

  while((i = fread(buf, 1, sizeof(buf), handle)))
    mhash(ctx, buf, i);

  mhash_deinit(ctx, buf);
  fclose(handle);
  
  /* the md5 hash is in 'buf' now ... */
  for(i = 0; i < 16; i ++)
    if(sighash[i] != buf[i]) 
      return 1; /* mismatch */

  return 0;
}


/* compare the md5 hash values from a .sig file (contents in 'sig') with
 * the actual files
 *
 * return: 0 is okay, 1 or errno on error
 */
int
taxbird_digest_verify(const char *sig)
{
  char buf[64];
  const char *ptr;
  unsigned int i;

  while(*sig) {
    if(isspace(*sig)) {
      /* ignore leading whitespace */
      sig ++;
      continue;
    }

    if(*sig == '#') {
      /* comment field, skip till end of line */
      sig = strchr(sig, 10);
      if(! sig) 
	return 1;

      continue;
    }

    /* match for an md5sum now ... */
    for(i = 0; i < 32; i++)
      if(! isxdigit(sig[i]))
	goto out_syntax_err;

    if(!isspace(sig[32]) || !isspace(sig[33]))
      goto out_syntax_err;

    ptr = strchr(sig + 34, 10); /* search end of line */
    i = ptr - sig - 34;         /* length of filename */

    if(i >= sizeof(buf)) {
      g_printerr(PACKAGE_NAME ": filename in sigfile too long: %20s\n", sig);
      return 1; 
    }

    memcpy(buf, sig + 34, i);
    buf[i] = 0; /* terminate */

    g_printerr(PACKAGE_NAME ": verifying hash of %s: ", buf);
    if(taxbird_digest_verify_file(buf, taxbird_digest_hexdecode(sig))) {
      g_printerr("FAILED!\n");
      return 1;
    }

    g_printerr("valid.\n");

    /* forward beyond end of current line */
    sig = ptr + 1; 
  }

  return 0; /* valid. */

 out_syntax_err:
  g_printerr(PACKAGE_NAME ": syntax error in .sig file, near %20s\n", sig);
  return 1; /* error exit, don't know whether all hashes match */
}
