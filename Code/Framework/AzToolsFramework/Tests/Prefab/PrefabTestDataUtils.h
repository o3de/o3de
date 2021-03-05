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

#include <Prefab/PrefabTestData.h>
#include <AzToolsFramework/Prefab/Link/Link.h>

namespace UnitTest
{
    namespace PrefabTestDataUtils
    {
        LinkData CreateLinkData(
            const InstanceData& instanceData,
            const AzToolsFramework::Prefab::TemplateId& sourceTemplateId,
            const AzToolsFramework::Prefab::TemplateId& targetTemplateId);

        InstanceData CreateInstanceDataWithNoPatches(
            const AZStd::string& name,
            const AZStd::string& source);

        void ValidateTemplateLoad(
            const TemplateData& expectedTemplateData);

        void ValidateTemplatePatches(
            const AzToolsFramework::Prefab::Link& actualLink,
            const AzToolsFramework::Prefab::PrefabDom& expectedTemplatePatches);

        void CheckIfTemplatesConnected(
            const TemplateData& expectedSourceTemplateData,
            const TemplateData& expectedTargetTemplateData,
            const LinkData& expectedLinkData);
    } 
}

