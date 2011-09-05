/* fips/rand/fips_drbg_selftest.c */
/* Written by Dr Stephen N Henson (steve@openssl.org) for the OpenSSL
 * project.
 */
/* ====================================================================
 * Copyright (c) 2011 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.OpenSSL.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    licensing@OpenSSL.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.OpenSSL.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 */

#define OPENSSL_FIPSAPI

#include <string.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/fips_rand.h>
#include "fips_rand_lcl.h"
#include "fips_locl.h"

#include "fips_drbg_selftest.h"

typedef struct {
	int post;
	int nid;
	unsigned int flags;

	/* KAT data for no PR */
	const unsigned char *ent;
	size_t entlen;
	const unsigned char *nonce;
	size_t noncelen;
	const unsigned char *pers;
	size_t perslen;
	const unsigned char *adin;
	size_t adinlen;
	const unsigned char *entreseed;
	size_t entreseedlen;
	const unsigned char *adinreseed;
	size_t adinreseedlen;
	const unsigned char *adin2;
	size_t adin2len;
	const unsigned char *kat;
	size_t katlen;
	const unsigned char *kat2;
	size_t kat2len;

	/* KAT data for PR */
	const unsigned char *ent_pr;
	size_t entlen_pr;
	const unsigned char *nonce_pr;
	size_t noncelen_pr;
	const unsigned char *pers_pr;
	size_t perslen_pr;
	const unsigned char *adin_pr;
	size_t adinlen_pr;
	const unsigned char *entpr_pr;
	size_t entprlen_pr;
	const unsigned char *ading_pr;
	size_t adinglen_pr;
	const unsigned char *entg_pr;
	size_t entglen_pr;
	const unsigned char *kat_pr;
	size_t katlen_pr;
	const unsigned char *kat2_pr;
	size_t kat2len_pr;

	} DRBG_SELFTEST_DATA;

#define make_drbg_test_data(nid, flag, pr, p) {p, nid, flag | DRBG_FLAG_TEST, \
	pr##_entropyinput, sizeof(pr##_entropyinput), \
	pr##_nonce, sizeof(pr##_nonce), \
	pr##_personalizationstring, sizeof(pr##_personalizationstring), \
	pr##_additionalinput, sizeof(pr##_additionalinput), \
	pr##_entropyinputreseed, sizeof(pr##_entropyinputreseed), \
	pr##_additionalinputreseed, sizeof(pr##_additionalinputreseed), \
	pr##_additionalinput2, sizeof(pr##_additionalinput2), \
	pr##_int_returnedbits, sizeof(pr##_int_returnedbits), \
	pr##_returnedbits, sizeof(pr##_returnedbits), \
	pr##_pr_entropyinput, sizeof(pr##_pr_entropyinput), \
	pr##_pr_nonce, sizeof(pr##_pr_nonce), \
	pr##_pr_personalizationstring, sizeof(pr##_pr_personalizationstring), \
	pr##_pr_additionalinput, sizeof(pr##_pr_additionalinput), \
	pr##_pr_entropyinputpr, sizeof(pr##_pr_entropyinputpr), \
	pr##_pr_additionalinput2, sizeof(pr##_pr_additionalinput2), \
	pr##_pr_entropyinputpr2, sizeof(pr##_pr_entropyinputpr2), \
	pr##_pr_int_returnedbits, sizeof(pr##_pr_int_returnedbits), \
	pr##_pr_returnedbits, sizeof(pr##_pr_returnedbits), \
	}

#define make_drbg_test_data_df(nid, pr, p) \
	make_drbg_test_data(nid, DRBG_FLAG_CTR_USE_DF, pr, p)

static DRBG_SELFTEST_DATA drbg_test[] = {
	make_drbg_test_data_df(NID_aes_128_ctr, aes_128_use_df, 0),
	make_drbg_test_data_df(NID_aes_192_ctr, aes_192_use_df, 0),
	make_drbg_test_data_df(NID_aes_256_ctr, aes_256_use_df, 1),
	make_drbg_test_data(NID_aes_128_ctr, 0, aes_128_no_df, 0),
	make_drbg_test_data(NID_aes_192_ctr, 0, aes_192_no_df, 0),
	make_drbg_test_data(NID_aes_256_ctr, 0, aes_256_no_df, 1),
	make_drbg_test_data(NID_sha1, 0, sha1, 0),
	make_drbg_test_data(NID_sha224, 0, sha224, 0),
	make_drbg_test_data(NID_sha256, 0, sha256, 1),
	make_drbg_test_data(NID_sha384, 0, sha384, 0),
	make_drbg_test_data(NID_sha512, 0, sha512, 0),
	make_drbg_test_data(NID_hmacWithSHA1, 0, hmac_sha1, 0),
	make_drbg_test_data(NID_hmacWithSHA224, 0, hmac_sha224, 0),
	make_drbg_test_data(NID_hmacWithSHA256, 0, hmac_sha256, 1),
	make_drbg_test_data(NID_hmacWithSHA384, 0, hmac_sha384, 0),
	make_drbg_test_data(NID_hmacWithSHA512, 0, hmac_sha512, 0),
	{0,0,0}
	};

typedef struct 
	{
	const unsigned char *ent;
	size_t entlen;
	int entcnt;
	const unsigned char *nonce;
	size_t noncelen;
	int noncecnt;
	} TEST_ENT;

static size_t test_entropy(DRBG_CTX *dctx, unsigned char **pout,
                                int entropy, size_t min_len, size_t max_len)
	{
	TEST_ENT *t = FIPS_drbg_get_app_data(dctx);
	*pout = (unsigned char *)t->ent;
	t->entcnt++;
	return t->entlen;
	}

static size_t test_nonce(DRBG_CTX *dctx, unsigned char **pout,
                                int entropy, size_t min_len, size_t max_len)
	{
	TEST_ENT *t = FIPS_drbg_get_app_data(dctx);
	*pout = (unsigned char *)t->nonce;
	t->noncecnt++;
	return t->noncelen;
	}

static int fips_drbg_single_kat(DRBG_CTX *dctx, DRBG_SELFTEST_DATA *td)
	{
	TEST_ENT t;
	int rv = 0;
	size_t adinlen;
	unsigned char randout[1024];

	/* Initial test without PR */

	if (!FIPS_drbg_init(dctx, td->nid, td->flags))
		return 0;
	if (!FIPS_drbg_set_callbacks(dctx, test_entropy, 0, 0, test_nonce, 0))
		return 0;

	FIPS_drbg_set_app_data(dctx, &t);

	t.ent = td->ent;
	t.entlen = td->entlen;
	t.nonce = td->nonce;
	t.noncelen = td->noncelen;
	t.entcnt = 0;
	t.noncecnt = 0;

	if (!FIPS_drbg_instantiate(dctx, td->pers, td->perslen))
		goto err;

	/* Note for CTR without DF some additional input values
	 * ignore bytes after the keylength: so reduce adinlen
	 * to half to ensure invalid data is fed in.
	 */
	if (!fips_post_corrupt(FIPS_TEST_DRBG, dctx->type, &dctx->flags))
		adinlen = td->adinlen / 2;
	else
		adinlen = td->adinlen;
	if (!FIPS_drbg_generate(dctx, randout, td->katlen, 0, 0,
				td->adin, adinlen))
		goto err;

	if (memcmp(randout, td->kat, td->katlen))
		goto err;

	t.ent = td->entreseed;
	t.entlen = td->entreseedlen;

	if (!FIPS_drbg_reseed(dctx, td->adinreseed, td->adinreseedlen))
		goto err;

	if (!FIPS_drbg_generate(dctx, randout, td->kat2len, 0, 0,
				td->adin2, td->adin2len))
		goto err;

	if (memcmp(randout, td->kat2, td->kat2len))
		goto err;

	FIPS_drbg_uninstantiate(dctx);

	/* Now test with PR */
	if (!FIPS_drbg_init(dctx, td->nid, td->flags))
		return 0;
	if (!FIPS_drbg_set_callbacks(dctx, test_entropy, 0, 0, test_nonce, 0))
		return 0;

	FIPS_drbg_set_app_data(dctx, &t);

	t.ent = td->ent_pr;
	t.entlen = td->entlen_pr;
	t.nonce = td->nonce_pr;
	t.noncelen = td->noncelen_pr;
	t.entcnt = 0;
	t.noncecnt = 0;

	if (!FIPS_drbg_instantiate(dctx, td->pers_pr, td->perslen_pr))
		goto err;

	t.ent = td->entpr_pr;
	t.entlen = td->entprlen_pr;

	/* Note for CTR without DF some additional input values
	 * ignore bytes after the keylength: so reduce adinlen
	 * to half to ensure invalid data is fed in.
	 */
	if (!fips_post_corrupt(FIPS_TEST_DRBG, dctx->type, &dctx->flags))
		adinlen = td->adinlen_pr / 2;
	else
		adinlen = td->adinlen_pr;
	if (!FIPS_drbg_generate(dctx, randout, td->katlen_pr, 0, 1,
				td->adin_pr, adinlen))
		goto err;

	if (memcmp(randout, td->kat_pr, td->katlen_pr))
		goto err;

	t.ent = td->entg_pr;
	t.entlen = td->entglen_pr;

	if (!FIPS_drbg_generate(dctx, randout, td->kat2len_pr, 0, 1,
				td->ading_pr, td->adinglen_pr))
		goto err;

	if (memcmp(randout, td->kat2_pr, td->kat2len_pr))
		goto err;

	rv = 1;

	err:
	if (rv == 0)
		FIPSerr(FIPS_F_FIPS_DRBG_SINGLE_KAT, FIPS_R_SELFTEST_FAILED);
	FIPS_drbg_uninstantiate(dctx);
	
	return rv;

	}

/* This is the "health check" function required by SP800-90. Induce several
 * failure modes and check an error condition is set.
 */

static int fips_drbg_health_check(DRBG_CTX *dctx, DRBG_SELFTEST_DATA *td)
	{
	unsigned char randout[1024];
	TEST_ENT t;
	size_t i;
	unsigned int reseed_counter_tmp;
	unsigned char *p = (unsigned char *)dctx;

	/* Initialise DRBG */

	if (!FIPS_drbg_init(dctx, td->nid, td->flags))
		goto err;

	if (!FIPS_drbg_set_callbacks(dctx, test_entropy, 0, 0, test_nonce, 0))
		goto err;

	FIPS_drbg_set_app_data(dctx, &t);

	t.ent = td->ent;
	t.entlen = td->entlen;
	t.nonce = td->nonce;
	t.noncelen = td->noncelen;
	t.entcnt = 0;
	t.noncecnt = 0;

	/* Don't report induced errors */
	dctx->flags |= DRBG_FLAG_NOERR;

	/* Try too large a personalisation length */
	if (FIPS_drbg_instantiate(dctx, td->pers, dctx->max_pers + 1) > 0)
		{
		FIPSerr(FIPS_F_FIPS_DRBG_HEALTH_CHECK, FIPS_R_PERSONALISATION_ERROR_UNDETECTED);
		goto err;
		}

	/* Test entropy source failure detection */

	t.entlen = 0;
	if (FIPS_drbg_instantiate(dctx, td->pers, td->perslen) > 0)
		{
		FIPSerr(FIPS_F_FIPS_DRBG_HEALTH_CHECK, FIPS_R_ENTROPY_ERROR_UNDETECTED);
		goto err;
		}

	/* Try to generate output from uninstantiated DRBG */
	if (FIPS_drbg_generate(dctx, randout, td->katlen, 0, 0,
				td->adin, td->adinlen))
		{
		FIPSerr(FIPS_F_FIPS_DRBG_HEALTH_CHECK, FIPS_R_GENERATE_ERROR_UNDETECTED);
		goto err;
		}

	dctx->flags &= ~DRBG_FLAG_NOERR;
	if (!FIPS_drbg_uninstantiate(dctx))
		{
		FIPSerr(FIPS_F_FIPS_DRBG_HEALTH_CHECK, FIPS_R_UNINSTANTIATE_ERROR);
		goto err;
		}

	/* Instantiate with valid data. NB: errors now reported again */
	if (!FIPS_drbg_init(dctx, td->nid, td->flags))
		goto err;
	if (!FIPS_drbg_set_callbacks(dctx, test_entropy, 0, 0, test_nonce, 0))
		goto err;
	FIPS_drbg_set_app_data(dctx, &t);

	t.entlen = td->entlen;
	if (!FIPS_drbg_instantiate(dctx, td->pers, td->perslen))
		goto err;

	/* Check generation is now OK */
	if (!FIPS_drbg_generate(dctx, randout, td->katlen, 0, 0,
				td->adin, td->adinlen))
		goto err;

	/* Try to generate with too high a strength.
	 */

	dctx->flags |= DRBG_FLAG_NOERR;
	if (dctx->strength != 256)
		{
		if (FIPS_drbg_generate(dctx, randout, td->katlen, 256, 0,
					td->adin, td->adinlen))
			{
			FIPSerr(FIPS_F_FIPS_DRBG_HEALTH_CHECK, FIPS_R_STRENGTH_ERROR_UNDETECTED);

			goto err;
			}
		}

	/* Request too much data for one request */
	if (FIPS_drbg_generate(dctx, randout, dctx->max_request + 1, 0, 0,
				td->adin, td->adinlen))
		{
		FIPSerr(FIPS_F_FIPS_DRBG_HEALTH_CHECK, FIPS_R_REQUEST_LENGTH_ERROR_UNDETECTED);
		goto err;
		}

	/* Check prediction resistance request fails if entropy source
	 * failure.
	 */

	t.entlen = 0;

	if (FIPS_drbg_generate(dctx, randout, td->katlen, 0, 1,
				td->adin, td->adinlen))
		{
		FIPSerr(FIPS_F_FIPS_DRBG_HEALTH_CHECK, FIPS_R_ENTROPY_ERROR_UNDETECTED);
		goto err;
		}
		
	dctx->flags &= ~DRBG_FLAG_NOERR;

	if (!FIPS_drbg_uninstantiate(dctx))
		{
		FIPSerr(FIPS_F_FIPS_DRBG_HEALTH_CHECK, FIPS_R_UNINSTANTIATE_ERROR);
		goto err;
		}


	/* Instantiate again with valid data */

	if (!FIPS_drbg_init(dctx, td->nid, td->flags))
		goto err;
	if (!FIPS_drbg_set_callbacks(dctx, test_entropy, 0, 0, test_nonce, 0))
		goto err;
	FIPS_drbg_set_app_data(dctx, &t);

	t.entlen = td->entlen;
	/* Test reseed counter works */
	if (!FIPS_drbg_instantiate(dctx, td->pers, td->perslen))
		goto err;
	/* Save initial reseed counter */
	reseed_counter_tmp = dctx->reseed_counter;
	/* Set reseed counter to beyond interval */
	dctx->reseed_counter = dctx->reseed_interval;

	/* Generate output and check entropy has been requested for reseed */
	t.entcnt = 0;
	if (!FIPS_drbg_generate(dctx, randout, td->katlen, 0, 0,
				td->adin, td->adinlen))
		goto err;
	if (t.entcnt != 1)
		{
		FIPSerr(FIPS_F_FIPS_DRBG_HEALTH_CHECK, FIPS_R_ENTROPY_NOT_REQUESTED_FOR_RESEED);
		goto err;
		}
	/* Check reseed counter has been reset */
	if (dctx->reseed_counter != reseed_counter_tmp + 1)
		{
		FIPSerr(FIPS_F_FIPS_DRBG_HEALTH_CHECK, FIPS_R_RESEED_COUNTER_ERROR);
		goto err;
		}

	FIPS_drbg_uninstantiate(dctx);
	p = (unsigned char *)&dctx->d;
	/* Standard says we have to check uninstantiate really zeroes
	 * the data...
	 */
	for (i = 0; i < sizeof(dctx->d); i++)
		{
		if (*p != 0)
			{
			FIPSerr(FIPS_F_FIPS_DRBG_HEALTH_CHECK, FIPS_R_UNINSTANTIATE_ZEROISE_ERROR);
			goto err;
			}
		p++;
		}

	return 1;

	err:
	/* A real error as opposed to an induced one: underlying function will
	 * indicate the error.
	 */
	if (!(dctx->flags & DRBG_FLAG_NOERR))
		FIPSerr(FIPS_F_FIPS_DRBG_HEALTH_CHECK, FIPS_R_FUNCTION_ERROR);
	FIPS_drbg_uninstantiate(dctx);
	return 0;

	}


int fips_drbg_kat(DRBG_CTX *dctx, int nid, unsigned int flags)
	{
	int rv;
	DRBG_SELFTEST_DATA *td;
	for (td = drbg_test; td->nid != 0; td++)
		{
		if (td->nid == nid && td->flags == flags)
			{
			rv = fips_drbg_single_kat(dctx, td);
			if (rv <= 0)
				return rv;
			return fips_drbg_health_check(dctx, td);
			}
		}
	return 0;
	}

int FIPS_selftest_drbg(void)
	{
	DRBG_CTX *dctx;
	DRBG_SELFTEST_DATA *td;
	int rv = 1;
	dctx = FIPS_drbg_new(0, 0);
	if (!dctx)
		return 0;
	for (td = drbg_test; td->nid != 0; td++)
		{
		if (td->post != 1)
			continue;
		if (!fips_post_started(FIPS_TEST_DRBG, td->nid, &td->flags))
			return 1;
		if (!fips_drbg_single_kat(dctx, td))
			{
			fips_post_failed(FIPS_TEST_DRBG, td->nid, &td->flags);
			rv = 0;
			continue;
			}
		if (!fips_post_success(FIPS_TEST_DRBG, td->nid, &td->flags))
			return 0;
		}
	FIPS_drbg_free(dctx);
	return rv;
	}



