/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RPI.Public/VisibleObjectContext.h>
#include <Atom/RPI.Reflect/Base.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/sort.h>

ATOM_RPI_PUBLIC_API AZ_DECLARE_BUDGET(RPI);

namespace AZ
{
    namespace RPI
    {
        void VisibleObjectContext::Shutdown()
        {
            m_visibleObjectListContext.Clear();

            m_finalizedVisibleObjectList.clear();
        }

        void VisibleObjectContext::AddVisibleObject(const void* userData, float depth)
        {
            if (userData)
            {
                VisibleObjectList& visibleObjectList = m_visibleObjectListContext.GetStorage();
                VisibleObjectProperties visibleObject;
                visibleObject.m_userData = userData;
                visibleObject.m_depth = depth;
                visibleObjectList.push_back(visibleObject);
            }
            else
            {
                AZ_Error(
                    "VisibleObjectContext",
                    false,
                    "Null userData was added to a VisibleObjectContext. Visible object will be ignored.");
            }
        }

        void VisibleObjectContext::FinalizeLists()
        {
            AZ_PROFILE_SCOPE(RPI, "VisibleObjectContext: FinalizeLists");
            m_finalizedVisibleObjectList.clear();

            // Reserve enough memory for all the visible objects
            size_t objectCount = 0;
            m_visibleObjectListContext.ForEach(
                [&objectCount](const VisibleObjectList& visibleObjectList)
                {
                    objectCount += visibleObjectList.size();
                });
            m_finalizedVisibleObjectList.reserve(objectCount);

            // Concatenate all the per-thread lists into a single list
            m_visibleObjectListContext.ForEach(
                [this](VisibleObjectList& visibleObjectList)
                {
                    m_finalizedVisibleObjectList.insert(m_finalizedVisibleObjectList.end(), visibleObjectList.begin(), visibleObjectList.end());
                    visibleObjectList.clear();
            });
        }

        VisibleObjectListView VisibleObjectContext::GetList() const
        {
            return m_finalizedVisibleObjectList;
        }
    }
}
