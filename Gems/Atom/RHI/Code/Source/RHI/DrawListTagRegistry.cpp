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

#include <Atom/RHI/DrawListTagRegistry.h>

namespace AZ
{
    namespace RHI
    {
        Ptr<DrawListTagRegistry> DrawListTagRegistry::Create()
        {
            return aznew DrawListTagRegistry;
        }

        void DrawListTagRegistry::Reset()
        {
            AZStd::unique_lock<AZStd::shared_mutex> lock(m_mutex);
            m_entriesByTag.fill({});
            m_allocatedTagCount = 0;
        }

        DrawListTag DrawListTagRegistry::AcquireTag(const Name& drawListName)
        {
            if (drawListName.IsEmpty())
            {
                return {};
            }

            DrawListTag drawListTag;
            Entry* foundEmptyEntry = nullptr;

            AZStd::unique_lock<AZStd::shared_mutex> lock(m_mutex);
            for (size_t i = 0; i < m_entriesByTag.size(); ++i)
            {
                Entry& entry = m_entriesByTag[i];

                // Found an empty entry. Cache off the tag and pointer, but keep searching to find if
                // another entry holds the same name.
                if (entry.m_refCount == 0 && !foundEmptyEntry)
                {
                    foundEmptyEntry = &entry;
                    drawListTag = DrawListTag(i);
                }
                else if (entry.m_name == drawListName)
                {
                    entry.m_refCount++;
                    return DrawListTag(i);
                }
            }

            // No other entry holds the name, so allocate the empty entry.
            if (foundEmptyEntry)
            {
                foundEmptyEntry->m_refCount = 1;
                foundEmptyEntry->m_name = drawListName;
                ++m_allocatedTagCount;
            }

            return drawListTag;
        }

        void DrawListTagRegistry::ReleaseTag(DrawListTag drawListTag)
        {
            if (drawListTag.IsValid())
            {
                AZStd::unique_lock<AZStd::shared_mutex> lock(m_mutex);
                Entry& entry = m_entriesByTag[drawListTag.GetIndex()];
                const size_t refCount = --entry.m_refCount;
                AZ_Assert(refCount != static_cast<size_t>(-1), "Attempted to forfeit a tag that is not valid. Tag{%d},Name{'%s'}", drawListTag, entry.m_name.GetCStr());
                if (refCount == 0)
                {
                    entry.m_name = Name();
                    --m_allocatedTagCount;
                }
            }
        }

        DrawListTag DrawListTagRegistry::FindTag(const Name& drawListName) const
        {
            AZStd::shared_lock<AZStd::shared_mutex> lock(m_mutex);
            for (size_t i = 0; i < m_entriesByTag.size(); ++i)
            {
                if (m_entriesByTag[i].m_name == drawListName)
                {
                    return DrawListTag(i);
                }
            }
            return {};
        }

        Name DrawListTagRegistry::GetName(DrawListTag tag) const
        {
            if (tag.GetIndex() < m_entriesByTag.size())
            {
                return m_entriesByTag[tag.GetIndex()].m_name;
            }
            else
            {
                return Name();
            }
        }

        size_t DrawListTagRegistry::GetAllocatedTagCount() const
        {
            return m_allocatedTagCount;
        }
    }
}
