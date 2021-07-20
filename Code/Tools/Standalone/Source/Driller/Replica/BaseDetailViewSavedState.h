/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_REPLICA_BASEDETAILVIEWSAVEDSTATE_H
#define DRILLER_REPLICA_BASEDETAILVIEWSAVEDSTATE_H

#include <AzCore/std/containers/vector.h>
#include <AzCore/UserSettings/UserSettings.h>

namespace Driller
{
    class BaseDetailViewSplitterSavedState
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(BaseDetailViewSplitterSavedState, "{280A523E-9A7F-4E23-BAF8-1F6084AB77D6}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(BaseDetailViewSplitterSavedState, AZ::SystemAllocator, 0);

        AZStd::vector< AZ::u8 > m_splitterStorage;

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);

            if (serialize)
            {
                serialize->Class<BaseDetailViewSplitterSavedState>()
                    ->Field("m_splitterStorage", &BaseDetailViewSplitterSavedState::m_splitterStorage)
                    ->Version(1);
                ;
            }
        }
    };

    class BaseDetailViewTreeSavedState
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(BaseDetailViewTreeSavedState, "{4B3ED3CE-5446-4DCD-98D3-62B577A75786}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(BaseDetailViewTreeSavedState, AZ::SystemAllocator, 0);

        AZStd::vector<AZ::u8> m_treeColumnStorage;

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);

            if (serialize)
            {
                serialize->Class<BaseDetailViewTreeSavedState>()
                    ->Field("m_treeColumnStorage", &BaseDetailViewTreeSavedState::m_treeColumnStorage)
                    ->Version(1);
            }
        }
    };
}
#endif
