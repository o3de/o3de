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
/****************************************************
A simple stream cipher based on RC4
****************************************************/

#ifndef CRYINCLUDE_CRYPTOGRAPHY_STREAMCIPHER_H
#define CRYINCLUDE_CRYPTOGRAPHY_STREAMCIPHER_H
#pragma once

#include <ICrypto.h>

class CStreamCipher
    : public IStreamCipher
{
public:
    StreamCipherState BeginCipher(const uint8* pKey, uint32 keyLength);
    void Init(StreamCipherState& state, const uint8* key, int keyLen);
    void Encrypt(StreamCipherState& state, const uint8* input, int inputLen, uint8* output) { ProcessBuffer(state, input, inputLen, output, true); }
    void Decrypt(StreamCipherState& state, const uint8* input, int inputLen, uint8* output) { ProcessBuffer(state, input, inputLen, output, true); }
    void EncryptStream(StreamCipherState& state, const uint8* input, int inputLen, uint8* output) { ProcessBuffer(state, input, inputLen, output, false); }
    void DecryptStream(StreamCipherState& state, const uint8* input, int inputLen, uint8* output) { ProcessBuffer(state, input, inputLen, output, false); }

private:
    uint8 GetNext(StreamCipherState& state);
    void ProcessBuffer(StreamCipherState& state, const uint8* input, int inputLen, uint8* output, bool resetKey);
};

#endif // CRYINCLUDE_CRYPTOGRAPHY_STREAMCIPHER_H
