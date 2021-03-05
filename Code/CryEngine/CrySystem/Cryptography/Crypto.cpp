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
#include "CrySystem_precompiled.h"

#include <Cryptography/Crypto.h>
#include <Cryptography/StreamCipher.h>

//-----------------------------------------------------------------------------
void Crypto::EncryptBuffer(uint8* pOutput, const uint8* pInput, uint32 bufferLength, const uint8* pKey, uint32 keyLength)
{
    if (pKey && (keyLength > 0))
    {
        StreamCipherState cipher;
        m_streamCipher.Init(cipher, pKey, keyLength);

        if (pInput && pOutput && (bufferLength > 0))
        {
            m_streamCipher.Encrypt(cipher, pInput, bufferLength, pOutput);
        }
    }
}

//-----------------------------------------------------------------------------
void Crypto::DecryptBuffer(uint8* pOutput, const uint8* pInput, uint32 bufferLength, const uint8* pKey, uint32 keyLength)
{
    if (pKey && (keyLength > 0))
    {
        StreamCipherState cipher;
        m_streamCipher.Init(cipher, pKey, keyLength);

        if (pInput && pOutput && (bufferLength > 0))
        {
            m_streamCipher.Decrypt(cipher, pInput, bufferLength, pOutput);
        }
    }
}

//-----------------------------------------------------------------------------
IRijndael* Crypto::GetRijndael()
{
    return &m_rijndael;
}

//-----------------------------------------------------------------------------
IStreamCipher* Crypto::GetStreamCipher()
{
    return &m_streamCipher;
}
