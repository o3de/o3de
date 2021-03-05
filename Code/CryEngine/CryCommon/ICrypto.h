/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef CRYINCLUDE_CRYCOMMON_ICRYPTO_H
#define CRYINCLUDE_CRYCOMMON_ICRYPTO_H
#pragma once

#include "ISerialize.h"

typedef void* TCipher;

class IRijndael;
class IStreamCipher;

struct StreamCipherState
{
    uint8 m_StartS[256];
    uint8 m_S[256];
    int m_StartI;
    int m_I;
    int m_StartJ;
    int m_J;
};

class ICrypto
{
public:
    //Need an empty virtual destructor to address a compiler error with GCC which doesn't allow delete to be called on incomplete types
    virtual ~ICrypto() {};

    // Exposed block encryption
    virtual void EncryptBuffer(uint8* pOutput, const uint8* pInput, uint32 bufferLength, const uint8* pKey, uint32 keyLength) = 0;
    virtual void DecryptBuffer(uint8* pOutput, const uint8* pInput, uint32 bufferLength, const uint8* pKey, uint32 keyLength) = 0;

    // Crypto implementations
    virtual IRijndael* GetRijndael() = 0;
    virtual IStreamCipher* GetStreamCipher() = 0;

    virtual void InitWhirlpoolHash(uint8* hash) = 0;
    virtual void InitWhirlpoolHash(uint8* hash, const string& str) = 0;
    virtual void InitWhirlpoolHash(uint8* hash, const uint8* input, size_t length) = 0;
};

#define _MAX_KEY_COLUMNS (256 / 32)
#define _MAX_ROUNDS      14
#define MAX_IV_SIZE      16

// Error codes
#define RIJNDAEL_SUCCESS 0
#define RIJNDAEL_UNSUPPORTED_MODE -1
#define RIJNDAEL_UNSUPPORTED_DIRECTION -2
#define RIJNDAEL_UNSUPPORTED_KEY_LENGTH -3
#define RIJNDAEL_BAD_KEY -4
#define RIJNDAEL_NOT_INITIALIZED -5
#define RIJNDAEL_BAD_DIRECTION -6
#define RIJNDAEL_CORRUPTED_DATA -7

enum class RijndaelDirection
{
    Encrypt, Decrypt
};
enum class RijndaelMode
{
    ECB, CBC, CFB1
};
enum class RijndaelKeyLength
{
    Key16Bytes, Key24Bytes, Key32Bytes
};

struct RijndaelState
{
    enum State
    {
        Valid, Invalid
    };

    State             m_state;
    RijndaelMode      m_mode;
    RijndaelDirection m_direction;
    uint8             m_initVector[MAX_IV_SIZE];
    uint32            m_uRounds;
    uint8             m_expandedKey[_MAX_ROUNDS + 1][4][4];
};

class IRijndael
{
public:
    //////////////////////////////////////////////////////////////////////////////////////////
    // API
    //////////////////////////////////////////////////////////////////////////////////////////

    // init(): Initializes the crypt session
    // Returns RIJNDAEL_SUCCESS or an error code
    // mode      : Rijndael::ECB, Rijndael::CBC or Rijndael::CFB1
    //             You have to use the same mode for encrypting and decrypting
    // dir       : Rijndael::Encrypt or Rijndael::Decrypt
    //             A cipher instance works only in one direction
    //             (Well , it could be easily modified to work in both
    //             directions with a single init() call, but it looks
    //             useless to me...anyway , it is a matter of generating
    //             two expanded keys)
    // key       : array of unsigned octets , it can be 16 , 24 or 32 bytes long
    //             this CAN be binary data (it is not expected to be null terminated)
    // keyLen    : Rijndael::Key16Bytes , Rijndael::Key24Bytes or Rijndael::Key32Bytes
    // initVector: initialization vector, you will usually use 0 here
    virtual int init(RijndaelState& state, RijndaelMode mode, RijndaelDirection dir, const uint8* key, RijndaelKeyLength keyLen, uint8* initVector = 0) = 0;
    // Encrypts the input array (can be binary data)
    // The input array length must be a multiple of 16 bytes, the remaining part
    // is DISCARDED.
    // so it actually encrypts inputLen / 128 blocks of input and puts it in outBuffer
    // Input len is in BITS!
    // outBuffer must be at least inputLen / 8 bytes long.
    // Returns the encrypted buffer length in BITS or an error code < 0 in case of error
    virtual int blockEncrypt(RijndaelState& state, const uint8* input, int inputLen, uint8* outBuffer) = 0;
    // Encrypts the input array (can be binary data)
    // The input array can be any length , it is automatically padded on a 16 byte boundary.
    // Input len is in BYTES!
    // outBuffer must be at least (inputLen + 16) bytes long
    // Returns the encrypted buffer length in BYTES or an error code < 0 in case of error
    virtual int padEncrypt(RijndaelState& state, const uint8* input, int inputOctets, uint8* outBuffer) = 0;
    // Decrypts the input vector
    // Input len is in BITS!
    // outBuffer must be at least inputLen / 8 bytes long
    // Returns the decrypted buffer length in BITS and an error code < 0 in case of error
    virtual int blockDecrypt(RijndaelState& state, const uint8* input, int inputLen, uint8* outBuffer) = 0;
    // Decrypts the input vector
    // Input len is in BYTES!
    // outBuffer must be at least inputLen bytes long
    // Returns the decrypted buffer length in BYTES and an error code < 0 in case of error
    virtual int padDecrypt(RijndaelState& state, const uint8* input, int inputOctets, uint8* outBuffer) = 0;
};

class IStreamCipher
{
public:
    virtual StreamCipherState BeginCipher(const uint8* pKey, uint32 keyLength) = 0;
    virtual void Init(StreamCipherState& state, const uint8* key, int keyLen) = 0;
    virtual void Encrypt(StreamCipherState& state, const uint8* input, int inputLen, uint8* output) = 0;
    virtual void Decrypt(StreamCipherState& state, const uint8* input, int inputLen, uint8* output) = 0;
    virtual void EncryptStream(StreamCipherState& state, const uint8* input, int inputLen, uint8* output) = 0;
    virtual void DecryptStream(StreamCipherState& state, const uint8* input, int inputLen, uint8* output) = 0;
};

class CWhirlpoolHash
{
public:
    static const int DIGESTBYTES = 64;
    static const int STRING_SIZE = DIGESTBYTES * 2 + 1;

    ILINE CWhirlpoolHash()
    {
        gEnv->pSystem->GetCrypto()->InitWhirlpoolHash(m_hash);
    }

    ILINE CWhirlpoolHash(const string& str)
    {
        gEnv->pSystem->GetCrypto()->InitWhirlpoolHash(m_hash, str);
    }

    ILINE CWhirlpoolHash(const uint8* input, size_t length)
    {
        gEnv->pSystem->GetCrypto()->InitWhirlpoolHash(m_hash, input, length);
    }

    ILINE CryFixedStringT<STRING_SIZE> GetHumanReadable() const
    {
        static const char* hexchars = "0123456789ABCDEF";
        CRY_ASSERT(strlen(hexchars) == 16);

        char buffer[DIGESTBYTES * 2 + 1];
        for (int i = 0; i < DIGESTBYTES; i++)
        {
            buffer[2 * i + 0] = hexchars[m_hash[i] >> 4];
            buffer[2 * i + 1] = hexchars[m_hash[i] & 15];
        }
        buffer[DIGESTBYTES * 2] = 0;
        return buffer;
    }

    ILINE void SerializeWith(TSerialize ser)
    {
        for (int i = 0; i < DIGESTBYTES; i++)
        {
            char name[] = {'w', 'p', 0, 0, 0};
            sprintf_s(name + 2, sizeof(name), "%.2i", i);
            ser.Value(name, m_hash[i]);
        }
    }

    ILINE bool operator==(const CWhirlpoolHash& rhs) const
    {
        return 0 == memcmp(m_hash, rhs.m_hash, DIGESTBYTES);
    }

    ILINE bool operator!=(const CWhirlpoolHash& rhs) const
    {
        return !this->operator==(rhs);
    }

    const uint8* operator()() const { return m_hash; }

private:
    uint8 m_hash[DIGESTBYTES];
};

#endif // CRYINCLUDE_CRYCOMMON_ICRYPTO_H
