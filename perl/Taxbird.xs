/*
 * Copyright (C) 2007  Stefan Siegl <stesie@brokenpipe.de>, Germany
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

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

#include <opensc/pkcs15.h>

MODULE = Taxbird  PACKAGE = Taxbird




SV *
get_opensc_certificates()
    INIT:
	
    CODE:
	struct sc_pkcs15_card *p15card = NULL;
	sc_card_t *card = NULL;

	RETVAL = &PL_sv_undef;

	/* initialize OpenSC context */
   	sc_context_t *ctx = NULL;
	sc_context_param_t ctx_param;
	memset(&ctx_param, 0, sizeof(ctx_param));
	ctx_param.ver      = 0;
	ctx_param.app_name = "Taxbird";

	int r = sc_context_create(&ctx, &ctx_param);
	if (r) {
		fprintf(stderr, "Failed to establish context: %s\n", 
		        sc_strerror(r));
		goto out;
	}

	if (sc_ctx_get_reader_count(ctx) == 0) {
		fprintf(stderr, "No smart card readers configured.\n");
		goto out;
	}
   
	/* connect to card */
   	sc_reader_t *reader = sc_ctx_get_reader(ctx, 0);
	if (sc_detect_card_presence(reader, 0) <= 0) {
		fprintf(stderr, "No card present.\n");
		goto out;
	}

	int slot_id = 0;
	if ((r = sc_connect_card(reader, slot_id, &card)) < 0) {
		fprintf(stderr, "Failed to connect to card: %s\n",
			sc_strerror(r));
		goto out;
	}

	if ((r = sc_lock(card)) < 0) {
		fprintf(stderr, "Failed to lock card: %s\n", 
			sc_strerror(r));
		goto out_disconnect;
	}

	r = sc_pkcs15_bind(card, &p15card);
	if (r) {
		fprintf(stderr, "PKCS#15 initialization failed: %s\n", 
		        sc_strerror(r));
		goto out;
	}
   
	/*
	 * okay, card is ready to use now, let's the certificate list
	 */
   	struct sc_pkcs15_object *objs[32];
	r = sc_pkcs15_get_objects(p15card, SC_PKCS15_TYPE_CERT_X509, objs, 32);
	if (r < 0) {
		fprintf(stderr, "Certificate enumeration failed: %s\n", 
			sc_strerror(r));
		goto out;
	}

	/* create the hash list to return ... */
	HV *certlist = newHV();

	int i;
	for (i = 0; i < r; i ++) {
		const char *lbl = (objs[i])->label;
		struct sc_pkcs15_cert_info *cert = 
		        (struct sc_pkcs15_cert_info *) (objs[i])->data;
		struct sc_pkcs15_id *id = &cert->id;

		if(id->len > 1)
			fprintf(stderr, "unexpected id length %d, "
			        "skipping certificate.\n", id->len);
		else
			hv_store(certlist, lbl, strlen(lbl), 
				 newSViv(id->value[0]), 0);
	}

	RETVAL = newRV_noinc((SV*) certlist);
	/* sv_2mortal((SV*) RETVAL); */

 out:
	if (p15card)
		sc_pkcs15_unbind(p15card);

	if (card) {
		sc_unlock(card);
 out_disconnect:
		sc_disconnect_card(card, 0);
	
	}
	if (ctx)
		sc_release_context(ctx);

    OUTPUT:
	RETVAL


