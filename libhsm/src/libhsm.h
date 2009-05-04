/* $Id$ */

/*
 * Copyright (c) 2009 .SE (The Internet Infrastructure Foundation).
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HSM_H
#define HSM_H 1

#include <cryptoki.h>
#include <pkcs11.h>
#include <ldns/ldns.h>
#include <uuid/uuid.h>

#define HSM_MAX_SESSIONS 10

/*! Data type to describe an HSM */
typedef struct {
	unsigned int id;           /*!< HSM numerical identifier */
	const char *name;          /*!< name of module */
	const char *path;          /*!< path to PKCS#11 library */
	const void *handle;        /*!< handle from dlopen()*/
	CK_FUNCTION_LIST_PTR sym;  /*!< Function list from dlsym */
} hsm_module_t;

/*! HSM Key Pair */
typedef struct {
	const hsm_module_t *module;  /*!< pointer to module */
	const CK_OBJECT_HANDLE private_key;  /*!< private key within module */
	const CK_OBJECT_HANDLE public_key;  /*!< public key within module */
	const uuid_t *uuid;          /*!< UUID of key (if available) */
} hsm_key_t;

/*! HSM Session */
typedef struct {
	hsm_module_t *module;
	CK_SESSION_HANDLE session;
} hsm_session_t;

/*! HSM context to keep track of sessions */
typedef struct {
	hsm_session_t *session[HSM_MAX_SESSIONS];  /*!< HSM sessions */
	size_t session_count;     /*!< number of configured HSMs */
} hsm_ctx_t;


/*! Open HSM library

\param config path to OpenDNSSEC XML configuration file
\param pin_callback This function will be called for tokens that have
                    no PIN configured. The default hsm_prompt_pin() can
                    be used. If this value is NULL, these tokens will
                    be skipped
\param data optional data that will be directly passed to the callback
            function
\return 0 if successful, !0 if failed

Attaches all configured HSMs, querying for PINs (using callback
function) if not known.
Also creates initial sessions (not part of any context) and login in to
each HSM.
*/
int hsm_open(const char *config,
             char *(pin_callback)(char *token_name, void *), void *data);


/*! Function that queries for a PIN, can be used as callback
    for hsm_open()

\param token_name The name will be included in the prompt
\param data This value is unused
\return The string the user enters
*/
const char *hsm_prompt_pin(const char *token_name, void *data);

/*! Close HSM library

    Log out and detach from all configured HSMs
*/
int hsm_close();


/*! Create new HSM context

Also destroys any associated session.
*/
const hsm_ctx_t *hsm_create_context(void);

/*! Destroy HSM context

\param context HSM context

Also destroys any associated sessions.
*/
void hsm_destroy_context(const hsm_ctx_t *context);


/*! List all known keys in all attached HSMs

\param context HSM context
*/
hsm_key_t **hsm_list_keys(const hsm_ctx_t *context);

/*! Find a key pair by UUID

\param context HSM context
\param uuid UUID of key to find
\return key identifier or NULL if not found
*/
const hsm_key_t *hsm_find_key_by_uuid(const hsm_ctx_t *context, const uuid_t *uuid);

/*! Generate new key pair in HSM

\param context HSM context
\param repository repository in where to create the key
\param keysize Size of RSA key
\return return key identifier or NULL if key generation failed

Keys generated by libhsm will have a UUID set as CKA_ID and CKA_LABEL.
Other stuff, like exponent, may be needed here as well.
*/
const hsm_key_t *hsm_generate_rsa_key(const hsm_ctx_t *context, const char *repository, unsigned long keysize);

/*! Remove key pair from HSM

\param context HSM context
\param key Key pair to be removed
\return 0 if successful, !0 if failed
*/
int hsm_remove_key(const hsm_ctx_t *context, const hsm_key_t *key);

/*! Get UUID using key identifier

\param context HSM context
\param key Key pair to get UUID from
\return UUID of key pair
*/
const uuid_t *hsm_get_uuid(const hsm_ctx_t *context, const hsm_key_t *key);

/*! Sign RRset using key

\param context HSM context
\param rrset RRset to sign
\param key Key pair used to sign
\return ldns_rr* Signed RRset
*/
ldns_rr* hsm_sign_rrset(const hsm_ctx_t *context, const ldns_rr_list* rrset, const hsm_key_t *key);

/*! Get DNSKEY RR

\param context HSM context
\param key Key to get DNSKEY RR from
\return ldns_rr*
*/
ldns_rr* hsm_get_dnskey(const hsm_ctx_t *context, const hsm_key_t *key);

/*! Fill a buffer with random data from any attached HSM

\param context HSM context
\param length Size of random buffer
\param buffer Buffer to fill with random data
\return 0 if successful, !0 if failed

*/
int hsm_random_buffer(const hsm_ctx_t *context, unsigned long length, unsigned char *buffer);

/*! Return unsigned 32-bit random number from any attached HSM
\param context HSM context
\return 32-bit ranom number
*/
u_int32_t hsm_random32(const hsm_ctx_t *context);

/*! Return unsigned 64-bit random number from any attached HSM
\param context HSM context
\return 64-bit ranom number
*/
u_int64_t hsm_random64(const hsm_ctx_t *context);



/*
 * Additional internal functions
 */

/*! Attached a named HSM using a PKCS#11 shared library and
   optional credentials (may be NULL, but then undefined)
*/
int hsm_attach(const char *repository, const char *path, const char *pin);

/*! Detach a named HSM */
int hsm_detach(const char *name);


#endif /* HSM_H */
