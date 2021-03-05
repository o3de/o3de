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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "RenderDll_precompiled.h"
#include "TextMessages.h"

const uint32 g_dwTextMessageMaxSizeInKB = 128;

void CTextMessages::PushEntry_Text(const Vec3& vPos, const ColorB col, const float fFontSize, const int nDrawFlags, const char* szText)
{
    AUTO_LOCK(m_TextMessageLock); // Not thread safe without this

    assert(szText);
    assert(!m_dwCurrentReadPos);        // iteration should not be started

    size_t texlen = strlen(szText);
    size_t size = sizeof(SText) + texlen + 1;

    if (size > 1020)
    {
        size = 1020;
        texlen = size - sizeof(SText) - 1;
    }

    size_t paddedSize = (size + 3) & ~3;
    uint8* pData = PushData((uint32) paddedSize);

    if (!pData)
    {
        return;
    }

    memcpy((char*)pData + sizeof(SText), szText, texlen);
    pData[sizeof(SText) + texlen] = '\0';

    SText& rHeader = *(SText*)pData;

    rHeader.Init((uint32) paddedSize);
    rHeader.m_vPos = vPos;
    rHeader.m_Color = col;
    rHeader.m_fFontSize = fFontSize;
    rHeader.m_nDrawFlags = nDrawFlags;
}

void CTextMessages::Clear(bool posonly)
{
    if (!posonly)
    {
        m_TextMessageData.clear();
        // TODO: ideally we should not reconstruct memory
        m_TextMessageData.shrink_to_fit();
    }

    m_dwCurrentReadPos = 0;
}

const CTextMessages::CTextMessageHeader* CTextMessages::GetNextEntry()
{
    uint32 dwSize = (uint32)m_TextMessageData.size();

    if (m_dwCurrentReadPos >= dwSize)
    {
        return 0;       // end reached
    }
    const CTextMessageHeader* pHeader = (const CTextMessageHeader*)&m_TextMessageData[m_dwCurrentReadPos];

    m_dwCurrentReadPos += pHeader->GetSize();

    return pHeader;
}

uint8* CTextMessages::PushData(const uint32 dwBytes)
{
    assert(dwBytes);
    assert(dwBytes % 4 == 0);

    uint32 dwSize = (uint32)m_TextMessageData.size();

    if (dwSize + dwBytes > g_dwTextMessageMaxSizeInKB * 1024)
    {
        return 0;
    }

    m_TextMessageData.resize(dwSize + dwBytes);

    return (uint8*)&m_TextMessageData[dwSize];
}

uint32 CTextMessages::ComputeSizeInMemory() const
{
    return (uint32)(sizeof(*this) + m_TextMessageData.size());
}
