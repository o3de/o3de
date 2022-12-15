/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/PrefabTestDataUtils.h>

#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <Prefab/PrefabTestDomUtils.h>

namespace UnitTest
{
    namespace PrefabTestDataUtils
    {
        using namespace AzToolsFramework::Prefab;
        LinkData CreateLinkData(
            const InstanceData& instanceData,
            TemplateId sourceTemplateId,
            TemplateId targetTemplateId)
        {
            LinkData newLinkData;
            newLinkData.m_instanceData = instanceData;
            newLinkData.m_sourceTemplateId = sourceTemplateId;
            newLinkData.m_targetTemplateId = targetTemplateId;

            return newLinkData;
        }

        InstanceData CreateInstanceDataWithNoPatches(
            const AZStd::string& name,
            AZ::IO::PathView source)
        {
            InstanceData newInstanceData;
            newInstanceData.m_name = name;
            newInstanceData.m_source = source;
            return newInstanceData;
        }

        void ValidateTemplateLoad(
            const TemplateData& expectedTemplateData)
        {
            PrefabSystemComponentInterface* prefabSystemComponent = AZ::Interface<PrefabSystemComponentInterface>::Get();
            ASSERT_TRUE(prefabSystemComponent != nullptr);

            ASSERT_TRUE(expectedTemplateData.m_id != InvalidTemplateId);
            auto templateReference = prefabSystemComponent->FindTemplate(expectedTemplateData.m_id);
            ASSERT_TRUE(templateReference.has_value());
            auto& actualTemplate = templateReference->get();

            EXPECT_EQ(expectedTemplateData.m_filePath, actualTemplate.GetFilePath());
            EXPECT_EQ(expectedTemplateData.m_isValid, actualTemplate.IsValid());
            EXPECT_EQ(expectedTemplateData.m_isLoadedWithErrors, actualTemplate.IsLoadedWithErrors());

            auto& actualTemplateDom = actualTemplate.GetPrefabDom();
            EXPECT_FALSE(actualTemplateDom.HasMember(PrefabDomUtils::LinkIdName));

            auto& actualInstancesLinkIds = actualTemplate.GetLinks();
            EXPECT_EQ(expectedTemplateData.m_instancesData.size(), actualInstancesLinkIds.size());
            for (auto& actualLinkId : actualInstancesLinkIds)
            {
                auto linkReference = prefabSystemComponent->FindLink(actualLinkId);
                ASSERT_TRUE(linkReference.has_value());

                auto& actualLink = linkReference->get();
                AZStd::string actualLinkName(actualLink.GetInstanceName());

                EXPECT_EQ(expectedTemplateData.m_instancesData.count(actualLinkName), 1);
                auto& expectedInstanceData = expectedTemplateData.m_instancesData.find(actualLinkName)->second;

                EXPECT_EQ(expectedTemplateData.m_id, actualLink.GetTargetTemplateId());
                EXPECT_EQ(expectedInstanceData.m_name, actualLinkName);
                EXPECT_EQ(
                    PrefabTestDomUtils::GetPrefabDomInstancePath(expectedInstanceData.m_name.c_str()),
                    actualLink.GetInstancePath());
                ValidateTemplatePatches(actualLink, expectedInstanceData.m_patches);
            }
        }

        void ValidateTemplatePatches(const Link& actualLink, const PrefabDom& expectedTemplatePatches)
        {
            PrefabDom linkDom;
            actualLink.GetLinkDom(linkDom, linkDom.GetAllocator());
            PrefabDomValueReference patchesReference = PrefabDomUtils::FindPrefabDomValue(linkDom, PrefabDomUtils::PatchesName);

            if (!expectedTemplatePatches.IsNull())
            {
                EXPECT_EQ(AZ::JsonSerialization::Compare(expectedTemplatePatches, patchesReference->get()),
                    AZ::JsonSerializerCompareResult::Equal);
            }
            else
            {
                EXPECT_FALSE(patchesReference.has_value());
            }
        }

        void CheckIfTemplatesConnected(
            const TemplateData& expectedSourceTemplateData,
            const TemplateData& expectedTargetTemplateData,
            const LinkData& expectedLinkData)
        {
            ValidateTemplateLoad(expectedSourceTemplateData);
            ValidateTemplateLoad(expectedTargetTemplateData);

            PrefabSystemComponentInterface* prefabSystemComponent = AZ::Interface<PrefabSystemComponentInterface>::Get();
            ASSERT_TRUE(prefabSystemComponent != nullptr);

            auto& actualSourceTemplate =
                prefabSystemComponent->FindTemplate(expectedSourceTemplateData.m_id)->get();
            auto& actualTargetTemplate =
                prefabSystemComponent->FindTemplate(expectedTargetTemplateData.m_id)->get();

            EXPECT_EQ(expectedLinkData.m_instanceData.m_source, actualSourceTemplate.GetFilePath());

            auto& actualTargetTemplateLinkIds = actualTargetTemplate.GetLinks();
            EXPECT_EQ(expectedTargetTemplateData.m_instancesData.size(), actualTargetTemplateLinkIds.size());

            bool expectedLinkFound = false;
            for (auto actualTargetTemplateLinkId : actualTargetTemplateLinkIds)
            {
                auto linkReference = prefabSystemComponent->FindLink(actualTargetTemplateLinkId);
                ASSERT_TRUE(linkReference.has_value());

                auto& actualLink = linkReference->get();
                if (expectedLinkData.m_instanceData.m_name == actualLink.GetInstanceName())
                {
                    EXPECT_EQ(expectedLinkData.m_isValid, actualLink.IsValid());
                    EXPECT_EQ(expectedLinkData.m_sourceTemplateId, actualLink.GetSourceTemplateId());
                    EXPECT_EQ(expectedLinkData.m_targetTemplateId, actualLink.GetTargetTemplateId());
                    ValidateTemplatePatches(actualLink, expectedLinkData.m_instanceData.m_patches);
                    EXPECT_EQ(
                        PrefabTestDomUtils::GetPrefabDomInstancePath(expectedLinkData.m_instanceData.m_name.c_str()),
                        actualLink.GetInstancePath());

                    expectedLinkFound = true;
                    break;
                }
            }
            EXPECT_TRUE(expectedLinkFound);
        }
    }
}
