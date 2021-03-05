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

#ifndef CRYINCLUDE_EDITOR_UTIL_CRYMEMFILE_H
#define CRYINCLUDE_EDITOR_UTIL_CRYMEMFILE_H
#pragma once

#include <QBuffer>

// derived class to get correct memory allocation/deallocation with custom memory manager - and to avoid memory leaks from calling Detach()
class CCryMemFile
    : public QBuffer
{
public: // ---------------------------------------------------------------

    CCryMemFile()
        : QBuffer(&m_lpBuffer)
    {
        open(QIODevice::WriteOnly);
    }
    CCryMemFile(char* lpBuffer, int nBufferSize)
        : QBuffer(&m_lpBuffer), m_lpBuffer(lpBuffer, nBufferSize)
    {
        open(QIODevice::WriteOnly);
    }

    virtual ~CCryMemFile()
    {
        close();        // call Close() to make sure the Free() is using my v-table
    }

    qulonglong GetPosition() const
    {
        return QBuffer::pos();
    }

    qulonglong GetLength() const
    {
        return QBuffer::size();
    }

    void Write(const void* lpBuf, unsigned int nCount)
    {
        QBuffer::write(reinterpret_cast<const char*>(lpBuf), nCount);
    }

    // only for temporary use
    void* GetMemPtr() const
    {
        return const_cast<char*>(m_lpBuffer.data());
    }

    void Close()
    {
        QBuffer::close();
    }

    char* Detach()
    {
        assert(0);      // dangerous - most likely we cause memory leak - better use GetMemPtr
        return 0;
    }

private:
    QByteArray m_lpBuffer;
};

#endif // CRYINCLUDE_EDITOR_UTIL_CRYMEMFILE_H
