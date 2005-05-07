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

/* verify signature on file FILE and extract vendor-id and contents on
 * success (returning 0),  on failure return 1 */
static int taxbird_sigcheck_verify(const char *filename, 
				   char **vendor_id, char **output);

static char *taxbird_sigcheck_get_id(const char *filename);


/* verify signature file's signature and verify referenced file's md5
 * hashes afterwards.
 * 
 * RETURN: 0 on success, 1 on failure */
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



  /* extract signature $Id: sigcheck.c,v 1.5 2005-05-07 20:08:32 stesie Exp $ ************************************************/
  
  if(! (*sig_id = taxbird_sigcheck_get_id(lookup_fn))) {
    err_msg = _("Signature's $Id: entry is not valid, "
		"while checking signature %s");
    err = 1;
    goto out;
  }



  /* verify the signature on the signature file ******************************/
  char *content;
  if(taxbird_sigcheck_verify(lookup_fn, vendor_id, &content)) {
    err_msg = NULL;
    err = 1;
    goto out;
  }



  /* now check md5 signatures ... *******************************************/
  if(taxbird_digest_verify(content)) {
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


 out4:
  g_free(content);

 out:
  g_free(lookup_fn);

  if(err_msg) {
    err_msg = g_strdup_printf(err_msg, fn);
    taxbird_dialog_error(NULL, err_msg);
    g_free(err_msg);
  }

  return err;
}



static char *
taxbird_sigcheck_get_id(const char *filename) 
{
  int fd = open(filename, O_RDONLY);
  if(fd < 0) return NULL;

  unsigned char buf[128];
  ssize_t chars_read = read(fd, buf, sizeof(buf));

  if(chars_read < 0) return NULL;
  buf[chars_read < sizeof(buf) ? chars_read : sizeof(buf) - 1] = 0;

  if(strncmp("$Id: ", buf, 5)) return NULL;

  unsigned char *p = strchr(buf + 5, '$');
  if(! p) return NULL;

  *p = 0; /* \0-terminate $Id: string */

  return g_strdup(buf + 5); 
}


/* verify signature on file FILE and extract vendor-id and contents on
 * success (returning 0),  on failure return 1 */
static int
taxbird_sigcheck_verify(const char *filename, char **vendor_id, char **output)
{
  int retry = 0;
  int err = 0;
  char *msg = NULL;

  gpgme_data_t file;
  if(gpgme_data_new_from_file(&file, filename, 1) != GPG_ERR_NO_ERROR){
    msg = _("Unable to open signature file %s.");
    err = 1;
    goto out2;
  }

  gpgme_ctx_t gpg_ctx;
  if(gpgme_new(&gpg_ctx) != GPG_ERR_NO_ERROR) {
    err = 1;
    goto out3;
  }

  gpgme_data_t content;
  if(gpgme_data_new(&content) != GPG_ERR_NO_ERROR) {
    err = 1;
    goto out4;
  }



  /* well, everything's prepared, call gpg now, to verify the signature ******/
  gpgme_error_t gpg_error = gpgme_op_verify(gpg_ctx, file, NULL, content);
  if(gpg_error != GPG_ERR_NO_ERROR) {
    err = 1;
    goto out5;
  }

  gpgme_verify_result_t gpg_result;
  if(!(gpg_result = gpgme_op_verify_result(gpg_ctx))) {
    err = 1;
    goto out5;
  }



  /* check the signature's status, possibly choosing a suitable error mesg. */
  gpgme_key_t gpg_key;
  if((gpg_result->signatures->summary &
      ~(GPGME_SIGSUM_VALID | GPGME_SIGSUM_GREEN))
     || (gpgme_get_key(gpg_ctx, gpg_result->signatures->fpr, &gpg_key, 0)
	 != GPG_ERR_NO_ERROR)
     || ! gpg_key) {

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
	      "Please make sure GPG is able to verify "
	      "digital signatures.\n");
    }

    err = 1;
    goto out5;
  }



  /* try to extract the key's comment */
  if(strlen(gpg_key->uids->comment) != 13 
     || strncmp(gpg_key->uids->comment, "Taxbird:", 8)) {
    char *msg = g_strdup_printf(_("Invalid comment on gpg-key '%s'. "
				  "Checking signature file %s."),
				gpg_key->uids->comment, filename);
    taxbird_dialog_error(NULL, msg);
    goto out5;
  }



  /* copy the vendor id back to our callee */
  *vendor_id = g_strdup(gpg_key->uids->comment + 8);



  /* finally copy the signed content itself */
  size_t content_len;
  *output = gpgme_data_release_and_get_mem(content, &content_len);
  *output = realloc(*output, content_len + 1);
  if(! content) {
    err = 1;
    goto out4; 
  }

  (*output)[content_len] = 0;

  goto out4; /* gpgme data handle already released */

 out5:
  gpgme_data_release(content);

 out4:
  gpgme_release(gpg_ctx);

 out3:
  gpgme_data_release(file);

 out2:
  
  if(retry)
    return taxbird_sigcheck_verify(filename, vendor_id, output);
  
  if(err) {
    if(! msg)
      msg = _("Unable to check the validity of the signature file %s.");

    msg = g_strdup_printf(msg, filename);
    taxbird_dialog_error(NULL, msg);
    g_free(msg);
  }
  
  return err;
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

  if(chdir(PACKAGE_DATA_DIR "/taxbird/pubkeys")) return;

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
