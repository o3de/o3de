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

#ifndef CRYINCLUDE_CRYCOMMON_ARRAY2D_H
#define CRYINCLUDE_CRYCOMMON_ARRAY2D_H
#pragma once

// Dynamic replacement for static 2d array
template <class T>
struct Array2d
{
    Array2d()
    {
        m_nSize = 0;
        m_pData = 0;
    }

    int GetSize() const { return m_nSize; }
    int GetDataSize() const { return m_nSize * m_nSize * sizeof(T); }

    T* GetData() { return m_pData; }

    T* GetDataEnd() { return &m_pData[m_nSize * m_nSize]; }

    void SetData(T* pData, int nSize)
    {
        Allocate(nSize);
        memcpy(m_pData, pData, nSize * nSize * sizeof(T));
    }

    void Allocate(int nSize)
    {
        if (m_nSize == nSize)
        {
            return;
        }

        delete [] m_pData;

        m_nSize = nSize;
        m_pData = new T [nSize * nSize];
        memset(m_pData, 0, nSize * nSize * sizeof(T));
    }

    ~Array2d()
    {
        delete [] m_pData;
    }

    void Reset()
    {
        delete [] m_pData;
        m_pData = 0;
        m_nSize = 0;
    }

    T* m_pData;
    int m_nSize;

    T* operator [] (const int& nPos) const
    {
        assert(nPos >= 0 && nPos < m_nSize);
        return &m_pData[nPos * m_nSize];
    }

    Array2d& operator = (const Array2d& other)
    {
        Allocate(other.m_nSize);
        memcpy(m_pData, other.m_pData, m_nSize * m_nSize * sizeof(T));
        return *this;
    }
};

#endif // CRYINCLUDE_CRYCOMMON_ARRAY2D_H
