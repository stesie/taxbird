/* Copyright(C) 2004,05 Stefan Siegl <ssiegl@gmx.de>
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
#include <libguile.h>
#include <string.h>
#include <glib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <gpgme.h>

#include "guile.h"
#include "form.h"

static SCM taxbird_guile_eval_file_SCM(SCM scm_fn);
static SCM taxbird_guile_check_sig_SCM(SCM scm_fn);

/* initialize taxbird's guile backend */
void taxbird_guile_init(void)
{
  char *loadpath_home = g_strdup_printf("%s/.taxbird/guile", getenv("HOME"));
  SCM loadpath = scm_list_3(scm_makfrom0str("."),
			    scm_take0str(loadpath_home),
			    scm_makfrom0str(PACKAGE_DATA_DIR "/taxbird/guile"));

  /* search current and taxbird's system directory by default */
  scm_c_define("tb:scm-directories", loadpath);

  scm_c_define("tb:field:text-input", scm_int2num(FIELD_TEXT_INPUT));
  scm_c_define("tb:field:text-output", scm_int2num(FIELD_TEXT_OUTPUT));
  scm_c_define("tb:field:chooser", scm_int2num(FIELD_CHOOSER));
  scm_c_define("tb:field:text-input-calc", scm_int2num(FIELD_TEXT_INPUT_CALC));
  scm_c_define("tb:field:text-input-input",
	       scm_int2num(FIELD_TEXT_INPUT_INPUT));

  scm_c_define_gsubr("tb:eval-file", 1, 0, 0, taxbird_guile_eval_file_SCM);
  scm_c_define_gsubr("tb:check-sig", 1, 0, 0, taxbird_guile_check_sig_SCM);
  scm_c_define_gsubr("tb:form-register", 6, 0, 0, taxbird_form_register);

  /* Scan autoload/ directories for files, that should be loaded automatically.
   * However don't load each file from these directories in order, but 
   * call taxbird_guile_eval_file to always evaluate the first file in the
   * loadpath chain, i.e. allow the user to overwrite autoload/ files by
   * putting hisself's in his private directory.
   */
  while(scm_ilength(loadpath)) {
    struct dirent *dirent;
    char *dirname = g_strdup_printf("%s/autoload/",
				    SCM_STRING_CHARS(SCM_CAR(loadpath)));
    DIR *dir = opendir(dirname);

    if(dir) {
      while((dirent = readdir(dir))) {
	char *fname;

	if(*dirent->d_name == '.') continue; /* don't autoload hidden files
					      * (and '.' or '..' directory) */

	fname = g_strdup_printf("autoload/%s", dirent->d_name);
	taxbird_guile_eval_file(fname);
	g_free(fname);
      }

      closedir(dir);
      g_free(dirname);
    }

    loadpath = SCM_CDR(loadpath);
  }
}



char *
taxbird_guile_dirlist_lookup(const char *fn) 
{
  SCM dirlist = scm_c_lookup_ref("tb:scm-directories");

  while(scm_ilength(dirlist)) {
    char *buf;
    struct stat statbuf;
    SCM path = SCM_CAR(dirlist);
   
    g_return_val_if_fail(SCM_STRINGP(path), NULL);

    buf = g_strdup_printf("%s/%s", SCM_STRING_CHARS(path), fn);
    if(! buf) {
      perror(PACKAGE_NAME);
      return NULL; /* out of memory */
    }

    if(! stat(buf, &statbuf)
       && ! S_ISDIR(statbuf.st_mode))
      /* not a directory ... */
      return buf;

    g_free(buf);

    /* next element */
    dirlist = SCM_CDR(dirlist);
  }

  return NULL;
}



/* evaluate the file with the given filename */
int
taxbird_guile_eval_file(const char *fn)
{
  char *lookup_fn;

  /* make sure that every file is evaluated only once */
  static GSList *evaluated_files = NULL;
  if(evaluated_files) {
    GSList *ptr = evaluated_files;
    do {
      if(! strcmp(ptr->data, fn))
	return 0;
    } while((ptr = ptr->next));
  }

  /* file hasn't been evaluated yet, add to list of evaluated files. */
  evaluated_files = g_slist_prepend(evaluated_files, g_strdup(fn));

  lookup_fn = taxbird_guile_dirlist_lookup(fn);
  if(! lookup_fn) {
    g_printerr(PACKAGE_NAME ": cannot find file: %s\n", fn);
    return -1;
  }

  g_printerr(PACKAGE_NAME ": loading '%s'\n", lookup_fn);
  scm_c_primitive_load(lookup_fn);
  g_free(lookup_fn);

  return 0;
}



static SCM 
taxbird_guile_eval_file_SCM(SCM scm_fn)
{
  g_return_val_if_fail(SCM_STRINGP(scm_fn), SCM_BOOL(0));
  return SCM_BOOL(! taxbird_guile_eval_file(SCM_STRING_CHARS(scm_fn)));
}



/* verify signature file's signature and verify referenced file's md5 hashes */
char *
taxbird_guile_check_sig(const char *fn)
{
  gpgme_ctx_t gpg_ctx;
  gpgme_data_t dh_file2gpg, dh_gpg2taxbird;
  gpgme_error_t gpg_error;
  gpgme_verify_result_t gpg_result;
  gpgme_key_t gpg_key;
  size_t content_len;
  char *content;
  char *retval = NULL;
  char *vendid; /* test vendor id */
  char *lookup_fn = taxbird_guile_dirlist_lookup(fn);

  if(! lookup_fn) {
    g_printerr(PACKAGE_NAME ": cannot find signature file: %s\n", fn);
    goto out;
  }

  if(gpgme_data_new_from_file(&dh_file2gpg, lookup_fn, 1) != GPG_ERR_NO_ERROR){
    g_printerr(PACKAGE_NAME ": unable to open data handle to gpg.\n");
    goto out2;
  }

  if(gpgme_new(&gpg_ctx) != GPG_ERR_NO_ERROR) {
    g_printerr(PACKAGE_NAME ": cannot allocate gpgme context.\n");
    goto out3;
  }

  if(gpgme_data_new(&dh_gpg2taxbird) != GPG_ERR_NO_ERROR) {
    g_printerr(PACKAGE_NAME ": unable to open in-memory handle from gpg.\n");
    goto out4;
  }

  gpg_error = gpgme_op_verify(gpg_ctx, dh_file2gpg, NULL, dh_gpg2taxbird);
  if(gpg_error != GPG_ERR_NO_ERROR) {
    g_printerr(PACKAGE_NAME ": cannot verify gpg signature. sorry.\n");
    goto out5;
  }

  if(!(gpg_result = gpgme_op_verify_result(gpg_ctx))) {
    g_printerr(PACKAGE_NAME ": cannot verify gpg signature. sorry.\n");
    goto out5;
  }

  if(gpg_result->signatures->summary
     & ~(GPGME_SIGSUM_VALID | GPGME_SIGSUM_GREEN)) {
    g_printerr(PACKAGE_NAME ": signature of %s is not valid.\n", fn);
    goto out5;
  }

  /* okay, signature is valid. extract vendor-id now ... */
  if(gpgme_get_key(gpg_ctx, gpg_result->signatures->fpr, &gpg_key, 0)
     != GPG_ERR_NO_ERROR || !gpg_key) {
    g_printerr(PACKAGE_NAME ": cannot find public key in gpg keyring.\n");
    goto out5;
  }

  if(strlen(gpg_key->uids->comment) != 13 
     || strncmp(gpg_key->uids->comment, "Taxbird:", 8)) {
    g_printerr(PACKAGE_NAME ": invalid gpg-key comment: '%s'.\n", 
	       gpg_key->uids->comment);
    goto out5;
  }

  vendid = g_strdup(gpg_key->uids->comment + 8);
  g_printerr(PACKAGE_NAME ": signature on '%s' seems valid; vendor-id: %5s.\n",
	     fn, vendid);

  /* now check md5 signatures ... */
  content = gpgme_data_release_and_get_mem(dh_gpg2taxbird, &content_len);
  content = realloc(content, content_len + 1);
  if(! content) goto out4; 
  content[content_len] = 0;

  if(! taxbird_digest_verify(content))
    retval = vendid;

  free(content);
  goto out4; /* gpgme data handle already released */

 out5:
  gpgme_data_release(dh_gpg2taxbird);
 out4:
  gpgme_release(gpg_ctx);
 out3:
  gpgme_data_release(dh_file2gpg);
 out2:
  g_free(lookup_fn);
 out:
  return retval;
}



static SCM
taxbird_guile_check_sig_SCM(SCM scm_fn)
{
  char *result;

  g_return_val_if_fail(SCM_STRINGP(scm_fn), SCM_BOOL(0));

  result = taxbird_guile_check_sig(SCM_STRING_CHARS(scm_fn));
  return (result ? scm_take0str(result) : SCM_BOOL(0));
}
