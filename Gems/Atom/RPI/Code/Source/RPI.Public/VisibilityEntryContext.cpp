/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RPI.Public/VisibilityEntryContext.h>
#include <Atom/RPI.Reflect/Base.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/sort.h>

AZ_DECLARE_BUDGET(RPI);

namespace AZ
{
    namespace RPI
    {
        void VisibilityEntryContext::Shutdown()
        {
            m_visibilityListContext.Clear();

            m_finalizedVisibilityList.clear();
        }

        void VisibilityEntryContext::AddVisibilityEntry(const void* userData, uint32_t lodIndex, float depth)
        {
            if (Validation::IsEnabled())
            {
                if (!userData)
                {
                    AZ_Error("DrawListContext", false, "Null userData was added to a VisibilityEntryContext. This is not permitted and will crash if validation is disabled.");
                    return;
                }
            }

            VisibilityList& visibilityList = m_visibilityListContext.GetStorage();
            VisiblityEntryProperties visibilityEntry;
            visibilityEntry.m_userData = userData;
            visibilityEntry.m_lodIndex = lodIndex;
            visibilityEntry.m_depth = depth;
            // TODO: Should we store/set the sort key here?
            visibilityList.push_back(visibilityEntry);
        }

        void VisibilityEntryContext::FinalizeLists()
        {
            AZ_PROFILE_SCOPE(RPI, "VisibilityEntryContext: FinalizeLists");
            m_finalizedVisibilityList.clear();
            m_visibilityListContext.ForEach(
                [this](VisibilityList& visibilityList)
                {
                    m_finalizedVisibilityList.insert(m_finalizedVisibilityList.end(), visibilityList.begin(), visibilityList.end());
                    visibilityList.clear();
            });
        }

        VisibilityListView VisibilityEntryContext::GetList() const
        {
            return m_finalizedVisibilityList;
        }
    }
}
