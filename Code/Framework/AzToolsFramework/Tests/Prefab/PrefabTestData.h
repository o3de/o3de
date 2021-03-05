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
        AZStd::string m_source;
        AzToolsFramework::Prefab::PrefabDom m_patches;
    };

    struct TemplateData
    {
        AzToolsFramework::Prefab::TemplateId m_id = AzToolsFramework::Prefab::InvalidTemplateId;
        bool m_isValid = true;
        bool m_isLoadedWithErrors = false;
        AZStd::string m_filePath;
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

