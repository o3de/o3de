/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/AzCoreModuleStatic.h>

// Component includes
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Script/ScriptSystemComponent.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Slice/SliceSystemComponent.h>
#include <AzCore/Slice/SliceMetadataInfoComponent.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/Console/LoggerSystemComponent.h>
#include <AzCore/EBus/EventSchedulerSystemComponent.h>
#include <AzCore/Task/TaskGraphSystemComponent.h>
#include <AzCore/Statistics/StatisticalProfilerProxySystemComponent.h>

namespace AZ
{
    AzCoreModuleStatic::AzCoreModuleStatic()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
        });
    }

    AZ::ComponentTypeList AzCoreModuleStatic::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList
        {
        };
    }
}
