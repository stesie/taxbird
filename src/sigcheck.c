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

#include <gnome.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include "sigcheck.h"
#include "dialog.h"
#include "guile.h"
#include "digest.h"

#include <openssl/pkcs7.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/evp.h>
#include <openssl/err.h>

static char *taxbird_sigcheck_get_id(const char *filename);

static X509_STORE *taxbird_sigcheck_make_empty_store(void);

/* verify signature file's signature and verify referenced file's md5
 * hashes afterwards.
 * 
 * RETURN: 0 on success, anything else on failure */
int 
taxbird_sigcheck(const char *fn, char **vendor_id, char **sig_id)
{
  int err = 0;
  char *err_msg = NULL;

  g_return_val_if_fail(fn, 1);

  /* look for the real file **************************************************/
  char *lookup_fn = taxbird_guile_dirlist_lookup(fn);
  if(! lookup_fn) {
    err_msg = _("Unable to find required signature file '%s' in "
		"the Taxbird path.");
    err = 1;
    goto out;
  }



  /* extract signature $Id: ************************************************/
  
  if(! (*sig_id = taxbird_sigcheck_get_id(lookup_fn))) {
    err_msg = _("Signature's $Id: entry is not valid, "
		"while checking signature %s");
    err = 1;
    goto out;
  }



  OpenSSL_add_all_digests();

  /* now verify the signature and it's contents itself *********************/
  BIO *in = BIO_new_file(lookup_fn, "r");

  if(! in) {
    err_msg = _("Unable to open signature file %s to do verification");
    err = 2;
    goto out;
  }

  BIO *indata = NULL;
  PKCS7 *p7 = SMIME_read_PKCS7(in, &indata);
  if(! p7) {
    err_msg = _("Unable to parse S/MIME signature %s, to do verification");
    err = 2;
    goto out2;
  }

  BIO *out = BIO_new(BIO_s_mem());
  if(! out) {
    err_msg = _("Unable to allocate BIO buffer, while trying to verify %s");
    err = 2;
    goto out3;
  }

  X509_STORE *store = taxbird_sigcheck_make_empty_store();
  if(! store) {
    err_msg = _("Unable to allocate X.509 store, trying to verify %s");
    err = 2;
    goto out3b;
  }

  ERR_clear_error();
  if(! PKCS7_verify(p7, NULL, store, indata, out,
		    PKCS7_NOVERIFY)) {
    err_msg = _("The S/MIME signature on %s is not valid, sorry. Please "
		"try to install an unmodified version.");
    err = 2;
    goto out4;
  }
  
  BUF_MEM *content;
  BIO_get_mem_ptr(out, &content);

  /* now check md5 signatures ... *******************************************/
  if(taxbird_digest_verify(content->data, content->length)) {
    err_msg = _("The MD5-Checksums from the signature file %s do "
		"not match the corresponding checksums of the installed "
		"source files. Sorry, you either want "
		"to self-sign your modified sources or want to "
		"install unmodified, original ones.\n\n"
		"If you've found a bug in one in the Guile sources, please "
		"don't hesitate but publish your solution to the "
		"taxbird@taxbird.de mailing list.");
    err = 1;
    goto out4;
  }


  /* finally extract vendor-id from used X.509 certificate ******************/
  STACK_OF(X509) *stack = NULL;
  if(! (stack = PKCS7_get0_signers(p7, NULL, PKCS7_NOVERIFY))) {
    err_msg = _("Unable to read X.509 certificate from signature %s.");
    err = 2; /* OpenSSL error */
    goto out4;
  }

  X509 *cert = sk_X509_value(stack, 0);
  X509_NAME *name = X509_get_issuer_name(cert);
  char *buf = X509_NAME_oneline(name, 0, 0), *p;

  if((p = strstr(buf, "/OU=")) && (p += 4) && p[5] == '/') {
    p[5] = 0;

    if(! (*vendor_id = strdup(p))) {
      err_msg = _("Not enough memory to store vendor id from signature %s.");
      err = 1;
    }
  } else {
    err_msg = _("Unable to extract vendor id from issuer name (signatur %s)");
    err = 1;
  }
  
  if(X509_cmp_current_time(X509_get_notAfter(cert)) < 0)
    err_msg = _("The S/MIME signature on %s is valid (the "
		"installed sources haven't been modified) "
		"but the X.509 certificate has expired. "
		"This is, your Taxbird installation is rather "
		"old and probably has been updated "
		"meanwhile.\n\n"
		"Please have a look at www.taxbird.de and "
		"look for an update, maybe consider "
		"contacting the mailing list.");
  
  OPENSSL_free(buf);
  sk_X509_free(stack);

 out4:
  X509_STORE_free(store);

 out3b:
  BIO_free(out);

 out3: 
  PKCS7_free(p7);

 out2:
  BIO_free(in);

 out:
  g_free(lookup_fn);

  if(err_msg) {
    err_msg = g_strdup_printf(err_msg, fn);

    if(err)
      taxbird_dialog_error(NULL, err_msg);
    else
      taxbird_dialog_info(NULL, err_msg);

    g_free(err_msg);
  }

  if(err == 2) {
    /* OpenSSL error, emit detailed error message to stderr */
    ERR_load_BIO_strings();
    ERR_load_PKCS7_strings();
    ERR_print_errors_fp(stderr);
  }

  return err;
}



static char *
taxbird_sigcheck_get_id(const char *filename) 
{
  FILE *handle = fopen(filename, "r");
  if(! handle) return NULL;
 
  unsigned char buf[256];
  while(fgets(buf, sizeof(buf), handle)) 
    if(! strncmp("Taxbird-Id: $Id: ", buf, 17)) {
      fclose(handle);
      
      unsigned char *p = strchr(buf + 17, '$');
      if(! p) return NULL;
      
      *p = 0; /* \0-terminate $Id: string */
      return g_strdup(buf + 17); 
    }
 
  fclose(handle);
  return NULL;
}



static X509_STORE *
taxbird_sigcheck_make_empty_store(void)
{
  X509_LOOKUP *lookup;
  X509_STORE *new_store = X509_STORE_new();
  if(! new_store) return NULL;

  lookup = X509_STORE_add_lookup(new_store, X509_LOOKUP_file());
  if(! lookup) goto end;
  X509_LOOKUP_load_file(lookup, NULL, X509_FILETYPE_DEFAULT);

  lookup = X509_STORE_add_lookup(new_store, X509_LOOKUP_hash_dir());
  if(! lookup) goto end;
  X509_LOOKUP_add_dir(lookup, NULL, X509_FILETYPE_DEFAULT);

  X509_STORE_set_flags(new_store, 0);
  return new_store;

 end:
  X509_STORE_free(new_store);
  return NULL;
}



