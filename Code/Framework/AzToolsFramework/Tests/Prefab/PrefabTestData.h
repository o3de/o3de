/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

namespace UnitTest
{
    struct InstanceData
    {
        InstanceData() = default;
        InstanceData(const InstanceData& other);
        InstanceData& operator=(const InstanceData& other);

        AZStd::string m_name;
        AZ::IO::Path m_source;
        AzToolsFramework::Prefab::PrefabDom m_patches;
    };

    struct TemplateData
    {
        AzToolsFramework::Prefab::TemplateId m_id = AzToolsFramework::Prefab::InvalidTemplateId;
        bool m_isValid = true;
        bool m_isLoadedWithErrors = false;
        AZ::IO::Path m_filePath;
        AZStd::unordered_map<AZStd::string, InstanceData> m_instancesData;
    };

    struct LinkData
    {
        bool m_isValid = true;
        InstanceData m_instanceData;
        AzToolsFramework::Prefab::TemplateId m_sourceTemplateId = AzToolsFramework::Prefab::InvalidTemplateId;
        AzToolsFramework::Prefab::TemplateId m_targetTemplateId = AzToolsFramework::Prefab::InvalidTemplateId;
    };
}

