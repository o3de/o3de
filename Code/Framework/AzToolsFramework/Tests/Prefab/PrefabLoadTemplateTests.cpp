/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <Prefab/MockPrefabFileIOActionValidator.h>
#include <Prefab/PrefabTestData.h>
#include <Prefab/PrefabTestDataUtils.h>
#include <Prefab/PrefabTestDomUtils.h>
#include <Prefab/PrefabTestFixture.h>

namespace UnitTest
{
    using PrefabLoadTemplateTest = PrefabTestFixture;

    TEST_F(PrefabLoadTemplateTest, LoadTemplate_TemplateWithNoNestedInstance)
    {
        TemplateData templateData;
        templateData.m_filePath = "path/to/template/with/no/nested/instance";

        MockPrefabFileIOActionValidator mockIOActionValidator;
        mockIOActionValidator.ReadPrefabDom(
            templateData.m_filePath, PrefabTestDomUtils::CreatePrefabDom());

        templateData.m_id = m_prefabLoaderInterface->LoadTemplateFromFile(templateData.m_filePath);

        PrefabTestDataUtils::ValidateTemplateLoad(templateData);
    }

    TEST_F(PrefabLoadTemplateTest, LoadTemplate_TemplateWithOneNestedInstance_WithNoPatches)
    {
        TemplateData sourceTemplateData;
        sourceTemplateData.m_filePath = "path/to/template/with/no/nested/instance";

        TemplateData targetTemplateData;
        targetTemplateData.m_filePath = "path/to/template/with/one/nested/instance";

        InstanceData targetTemplateInstanceData = PrefabTestDataUtils::CreateInstanceDataWithNoPatches(
            "sourceTemplateInstance", sourceTemplateData.m_filePath);

        targetTemplateData.m_instancesData[targetTemplateInstanceData.m_name] = targetTemplateInstanceData;
       
        MockPrefabFileIOActionValidator mockIOActionValidator;
        mockIOActionValidator.ReadPrefabDom(
            sourceTemplateData.m_filePath, PrefabTestDomUtils::CreatePrefabDom());
        mockIOActionValidator.ReadPrefabDom(
            targetTemplateData.m_filePath, PrefabTestDomUtils::CreatePrefabDom({ targetTemplateInstanceData }));

        targetTemplateData.m_id = m_prefabLoaderInterface->LoadTemplateFromFile(targetTemplateData.m_filePath);
        sourceTemplateData.m_id = m_prefabSystemComponent->GetTemplateIdFromFilePath(sourceTemplateData.m_filePath);

        LinkData linkData = PrefabTestDataUtils::CreateLinkData(
            targetTemplateInstanceData, sourceTemplateData.m_id, targetTemplateData.m_id);

        PrefabTestDataUtils::CheckIfTemplatesConnected(sourceTemplateData, targetTemplateData, linkData);
    }

    TEST_F(PrefabLoadTemplateTest, LoadTemplate_TemplateDependingOnItself_TemplateLoadedWithErrorsAdded)
    {
        TemplateData templateData;
        templateData.m_filePath = "path/to/template/depending/on/itself";

        auto templatePrefabDom = PrefabTestDomUtils::CreatePrefabDom({
            PrefabTestDataUtils::CreateInstanceDataWithNoPatches("instance", templateData.m_filePath) });

        MockPrefabFileIOActionValidator mockIOActionValidator;
        mockIOActionValidator.ReadPrefabDom(templateData.m_filePath, templatePrefabDom);
        
        AZ_TEST_START_TRACE_SUPPRESSION;
        templateData.m_id = m_prefabLoaderInterface->LoadTemplateFromFile(templateData.m_filePath);
        AZ_TEST_STOP_TRACE_SUPPRESSION(3);
        templateData.m_isLoadedWithErrors = true;

        PrefabTestDataUtils::ValidateTemplateLoad(templateData);
    }

    TEST_F(PrefabLoadTemplateTest, LoadTemplate_SourceTemplateDependingOnTargetTemplate_TemplatesLoadedWithErrorsAdded)
    {
        // Prepare two Template Data which has cyclical dependency between them.
        // Set data of expected source Template. 
        TemplateData sourceTemplateData;
        sourceTemplateData.m_filePath = "path/to/source/template";

        // Set data of expected target Template. 
        TemplateData targetTemplateData;
        targetTemplateData.m_filePath = "path/to/target/template";

        // Set data of expected nested Instance in source Template.
        // The Template of this Instance is target Template so that
        // source Template depends on target Template.
        InstanceData sourceTemplateInstanceData = PrefabTestDataUtils::CreateInstanceDataWithNoPatches(
            "targetTemplateInstance", targetTemplateData.m_filePath);
        
        // Data of expected nested Instance in target Template.
        // The Template of this Instance is source Template so that
        // target Template depends on source Template.
        InstanceData targetTemplateInstanceData = PrefabTestDataUtils::CreateInstanceDataWithNoPatches(
            "sourceTemplateInstance", sourceTemplateData.m_filePath);

        // Set expected target Template's Instance data.
        // There should be NO Instance data in expected source Template
        // since cyclical dependency will be detected and LoadTemplate will stop.
        targetTemplateData.m_instancesData[targetTemplateInstanceData.m_name] = targetTemplateInstanceData;

        // Create PrefabDoms for both source/target Template.
        auto sourceTemplatePrefabDom = PrefabTestDomUtils::CreatePrefabDom({ sourceTemplateInstanceData });
        auto targetTemplatePrefabDom = PrefabTestDomUtils::CreatePrefabDom({ targetTemplateInstanceData });

        // The mock file IO will let the PrefabSystemComponent read expected PrefabDoms while calling LoadTemplate. 
        MockPrefabFileIOActionValidator mockIOActionValidator;
        mockIOActionValidator.ReadPrefabDom(
            sourceTemplateData.m_filePath, sourceTemplatePrefabDom);
        mockIOActionValidator.ReadPrefabDom(
            targetTemplateData.m_filePath, targetTemplatePrefabDom);

        // Load target and source Templates and get their Ids.
        AZ_TEST_START_TRACE_SUPPRESSION;
        targetTemplateData.m_id = m_prefabLoaderInterface->LoadTemplateFromFile(targetTemplateData.m_filePath);
        AZ_TEST_STOP_TRACE_SUPPRESSION(4);
        sourceTemplateData.m_id = m_prefabSystemComponent->GetTemplateIdFromFilePath(sourceTemplateData.m_filePath);

        // Because of cyclical dependency, the two Templates should be loaded with errors.
        sourceTemplateData.m_isLoadedWithErrors = true;
        targetTemplateData.m_isLoadedWithErrors = true;

        // Set expected data of Link from source Template to target Template.
        // There should be no Link from target Template to source Template.
        LinkData linkData = PrefabTestDataUtils::CreateLinkData(
            targetTemplateInstanceData, sourceTemplateData.m_id, targetTemplateData.m_id);

        // Verify if actual source/target Templates have the expected Template data.
        // Also check if actual Link from source to target has the expected Link data.
        PrefabTestDataUtils::CheckIfTemplatesConnected(sourceTemplateData, targetTemplateData, linkData);
    }

    TEST_F(PrefabLoadTemplateTest, LoadTemplate_InstanceWithEmptySource_TemplateLoadedWithErrorsAdded)
    {
        TemplateData templateData;
        templateData.m_filePath = "path/to/template/with/no/instance/source";
        templateData.m_isLoadedWithErrors = true;

        auto templatePrefabDom = PrefabTestDomUtils::CreatePrefabDom({
            PrefabTestDataUtils::CreateInstanceDataWithNoPatches("templateInstance", "") });

        MockPrefabFileIOActionValidator mockIOActionValidator;
        mockIOActionValidator.ReadPrefabDom(templateData.m_filePath, templatePrefabDom);

        AZ_TEST_START_TRACE_SUPPRESSION;
        templateData.m_id = m_prefabLoaderInterface->LoadTemplateFromFile(templateData.m_filePath);
        AZ_TEST_STOP_TRACE_SUPPRESSION(2);

        PrefabTestDataUtils::ValidateTemplateLoad(templateData);
    }

    TEST_F(PrefabLoadTemplateTest, LoadTemplate_InstanceWithEmptyName_TemplateLoadedWithErrorsAdded)
    {
        TemplateData templateData;
        templateData.m_filePath = "path/to/template/with/no/instance/name";
        templateData.m_isLoadedWithErrors = true;

        auto templatePrefabDom = PrefabTestDomUtils::CreatePrefabDom({
            PrefabTestDataUtils::CreateInstanceDataWithNoPatches("", "template/instance/source") });

        MockPrefabFileIOActionValidator mockIOActionValidator;
        mockIOActionValidator.ReadPrefabDom(templateData.m_filePath, templatePrefabDom);

        AZ_TEST_START_TRACE_SUPPRESSION;
        templateData.m_id = m_prefabLoaderInterface->LoadTemplateFromFile(templateData.m_filePath);
        AZ_TEST_STOP_TRACE_SUPPRESSION(2);

        PrefabTestDataUtils::ValidateTemplateLoad(templateData);
    }

    TEST_F(PrefabLoadTemplateTest, LoadTemplate_OpenSourceTemplateFileFailed_TemplateLoadedWithErrorsAdded)
    {
        TemplateData templateData;
        templateData.m_filePath = "path/to/template";
        templateData.m_isLoadedWithErrors = true;

        InstanceData templateInstanceData = PrefabTestDataUtils::CreateInstanceDataWithNoPatches(
            "templateInstance", "wrong/path");
       
        MockPrefabFileIOActionValidator mockIOActionValidator;
        mockIOActionValidator.ReadPrefabDom(
            templateData.m_filePath,
            PrefabTestDomUtils::CreatePrefabDom({ templateInstanceData }));
        mockIOActionValidator.ReadPrefabDom(
            templateInstanceData.m_source, PrefabTestDomUtils::CreatePrefabDom(),
            AZ::IO::ResultCode::Success, AZ::IO::ResultCode::Error);

        AZ_TEST_START_TRACE_SUPPRESSION;
        templateData.m_id = m_prefabLoaderInterface->LoadTemplateFromFile(templateData.m_filePath);
        AZ_TEST_STOP_TRACE_SUPPRESSION(3);

        PrefabTestDataUtils::ValidateTemplateLoad(templateData);
    }

    TEST_F(PrefabLoadTemplateTest, LoadTemplate_MultiLevelTemplates_WithNoPatches)
    {
        MockPrefabFileIOActionValidator mockIOActionValidator;
        AZStd::vector<TemplateData> templatesData;

        const int nestedHierarchyLevel = 3;
        for (int i = 0; i < nestedHierarchyLevel; i++)
        {
            TemplateData templateData;
            templateData.m_filePath = AZStd::string::format("path/to/level/%d/template", i);
            templatesData.emplace_back(templateData);

            if (i !=  0)
            {
                InstanceData templateInstanceData = PrefabTestDataUtils::CreateInstanceDataWithNoPatches(
                    AZStd::string::format("level%dTemplateInstance", i), templatesData[i - 1].m_filePath);

                templatesData[i].m_instancesData[templateInstanceData.m_name] = templateInstanceData;
              
                mockIOActionValidator.ReadPrefabDom(
                    templatesData[i].m_filePath, PrefabTestDomUtils::CreatePrefabDom({ templateInstanceData }));
            }
            else
            {
                mockIOActionValidator.ReadPrefabDom(
                    templatesData[i].m_filePath, PrefabTestDomUtils::CreatePrefabDom());
            }
        }

        templatesData.back().m_id = m_prefabLoaderInterface->LoadTemplateFromFile(templatesData.back().m_filePath);
        for (int i = nestedHierarchyLevel - 2; i >= 0; i--)
        {
            templatesData[i].m_id = m_prefabSystemComponent->GetTemplateIdFromFilePath(templatesData[i].m_filePath);

            LinkData linkData = PrefabTestDataUtils::CreateLinkData(
                templatesData[i + 1].m_instancesData[AZStd::string::format("level%dTemplateInstance", i + 1)],
                templatesData[i].m_id, templatesData[i + 1].m_id);

            PrefabTestDataUtils::CheckIfTemplatesConnected(templatesData[i], templatesData[i + 1], linkData);
        }
    }

    TEST_F(PrefabLoadTemplateTest, LoadTemplate_TemplateWithMultiInstances_WithNoPatches)
    {
        MockPrefabFileIOActionValidator mockIOActionValidator;
        AZStd::vector<TemplateData> sourceTemplatesData;
        AZStd::vector<InstanceData> targetTemplateInstancesData;
        TemplateData targetTemplateData;
        targetTemplateData.m_filePath = "path/to/target/template";

        const int numInstances = 3;
        for (int i = 0; i < numInstances; i++)
        {
            TemplateData sourceTemplateData;
            sourceTemplateData.m_filePath = AZStd::string::format("path/to/source/%d/template", i);

            InstanceData targetTemplateInstanceData = PrefabTestDataUtils::CreateInstanceDataWithNoPatches(
                AZStd::string::format("source%dTemplateInstance", i), sourceTemplateData.m_filePath);

            targetTemplateData.m_instancesData[targetTemplateInstanceData.m_name] = targetTemplateInstanceData;

            mockIOActionValidator.ReadPrefabDom(
                sourceTemplateData.m_filePath, PrefabTestDomUtils::CreatePrefabDom());

            sourceTemplatesData.emplace_back(sourceTemplateData);
            targetTemplateInstancesData.emplace_back(targetTemplateInstanceData);

        }
        mockIOActionValidator.ReadPrefabDom(
            targetTemplateData.m_filePath, PrefabTestDomUtils::CreatePrefabDom(targetTemplateInstancesData));

        targetTemplateData.m_id = m_prefabLoaderInterface->LoadTemplateFromFile(targetTemplateData.m_filePath);
        for (int i = 0; i < numInstances; i++)
        {
            sourceTemplatesData[i].m_id = m_prefabSystemComponent->GetTemplateIdFromFilePath(sourceTemplatesData[i].m_filePath);

            LinkData linkFromSourceData = PrefabTestDataUtils::CreateLinkData(targetTemplateInstancesData[i],
                sourceTemplatesData[i].m_id, targetTemplateData.m_id);
           
            PrefabTestDataUtils::CheckIfTemplatesConnected(sourceTemplatesData[i], targetTemplateData, linkFromSourceData);
        }
    }

    TEST_F(PrefabLoadTemplateTest, LoadTemplate_LoadCorruptedPrefabFileData_InvalidTemplateIdReturned)
    {
        const AZStd::string corruptedPrefabContent = "{ Corrupted PrefabDom";
        AZ::IO::Path pathToCorruptedPrefab("path/to/corrupted/prefab/file");
        pathToCorruptedPrefab.MakePreferred();

        MockPrefabFileIOActionValidator mockIOActionValidator;
        mockIOActionValidator.ReadPrefabDom(pathToCorruptedPrefab, corruptedPrefabContent);

        AZ_TEST_START_TRACE_SUPPRESSION;
        TemplateId templateId = m_prefabLoaderInterface->LoadTemplateFromFile(pathToCorruptedPrefab);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        EXPECT_EQ(templateId, AzToolsFramework::Prefab::InvalidTemplateId);
    }

    TEST_F(PrefabLoadTemplateTest, LoadTemplate_LoadFromString_InvalidPathReturnsInvalidTemplateId)
    {
        PrefabDom emptyPrefabDom = PrefabTestDomUtils::CreatePrefabDom();
        AZStd::string emptyPrefabDomStr = PrefabTestDomUtils::DomToString(emptyPrefabDom);
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_EQ(m_prefabLoaderInterface->LoadTemplateFromString(emptyPrefabDomStr, "|?<>"), AzToolsFramework::Prefab::InvalidTemplateId);
        EXPECT_EQ(m_prefabLoaderInterface->LoadTemplateFromString(emptyPrefabDomStr, "notAFile/"), AzToolsFramework::Prefab::InvalidTemplateId);
        AZ_TEST_STOP_TRACE_SUPPRESSION(2);
    }

    TEST_F(PrefabLoadTemplateTest, LoadTemplate_LoadFromString_LoadsEmptyPrefab)
    {
        TemplateData templateData;
        templateData.m_filePath = "path/to/empty/prefab";

        PrefabDom emptyPrefabDom = PrefabTestDomUtils::CreatePrefabDom();
        AZStd::string emptyPrefabDomStr = PrefabTestDomUtils::DomToString(emptyPrefabDom);
        templateData.m_id = m_prefabLoaderInterface->LoadTemplateFromString(
            emptyPrefabDomStr,
            templateData.m_filePath
        );

        PrefabTestDataUtils::ValidateTemplateLoad(templateData);
    }

    TEST_F(PrefabLoadTemplateTest, LoadTemplate_LoadFromString_TemplateDependingOnItself_LoadedWithErrors)
    {
        TemplateData templateData;
        templateData.m_filePath = "path/to/self/dependency";

        PrefabDom selfDependentPrefab = PrefabTestDomUtils::CreatePrefabDom(
            { PrefabTestDataUtils::CreateInstanceDataWithNoPatches("instance", templateData.m_filePath) }
        );

        AZStd::string selfDependentPrefabStr = PrefabTestDomUtils::DomToString(selfDependentPrefab);
        AZ_TEST_START_TRACE_SUPPRESSION;
        templateData.m_id = m_prefabLoaderInterface->LoadTemplateFromString(
            selfDependentPrefabStr,
            templateData.m_filePath);
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT; // produces different counts in Jenkins vs local

        templateData.m_isLoadedWithErrors = true;

        PrefabTestDataUtils::ValidateTemplateLoad(templateData);
    }

    TEST_F(PrefabLoadTemplateTest, LoadTemplate_LoadFromString_CorruptedReturnsInvalidTemplateId)
    {
        AZStd::string corruptPrefab = "{ Corrupted PrefabDom";
        AZ_TEST_START_TRACE_SUPPRESSION;
        TemplateId templateId = m_prefabLoaderInterface->LoadTemplateFromString(corruptPrefab);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_EQ(templateId, AzToolsFramework::Prefab::InvalidTemplateId);
    }
}
