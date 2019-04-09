// Copyright (C) 2019 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef INCLUDE_FUNAPI_OPENSSL_WRAPPER_H_
#define INCLUDE_FUNAPI_OPENSSL_WRAPPER_H_

#include <openssl/ssl.h>
#include <openssl/err.h>

#ifdef __cplusplus
extern "C" {
#endif

SSL_CTX *Fun_SSL_CTX_new(const SSL_METHOD *meth);
SSL *Fun_SSL_new(SSL_CTX *ctx);
int Fun_SSL_set_fd(SSL *s, int fd);
int Fun_SSL_library_init(void);

int Fun_SSL_shutdown(SSL *s);
void Fun_SSL_free(SSL *ssl);
void Fun_SSL_CTX_free(SSL_CTX *ctx);

int Fun_SSL_connect(SSL *ssl);
int Fun_SSL_write(SSL *ssl, const void *buf, int num);
int Fun_SSL_read(SSL *ssl, void *buf, int num);

const SSL_METHOD *Fun_SSLv23_client_method(void);
const SSL_CIPHER *Fun_SSL_get_current_cipher(const SSL *s);
const char *Fun_SSL_CIPHER_get_name(const SSL_CIPHER *c);

int Fun_SSL_CTX_load_verify_locations(SSL_CTX *ctx, const char *CAfile,
                                      const char *CApath);
void Fun_SSL_CTX_set_verify(SSL_CTX *ctx, int mode,
                            int (*callback) (int, X509_STORE_CTX *));
void Fun_SSL_CTX_set_verify_depth(SSL_CTX *ctx, int depth);
long Fun_SSL_get_verify_result(const SSL *ssl);

X509 *Fun_SSL_get_peer_certificate(const SSL *s);

int Fun_SSL_get_error(const SSL *s, int ret_code);


char *Fun_ERR_error_string(unsigned long e, char *buf);
void Fun_SSL_load_error_strings(void);
unsigned long Fun_ERR_get_error(void);

#ifdef __cplusplus
}
#endif

#endif // INCLUDE_FUNAPI_OPENSSL_WRAPPER_H_
