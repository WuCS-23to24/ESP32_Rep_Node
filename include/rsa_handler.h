#ifndef F2910BD3_8F6B_4D95_B4AB_CA34DACF1DD4
#define F2910BD3_8F6B_4D95_B4AB_CA34DACF1DD4
#ifndef D0D95129_501D_4BC6_A6EB_4C567C7C456E
#define D0D95129_501D_4BC6_A6EB_4C567C7C456E

#include "mbedtls/bignum.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/esp_config.h"
#include "mbedtls/platform.h"
#include "mbedtls/rsa.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include <Arduino.h>

struct rsa_public_key
{
    static const uint8_t size_rsa_modulus = 128;
    static const uint8_t size_rsa_public_exponent = 32;

    uint8_t rsa_modulus[size_rsa_modulus];
    uint8_t rsa_public_exponent[size_rsa_public_exponent];
};

class RSA
{
  public:
    RSA(int *ret)
    {
        /*
         * Initialize all contexts
         */
        mbedtls_ctr_drbg_init(&ctr_drbg);
        mbedtls_rsa_init(&local_rsa, MBEDTLS_RSA_PKCS_V15, 0);
        mbedtls_rsa_init(&remote_rsa, MBEDTLS_RSA_PKCS_V15, 0);
        mbedtls_entropy_init(&entropy);

        /*
         * Seed the CTR_DRBG context
         */
        if ((*ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)pers,
                                          pers_len)) != 0)
        {
            mbedtls_rsa_free(&local_rsa);
            mbedtls_rsa_free(&remote_rsa);
            mbedtls_ctr_drbg_free(&ctr_drbg);
            mbedtls_entropy_free(&entropy);

            return;
        }

        /*
         * Generating RSA key
         */
        if ((*ret = mbedtls_rsa_gen_key(&local_rsa, mbedtls_ctr_drbg_random, &ctr_drbg, key_size, exponent)) != 0)
        {
            mbedtls_rsa_free(&local_rsa);
            mbedtls_rsa_free(&remote_rsa);
            mbedtls_ctr_drbg_free(&ctr_drbg);
            mbedtls_entropy_free(&entropy);
        }
    }

    int import_remote_rsa_public_key(rsa_public_key *import_key)
    {
        if (import_key == NULL)
        {
            return -1;
        }

        int ret = 0;

        if ((ret = mbedtls_rsa_import_raw(&remote_rsa, import_key->rsa_modulus, import_key->size_rsa_modulus, NULL, 0,
                                          NULL, 0, NULL, 0, import_key->rsa_public_exponent,
                                          import_key->size_rsa_public_exponent)) != 0)
        {
            return ret;
        }

        if ((ret = mbedtls_rsa_complete(&remote_rsa)) != 0)
        {
            return ret;
        }

        if ((ret = mbedtls_rsa_check_pubkey(&remote_rsa)) != 0)
        {
            return ret;
        }

        return 0;
    }

    int export_local_rsa_public_key(rsa_public_key *export_key)
    {
        if (export_key == NULL)
        {
            return -1;
        }

        return mbedtls_rsa_export_raw(&local_rsa, export_key->rsa_modulus, export_key->size_rsa_modulus, NULL, 0, NULL,
                                      0, NULL, 0, export_key->rsa_public_exponent,
                                      export_key->size_rsa_public_exponent);
    }

    int encrypt_data(const unsigned char input_data[128], unsigned char output_data[128])
    {
        if (input_data == NULL || output_data == NULL)
        {
            return -1;
        }

        return mbedtls_rsa_public(&remote_rsa, input_data, output_data);
    }

    int decrypt_data(const unsigned char input_data[128], unsigned char output_data[128])
    {
        if (input_data == NULL || output_data == NULL)
        {
            return -1;
        }

        return mbedtls_rsa_private(&local_rsa, mbedtls_ctr_drbg_random, &ctr_drbg, input_data, output_data);
    }

  private:
    const char *pers = "rsa_genkey";
    const int pers_len = 11;
    const int default_radix = 16;
    const int key_size = 1024;
    const int exponent = 65537;

    mbedtls_rsa_context local_rsa;
    mbedtls_rsa_context remote_rsa;

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;

    inline void malloc_mpi_output_buffer(mbedtls_mpi *X, int radix, unsigned char **buffer, int *len)
    {
        size_t n = mbedtls_mpi_bitlen(X);

        if (radix >= 4)
            n >>= 1;

        if (radix >= 16)
            n >>= 1;

        n += 1;
        n += 1;
        n += 1;
        n += (n & 1);

        *buffer = (unsigned char *)malloc(n * sizeof(unsigned char));
        *len = n;
    }
};

#endif /* D0D95129_501D_4BC6_A6EB_4C567C7C456E */


#endif /* F2910BD3_8F6B_4D95_B4AB_CA34DACF1DD4 */
