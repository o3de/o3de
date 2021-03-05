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

#include <Prefab/PrefabTestDomUtils.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/std/optional.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Prefab/Instance/TemplateInstanceMapperInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>

namespace UnitTest
{
    namespace PrefabTestDomUtils
    {

        void SetPrefabDomInstance(
            PrefabDom& prefabDom,
            const char* instanceName,
            const char* source,
            const PrefabDomValue& patches)
        {
            rapidjson::SetValueByPointer(prefabDom, GetPrefabDomSourcePath(instanceName), source);
            if (!patches.IsNull())
            {
                rapidjson::SetValueByPointer(prefabDom, GetPrefabDomPatchesPath(instanceName), patches, prefabDom.GetAllocator());
            }
        }

        PrefabDom CreatePrefabDom()
        {
            PrefabDom newPrefabDom;
            rapidjson::SetValueByPointer(newPrefabDom, "/Entities", rapidjson::Value());

            return newPrefabDom;
        }

        PrefabDom CreatePrefabDom(
            const AZStd::vector<InstanceData>& instancesData)
        {
            PrefabDom newPrefabDom = CreatePrefabDom();
            for (auto& instanceData : instancesData)
            {
                PrefabTestDomUtils::SetPrefabDomInstance(
                    newPrefabDom, instanceData.m_name.c_str(),
                    instanceData.m_source.c_str(), instanceData.m_patches);
            }

            return newPrefabDom;
        }

        void ValidateInstances(
            const TemplateId& templateId,
            const PrefabDomValue& expectedContent,
            const PrefabDomPath& contentPath,
            bool isContentAnInstance)
        {
            TemplateInstanceMapperInterface* templateInstanceMapper =
                AZ::Interface<TemplateInstanceMapperInterface>::Get();
            ASSERT_TRUE(templateInstanceMapper != nullptr);

            ASSERT_TRUE(templateId != AzToolsFramework::Prefab::InvalidTemplateId);
            auto instancesReference = templateInstanceMapper->FindInstancesOwnedByTemplate(templateId);
            ASSERT_TRUE(instancesReference.has_value());
            auto& actualInstances = instancesReference->get();

            for (auto instance : actualInstances)
            {
                PrefabDom instancePrefabDom;
                const bool result = PrefabDomUtils::StoreInstanceInPrefabDom(*instance, instancePrefabDom);
                ASSERT_TRUE(result);

                auto* actualContent = contentPath.Get(instancePrefabDom);
                ASSERT_TRUE(actualContent != nullptr);

                if (isContentAnInstance)
                {
                    ComparePrefabDoms(*actualContent, expectedContent, false);
                }
                else
                {
                    ComparePrefabDomValues(*actualContent, expectedContent);
                }
            }
        }

        void ValidatePrefabDomEntities(const AZStd::vector<EntityAlias>& entityAliases, PrefabDom& prefabDom)
        {
            PrefabDomValueReference templateEntities = PrefabDomUtils::FindPrefabDomValue(prefabDom, "Entities");
            ASSERT_TRUE(templateEntities.has_value());
            for (EntityAlias entityAlias : entityAliases)
            {
                EXPECT_TRUE(PrefabDomUtils::FindPrefabDomValue(templateEntities->get(), entityAlias.c_str()).has_value());
            }
        }

        void ValidatePrefabDomInstances(
            const AZStd::vector<InstanceAlias>& instanceAliases,
            const AzToolsFramework::Prefab::PrefabDom& prefabDom,
            const AzToolsFramework::Prefab::PrefabDom& expectedNestedInstanceDom)
        {
            PrefabDomValueConstReference templateInstances = PrefabDomUtils::FindPrefabDomValue(prefabDom, "Instances");
            ASSERT_TRUE(templateInstances.has_value());
            for (InstanceAlias instanceAlias : instanceAliases)
            {
                PrefabDomValueConstReference actualNestedInstanceDom = PrefabDomUtils::FindPrefabDomValue(templateInstances->get(), instanceAlias.c_str());
                ASSERT_TRUE(actualNestedInstanceDom.has_value());
                ComparePrefabDoms(actualNestedInstanceDom, expectedNestedInstanceDom, false);
            }
        }

        void ComparePrefabDoms(PrefabDomValueConstReference valueA, PrefabDomValueConstReference valueB, bool shouldCompareLinkIds)
        {
            ASSERT_TRUE(valueA.has_value());
            ASSERT_TRUE(valueB.has_value());
            const PrefabDomValue& valueADom = valueA->get();
            const PrefabDomValue& valueBDom = valueB->get();

            if (shouldCompareLinkIds)
            {
                EXPECT_EQ(AZ::JsonSerialization::Compare(valueADom, valueBDom), AZ::JsonSerializerCompareResult::Equal);
            }
            else
            {
                // Compare the source values of the two DOMs.
                PrefabDomValueConstReference actualNestedInstanceDomSource =
                    PrefabDomUtils::FindPrefabDomValue(valueADom, PrefabDomUtils::SourceName);
                PrefabDomValueConstReference expectedNestedInstanceDomSource =
                    PrefabDomUtils::FindPrefabDomValue(valueBDom, PrefabDomUtils::SourceName);
                ComparePrefabDomValues(actualNestedInstanceDomSource, expectedNestedInstanceDomSource);

                // Compare the entities values of the two DOMs.
                PrefabDomValueConstReference actualNestedInstanceDomEntities =
                    PrefabDomUtils::FindPrefabDomValue(valueADom, PrefabTestDomUtils::EntitiesValueName);
                PrefabDomValueConstReference expectedNestedInstanceDomEntities =
                    PrefabDomUtils::FindPrefabDomValue(valueBDom, PrefabTestDomUtils::EntitiesValueName);
                ComparePrefabDomValues(actualNestedInstanceDomEntities, expectedNestedInstanceDomEntities);

                // Compare the instances values of the two DOMs, which involves iterating over each expected instance and comparing it
                // with its counterpart in the actual instance.
                PrefabDomValueConstReference actualNestedInstanceDomInstances =
                    PrefabDomUtils::FindPrefabDomValue(valueADom, PrefabDomUtils::InstancesName);
                PrefabDomValueConstReference expectedNestedInstanceDomInstances =
                    PrefabDomUtils::FindPrefabDomValue(valueBDom, PrefabDomUtils::InstancesName);
                if (expectedNestedInstanceDomInstances.has_value())
                {
                    ASSERT_TRUE(actualNestedInstanceDomInstances.has_value());
                    for (auto instanceIterator = expectedNestedInstanceDomInstances->get().MemberBegin();
                        instanceIterator != expectedNestedInstanceDomInstances->get().MemberEnd(); ++instanceIterator)
                    {
                        ComparePrefabDoms(instanceIterator->value,
                            PrefabDomUtils::FindPrefabDomValue(actualNestedInstanceDomInstances->get(), instanceIterator->name.GetString()),
                            shouldCompareLinkIds);
                    }
                }
            }
        }

        void ComparePrefabDomValues(PrefabDomValueConstReference valueA, PrefabDomValueConstReference valueB)
        {
            if (!valueA.has_value())
            {
                EXPECT_FALSE(valueB.has_value());
            }
            else
            {
                EXPECT_TRUE(valueB.has_value());
                EXPECT_EQ(AZ::JsonSerialization::Compare(valueA->get(), valueB->get()), AZ::JsonSerializerCompareResult::Equal);
            }
        }

        void PrintPrefabDom(const AzToolsFramework::Prefab::PrefabDomValue& prefabDom)
        {
            rapidjson::StringBuffer prefabBuffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(prefabBuffer);
            prefabDom.Accept(writer);
            std::cout << prefabBuffer.GetString() << std::endl;
        }

        void ValidateEntitiesOfInstances(
            const AzToolsFramework::Prefab::TemplateId& templateId,
            const AzToolsFramework::Prefab::PrefabDom& expectedPrefabDom,
            const AZStd::vector<EntityAlias>& entityAliases)
        {
            for (auto& entityAlias : entityAliases)
            {
                PrefabDomPath entityPath = PrefabTestDomUtils::GetPrefabDomEntityPath(entityAlias);
                const PrefabDomValue* expectedEntityValue = PrefabTestDomUtils::GetPrefabDomEntity(expectedPrefabDom, entityAlias);
                ASSERT_TRUE(expectedEntityValue != nullptr);

                PrefabTestDomUtils::ValidateInstances(templateId, *expectedEntityValue, entityPath);
            }
        }

        void ValidateNestedInstancesOfInstances(
            const AzToolsFramework::Prefab::TemplateId& templateId,
            const AzToolsFramework::Prefab::PrefabDom& expectedPrefabDom,
            const AZStd::vector<InstanceAlias>& nestedInstanceAliases)
        {
            for (auto& nestedInstanceAlias : nestedInstanceAliases)
            {
                PrefabDomPath nestedInstancePath = PrefabTestDomUtils::GetPrefabDomInstancePath(nestedInstanceAlias);
                const PrefabDomValue* nestedInstanceValue =
                    PrefabTestDomUtils::GetPrefabDomInstance(expectedPrefabDom, nestedInstanceAlias);
                ASSERT_TRUE(nestedInstanceValue != nullptr);

                PrefabTestDomUtils::ValidateInstances(templateId, *nestedInstanceValue, nestedInstancePath, true);
            }
        }
    }
}
