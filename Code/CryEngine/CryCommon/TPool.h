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
#pragma once

#include <CrySizer.h>
#include <CryPodArray.h>

template <class T>
class TPool
{
public:

    TPool(int nPoolSize)
    {
        m_nPoolSize = nPoolSize;
        m_pPool = new T[nPoolSize];
        m_lstFree.PreAllocate(nPoolSize, 0);
        m_lstUsed.PreAllocate(nPoolSize, 0);
        for (int i = 0; i < nPoolSize; i++)
        {
            m_lstFree.Add(&m_pPool[i]);
        }
    }

    ~TPool()
    {
        delete[] m_pPool;
    }

    void ReleaseObject(T* pInst)
    {
        if (m_lstUsed.Delete(pInst))
        {
            m_lstFree.Add(pInst);
        }
    }

    int GetUsedInstancesCount(int& nAll)
    {
        nAll = m_nPoolSize;
        return m_lstUsed.Count();
    }

    T* GetObject()
    {
        T* pInst = NULL;
        if (m_lstFree.Count())
        {
            pInst = m_lstFree.Last();
            m_lstFree.DeleteLast();
            m_lstUsed.Add(pInst);
        }
        else
        {
            assert(!"TPool::GetObject: Out of free elements error");
        }

        return pInst;
    }

    void GetMemoryUsage(class ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_lstFree);
        pSizer->AddObject(m_lstUsed);

        if (m_pPool)
        {
            for (int i = 0; i < m_nPoolSize; i++)
            {
                m_pPool[i].GetMemoryUsage(pSizer);
            }
        }
    }

    PodArray<T*> m_lstFree;
    PodArray<T*> m_lstUsed;
    T* m_pPool;
    int m_nPoolSize;
};