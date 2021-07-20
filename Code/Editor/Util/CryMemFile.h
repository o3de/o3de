/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
