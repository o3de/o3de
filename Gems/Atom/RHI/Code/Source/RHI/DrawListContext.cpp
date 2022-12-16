/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/DrawListContext.h>

#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/sort.h>

namespace AZ
{
    namespace RHI
    {
        bool DrawListContext::IsInitialized() const
        {
            return m_drawListMask.any();
        }

        void DrawListContext::Init(DrawListMask drawListMask)
        {
            AZ_Assert(m_drawListMask.none(), "You must call Shutdown() before re-initializing DrawListContext.")

            if (drawListMask.any())
            {
                m_drawListMask = drawListMask;
            }
        }

        void DrawListContext::Shutdown()
        {
            m_threadListsByTag.Clear();

            for (auto& drawList : m_mergedListsByTag)
            {
                drawList.clear();
            }

            m_drawListMask.reset();
        }

        void DrawListContext::AddDrawPacket(const DrawPacket* drawPacket, float depth)
        {
            if (Validation::IsEnabled())
            {
                if (!drawPacket)
                {
                    AZ_Error("DrawListContext", false, "Null draw packet was added to a draw list context. This is not permitted and will crash if validation is disabled.");
                    return;
                }
            }

            DrawListsByTag& threadListsByTag = m_threadListsByTag.GetStorage();

            for (size_t i = 0; i < drawPacket->GetDrawItemCount(); ++i)
            {
                const DrawListTag drawListTag = drawPacket->GetDrawListTag(i);

                if (m_drawListMask[drawListTag.GetIndex()])
                {
                    DrawItemProperties drawItem = drawPacket->GetDrawItem(i);
                    drawItem.m_depth = depth;
                    threadListsByTag[drawListTag.GetIndex()].push_back(drawItem);
                }
            }
        }

        void DrawListContext::AddDrawItem(DrawListTag drawListTag, DrawItemProperties drawItemProperties)
        {
            if (Validation::IsEnabled())
            {
                if (drawListTag.IsNull())
                {
                    AZ_Error("DrawListContext", false, "Null draw list tag specified in AddDrawItem. This is not permitted and will crash if validation is disabled..");
                    return;
                }
            }

            if (m_drawListMask[drawListTag.GetIndex()])
            {
                DrawListsByTag& drawListsByTag = m_threadListsByTag.GetStorage();
                drawListsByTag[drawListTag.GetIndex()].push_back(drawItemProperties);
            }
        }

        void DrawListContext::FinalizeLists()
        {
            AZ_PROFILE_SCOPE(RHI, "DrawListContext: FinalizeLists");
            for (size_t i = 0; i < m_mergedListsByTag.size(); ++i)
            {
                if (m_drawListMask[i])
                {
                    m_mergedListsByTag[i].clear();
                }
            }

            m_threadListsByTag.ForEach([this](DrawListsByTag& drawListsByTag)
            {
                for (size_t i = 0; i < drawListsByTag.size(); ++i)
                {
                    if (m_drawListMask[i])
                    {
                        auto& sourceList = drawListsByTag[i];
                        auto& resultList = m_mergedListsByTag[i];

                        resultList.insert(resultList.end(), sourceList.begin(), sourceList.end());
                        sourceList.clear();
                    }
                }
            });
        }

        DrawListView DrawListContext::GetList(DrawListTag drawListTag) const
        {
            if (drawListTag.IsValid())
            {
                return m_mergedListsByTag[drawListTag.GetIndex()];
            }
            else
            {
                return {};
            }
        }

        DrawListsByTag& DrawListContext::GetMergedDrawListsByTag()
        {
            return m_mergedListsByTag;
        }
    }
}
