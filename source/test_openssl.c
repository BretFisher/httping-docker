/* Released under AGPL v3 with exception for the OpenSSL library. See license.txt */

#include <stdio.h>
#include <openssl/bio.h>
#include <openssl/conf.h>
#include <openssl/engine.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

int main(int argc, char *argv[])
{
	BIO *bio_err = NULL;

	SSL_library_init();
	SSL_load_error_strings();
	ERR_load_crypto_strings();

	/* error write context */
	bio_err = BIO_new_fp(stderr, BIO_NOCLOSE);

	const SSL_METHOD *meth = SSLv23_method();

	SSL_CTX *ctx = SSL_CTX_new(meth);

	/***/

	BIO_free(bio_err);

	ERR_free_strings();

	ERR_remove_state(0);
	ENGINE_cleanup();
	CONF_modules_free();
	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();

	return 0;
}
