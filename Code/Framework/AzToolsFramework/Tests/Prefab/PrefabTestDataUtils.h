/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            AzToolsFramework::Prefab::TemplateId sourceTemplateId,
            AzToolsFramework::Prefab::TemplateId targetTemplateId);

        InstanceData CreateInstanceDataWithNoPatches(
            const AZStd::string& name,
            AZ::IO::PathView source);

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

