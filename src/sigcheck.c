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
#include <gpgme.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include "sigcheck.h"
#include "dialog.h"
#include "guile.h"

/* import all keys available from the public-keys directory */
static void taxbird_sigcheck_import_keys(void);

/* import the public keys stored in the file with the provided name */
static void taxbird_sigcheck_import_key(const char *fname);


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
  int retry = 0; /* whether to automatically call this function again */

  if(! lookup_fn) {
    char *msg = g_strdup_printf(_("Unable to find required signature "
				  "file '%s' in the Taxbird path."), fn);
    taxbird_dialog_error(NULL, msg);
    g_free(msg); 

    goto out;
  }

  if(gpgme_data_new_from_file(&dh_file2gpg, lookup_fn, 1) != GPG_ERR_NO_ERROR){
    taxbird_dialog_error(NULL, _("Unable to convert handle of signature "
				 "file to a libgpgme data handle.")); 
    goto out2;
  }

  if(gpgme_new(&gpg_ctx) != GPG_ERR_NO_ERROR) {
    taxbird_dialog_error(NULL, _("Cannot allocate libgpgme context, "
				 "sorry."));
    goto out3;
  }

  if(gpgme_data_new(&dh_gpg2taxbird) != GPG_ERR_NO_ERROR) {
    taxbird_dialog_error(NULL, _("Unable to open in-memory handle from "
				 "GPG, using libgpgme."));
    goto out4;
  }

  gpg_error = gpgme_op_verify(gpg_ctx, dh_file2gpg, NULL, dh_gpg2taxbird);
  if(gpg_error != GPG_ERR_NO_ERROR) {
    taxbird_dialog_error(NULL, _("GPG signature is not valid. Sorry.\n"));
    goto out5;
  }

  if(!(gpg_result = gpgme_op_verify_result(gpg_ctx))) {
    taxbird_dialog_error(NULL, _("GPG signature is not valid. Sorry.\n"));
    goto out5;
  }

  if((gpg_result->signatures->summary &
      ~(GPGME_SIGSUM_VALID | GPGME_SIGSUM_GREEN))
     || (gpgme_get_key(gpg_ctx, gpg_result->signatures->fpr, &gpg_key, 0)
	 != GPG_ERR_NO_ERROR)
     || ! gpg_key) {
    char *msg = NULL;

    if(gpg_result->signatures->summary & GPGME_SIGSUM_KEY_REVOKED)
      msg = _("The Taxbird signature (%s) is not valid anymore. "
	      "It's public key has been revoked. \n\n"
	      "Please use taxbird-update.sh to update your installation.");

    else if(gpg_result->signatures->summary & GPGME_SIGSUM_KEY_EXPIRED)
      msg = _("The Taxbird signature on %s is not valid anymore. "
	      "It's public key has expired.\n\n"
	      "Please update your taxbird installation, using "
	      "taxbird-update.sh");

    else if(gpg_result->signatures->summary & GPGME_SIGSUM_SIG_EXPIRED)
      msg = _("The Taxbird signature on %s has expired. \n\n"
	      "Please use taxbird-update.sh to update your Taxbird "
	      "installation.");

    else if(gpg_result->signatures->summary & GPGME_SIGSUM_KEY_MISSING) {
      static int gpg_keys_imported = 0;
      if(! gpg_keys_imported && ++ gpg_keys_imported) {
	/* automatically try to import all available gpg keys,
	 * and try once more ... */
	taxbird_sigcheck_import_keys();
	retry = 1;
	goto out5;
      }

      msg = _("Key for signature file %s is not available. "
	      "Please make sure gpg is able to verify "
	      "digital signatures.\n");
    }

    else
      msg = _("Unable to verify signature of %s");

    msg = g_strdup_printf(msg, fn);
    taxbird_dialog_error(NULL, msg);
    g_free(msg);

    goto out5;
  }

  if(strlen(gpg_key->uids->comment) != 13 
     || strncmp(gpg_key->uids->comment, "Taxbird:", 8)) {
    char *msg = g_strdup_printf(_("Invalid comment on gpg-key '%s'. "
				  "Checking signature file %s."),
				gpg_key->uids->comment, fn);
    taxbird_dialog_error(NULL, msg);
    goto out5;
  }

  vendid = g_strdup(gpg_key->uids->comment + 8);

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
  return retry ? taxbird_guile_check_sig(fn) : retval;
}



static void
taxbird_sigcheck_import_keys(void)
{
#ifdef HAVE_GET_CURRENT_DIR_NAME
  char *cwd = get_current_dir_name();
  if(! cwd) return; /* damn, shall we complain? */
#else
  int cwd = open(".", O_RDONLY);
  if(cwd < 0) return;
#endif /* ... ! HAVE_GET_CURRENT_DIR_NAME */

  chdir(PACKAGE_DATA_DIR "/taxbird/pubkeys");

  DIR *dir = opendir(".");
  if(dir) {
    struct dirent *dirent;

    while((dirent = readdir(dir)))
      if(dirent->d_name[0] != '.') /* don't import . and .. (dirs) and
				    * hidden files */
	taxbird_sigcheck_import_key(dirent->d_name);

    closedir(dir);
  }


#ifdef HAVE_GET_CURRENT_DIR_NAME
  chdir(cwd);
  free(cwd);
#else
  fchdir(cwd);
  close(cwd);
#endif /* ... ! HAVE_GET_CURRENT_DIR_NAME */
}



/* import the public keys stored in the file with the provided name */
static void
taxbird_sigcheck_import_key(const char *fname)
{
  int err = 0;
  gpgme_data_t keyfile;
  gpgme_ctx_t ctx;

  if(gpgme_data_new_from_file(&keyfile, fname, 1) != GPG_ERR_NO_ERROR) {
    err = 1;
    goto out;
  }

  if(gpgme_new(&ctx) != GPG_ERR_NO_ERROR) {
    err = 1;
    goto out2;
  }

  if(gpgme_op_import(ctx, keyfile) != GPG_ERR_NO_ERROR) {
    err = 1;
    goto out3;
  }

 out3:
  gpgme_release(ctx);

 out2:
  gpgme_data_release(keyfile);

 out:
  if(err) {
    char *msg = g_strdup_printf(_("The GPG public key file '%s' could "
				  "not be imported successfully. You "
				  "will not be able to use the signature "
				  "mechanism this way."), fname);
    taxbird_dialog_error(NULL, msg);
    g_free(msg);
  }
  else {
    g_printerr(PACKAGE_NAME ": gpg public keys from '%s' "
	       "successfully imported\n", fname);
  }
}
