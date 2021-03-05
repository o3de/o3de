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
#ifndef CRYINCLUDE_CRYSYSTEM_CRYPTO_H
#define CRYINCLUDE_CRYSYSTEM_CRYPTO_H
#pragma once

#include <ICrypto.h>

#include <Cryptography/rijndael.h>
#include <Cryptography/StreamCipher.h>
#include <Cryptography/Whirlpool.h>

class Crypto
    : public ICrypto
{
public:
    // Exposed block encryption
    void EncryptBuffer(uint8* pOutput, const uint8* pInput, uint32 bufferLength, const uint8* pKey, uint32 keyLength) override;
    void DecryptBuffer(uint8* pOutput, const uint8* pInput, uint32 bufferLength, const uint8* pKey, uint32 keyLength) override;

    // Crypto implementations
    IRijndael* GetRijndael() override;
    IStreamCipher* GetStreamCipher() override;

    void InitWhirlpoolHash(uint8* hash);
    void InitWhirlpoolHash(uint8* hash, const string& str);
    void InitWhirlpoolHash(uint8* hash, const uint8* input, size_t length);

protected:
    Rijndael m_rijndael;
    CStreamCipher m_streamCipher;
};

#endif // CRYINCLUDE_CRYSYSTEM_CRYPTO_H
