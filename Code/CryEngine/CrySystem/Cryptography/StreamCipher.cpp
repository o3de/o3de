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
#include "StreamCipher.h"

StreamCipherState CStreamCipher::BeginCipher(const uint8* pKey, uint32 keyLength)
{
    StreamCipherState cipher;
    Init(cipher, pKey, keyLength);
    return cipher;
}

void CStreamCipher::Init(StreamCipherState& state, const uint8* key, int keyLen)
{
    int i, j;

    for (i = 0; i < 256; i++)
    {
        state.m_S[i] = i;
    }

    if (key)
    {
        for (i = j = 0; i < 256; i++)
        {
            uint8 temp;

            j = (j + key[i % keyLen] + state.m_S[i]) & 255;
            temp = state.m_S[i];
            state.m_S[i] = state.m_S[j];
            state.m_S[j] = temp;
        }
    }

    state.m_I = state.m_J = 0;

    for (i = 0; i < 1024; i++)
    {
        GetNext(state);
    }

    memcpy(state.m_StartS, state.m_S, sizeof(state.m_StartS));

    state.m_StartI = state.m_I;
    state.m_StartJ = state.m_J;
}

uint8 CStreamCipher::GetNext(StreamCipherState& state)
{
    uint8 tmp;

    state.m_I = (state.m_I + 1) & 0xff;
    state.m_J = (state.m_J + state.m_S[state.m_I]) & 0xff;

    tmp = state.m_S[state.m_J];
    state.m_S[state.m_J] = state.m_S[state.m_I];
    state.m_S[state.m_I] = tmp;

    return state.m_S[(state.m_S[state.m_I] + state.m_S[state.m_J]) & 0xff];
}

void CStreamCipher::ProcessBuffer(StreamCipherState& state, const uint8* input, int inputLen, uint8* output, bool resetKey)
{
    if (resetKey)
    {
        memcpy(state.m_S, state.m_StartS, sizeof(state.m_S));
        state.m_I = state.m_StartI;
        state.m_J = state.m_StartJ;
    }

    for (int i = 0; i < inputLen; i++)
    {
        output[i] = input[i] ^ GetNext(state);
    }
}
