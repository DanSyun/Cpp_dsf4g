#pragma once

#include "rsa.h"
#include "pem.h"
#include "err.h"
#include "type_def.h"
#include "singleton.hpp"

class RSACipher: public Singleton<RSACipher>
{
public:
    RSACipher()
    {
        pri_key = NULL;
        pub_key = NULL;
        ERR_load_crypto_strings();
    }
    ~RSACipher()
    {
        if (pri_key != NULL)
            RSA_free(pri_key);

        if (pub_key != NULL)
            RSA_free(pub_key);
    }
    bool GetPubKey(const char* path)
    {
        RSA* p_rsa = NULL;
        FILE* fd = NULL;

        fd = fopen(path, "r");
        if (fd == NULL)
            return false;

        if ((p_rsa = PEM_read_RSA_PUBKEY(fd, NULL, NULL, NULL)) == NULL)
            return false;

        fclose(fd);
        pub_key = p_rsa;
        return true;
    }
    bool GetPriKey(const char* path)
    {
        RSA* p_rsa = NULL;
        FILE* fd = NULL;

        fd = fopen(path, "r");
        if (fd == NULL)
            return false;

        if ((p_rsa = PEM_read_RSAPrivateKey(fd, NULL, NULL, NULL)) == NULL)
            return false;

        fclose(fd);
        pri_key = p_rsa;
        return true;
    }
    bool EncryptWithPriKey(const uint8* src, uint32 src_len, uint8* des, int& des_len)
    {
        if (pri_key == NULL)
            return false;	

    	if (RSA_size(pri_key) != EN_MAX_RSA_TOKEN_LEN)
            return false;	
    	
        memset(des, 0, EN_MAX_RSA_TOKEN_LEN);		

        if ((des_len = RSA_private_encrypt(src_len, src, des, pri_key, RSA_PKCS1_PADDING)) == -1)
            return false;

        return true;
    }
    bool DecryptWithPubKey(const uint8* src, uint32 src_len, uint8* des, int& des_len)
    {
        if (NULL == pub_key)
            return false;	

    	if (RSA_size(pub_key) != EN_MAX_RSA_TOKEN_LEN)
            return false;	

        memset(des, 0, EN_MAX_RSA_TOKEN_LEN);

        if ((des_len = RSA_public_decrypt(src_len, src, des, pub_key, RSA_PKCS1_PADDING)) == -1)
            return false;

        return true;
    }

private:
    RSA *pri_key;
    RSA *pub_key;
};

