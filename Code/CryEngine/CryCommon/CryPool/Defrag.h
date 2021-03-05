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

#ifndef CRYINCLUDE_CRYPOOL_DEFRAG_H
#define CRYINCLUDE_CRYPOOL_DEFRAG_H
#pragma once


namespace NCryPoolAlloc
{
    template<class T>
    class CDefragStacked
        :   public  T
    {
        template<class TItem>
        ILINE bool                      DefragElement(TItem* pItem)
        {
            T::m_Items.Validate();
            if (pItem)
            {
                for (; pItem->Next(); pItem = pItem->Next())
                {
                    if (!pItem->IsFree())
                    {
                        continue;
                    }
                    if (pItem->Next()->Locked())
                    {
                        continue;
                    }
                    if (!pItem->Available(pItem->Next()->Align(), pItem->Next()->Align()))
                    {
                        continue;
                    }
                    T::m_Items.Validate(pItem);
                    Stack(pItem);
                    T::m_Items.Validate(pItem);
                    Merge(pItem);
                    T::m_Items.Validate();
                    return true;
                }
            }
            return false;
        }
    public:
        ILINE bool                      Beat()
        {
            return T::Defragmentable() && DefragElement(T::m_Items.First());
        };
    };
}


#endif // CRYINCLUDE_CRYPOOL_DEFRAG_H

