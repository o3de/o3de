/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "assetBuilderSDKTest.h"

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/UnitTest/TestTypes.h>

#include "native/tests/BaseAssetProcessorTest.h"
#include "native/unittests/UnitTestRunner.h"

namespace AssetProcessor
{
    struct AssetBehaviorContextTest
        : public ::testing::Test
    {
        struct DataMembers
        {
            UnitTestUtils::AssertAbsorber m_absorber;
            DataMembers() = default;
        };

        // the component application creates and returns a system entity, but doesn't keep track of it
        AZ::Entity* m_systemEntity = nullptr;

        // store all data we create here so that it can be destroyed on shutdown before we remove allocators
        DataMembers* m_data = nullptr;

        // the app is created separately so that we can control its lifetime.
        AZStd::unique_ptr<AZ::ComponentApplication> m_app; 

        void SetUp() override
        {
            m_app.reset(aznew AZ::ComponentApplication());
            AZ::ComponentApplication::Descriptor desc;
            m_systemEntity = m_app->Create(desc);

            AssetBuilderSDK::InitializeSerializationContext();
            AssetBuilderSDK::InitializeBehaviorContext();

            m_data = azcreate(DataMembers, ());
        }

        void TearDown() override
        {
            EXPECT_EQ(0, m_data->m_absorber.m_numAssertsAbsorbed);
            EXPECT_EQ(0, m_data->m_absorber.m_numErrorsAbsorbed);
            EXPECT_EQ(0, m_data->m_absorber.m_numWarningsAbsorbed);
            azdestroy(m_data);

            delete m_systemEntity;
            m_systemEntity = nullptr;
            m_app->Destroy();
            m_app.reset();
        }

        bool IsBehaviorFlaggedForEditor(const AZ::AttributeArray& attributes)
        {
            AZ::Script::Attributes::ScopeFlags scopeType = AZ::Script::Attributes::ScopeFlags::Launcher;
            AZ::Attribute* scopeAttribute = AZ::FindAttribute(AZ::Script::Attributes::Scope, attributes);
            if (scopeAttribute)
            {
                AZ::AttributeReader scopeAttributeReader(nullptr, scopeAttribute);
                scopeAttributeReader.Read<AZ::Script::Attributes::ScopeFlags>(scopeType);
            }
            return (scopeType == AZ::Script::Attributes::ScopeFlags::Automation ||
                    scopeType == AZ::Script::Attributes::ScopeFlags::Common);
        }
    };

    TEST_F(AssetBehaviorContextTest, DetectBehaviorAssetBuilderPattern)
    {
        auto&& behaviorContext = m_app->GetBehaviorContext();
        auto behaviorClassEntry = behaviorContext->m_classes.find("AssetBuilderPattern");
        EXPECT_TRUE(behaviorContext->m_classes.end() != behaviorClassEntry);
        AZ::BehaviorClass* behaviorClass = behaviorClassEntry->second;
        EXPECT_TRUE(IsBehaviorFlaggedForEditor(behaviorClass->m_attributes));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("type"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("pattern"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("Regex"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("Wildcard"));
        EXPECT_EQ(1, behaviorClass->m_constructors.size());

        EXPECT_TRUE(behaviorContext->m_classes.end() != behaviorContext->m_classes.find("AZStd::vector<AssetBuilderPattern, allocator>"));
    }

    TEST_F(AssetBehaviorContextTest, DetectBehaviorJobDescriptor)
    {
        auto&& behaviorContext = m_app->GetBehaviorContext();
        auto behaviorClassEntry = behaviorContext->m_classes.find("JobDescriptor");
        EXPECT_TRUE(behaviorContext->m_classes.end() != behaviorClassEntry);
        AZ::BehaviorClass* behaviorClass = behaviorClassEntry->second;
        EXPECT_TRUE(IsBehaviorFlaggedForEditor(behaviorClass->m_attributes));
        EXPECT_TRUE(behaviorClass->m_methods.end() != behaviorClass->m_methods.find("set_platform_identifier"));
        EXPECT_TRUE(behaviorClass->m_methods.end() != behaviorClass->m_methods.find("get_platform_identifier"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("jobParameters"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("additionalFingerprintInfo"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("priority"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("checkExclusiveLock"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("checkServer"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("jobDependencyList"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("failOnError"));
        EXPECT_EQ(2, behaviorClass->m_constructors.size());

        EXPECT_TRUE(behaviorContext->m_classes.end() != behaviorContext->m_classes.find("AZStd::vector<JobDescriptor, allocator>"));
    }

    TEST_F(AssetBehaviorContextTest, DetectBehaviorProductDependency)
    {
        auto&& behaviorContext = m_app->GetBehaviorContext();
        auto behaviorClassEntry = behaviorContext->m_classes.find("ProductDependency");
        EXPECT_TRUE(behaviorContext->m_classes.end() != behaviorClassEntry);
        AZ::BehaviorClass* behaviorClass = behaviorClassEntry->second;
        EXPECT_TRUE(IsBehaviorFlaggedForEditor(behaviorClass->m_attributes));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("dependencyId"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("flags"));
        EXPECT_EQ(1, behaviorClass->m_constructors.size());

        EXPECT_TRUE(behaviorContext->m_classes.end() != behaviorContext->m_classes.find("AZStd::vector<ProductDependency, allocator>"));
    }

    TEST_F(AssetBehaviorContextTest, DetectBehaviorJobProduct)
    {
        auto&& behaviorContext = m_app->GetBehaviorContext();
        auto behaviorClassEntry = behaviorContext->m_classes.find("JobProduct");
        EXPECT_TRUE(behaviorContext->m_classes.end() != behaviorClassEntry);
        AZ::BehaviorClass* behaviorClass = behaviorClassEntry->second;
        EXPECT_TRUE(IsBehaviorFlaggedForEditor(behaviorClass->m_attributes));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("productFileName"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("productAssetType"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("productSubID"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("productDependencies"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("pathDependencies"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("dependenciesHandled"));
        EXPECT_EQ(2, behaviorClass->m_constructors.size());

        EXPECT_TRUE(behaviorContext->m_classes.end() != behaviorContext->m_classes.find("AZStd::vector<JobProduct, allocator>"));
    }

    TEST_F(AssetBehaviorContextTest, DetectBehaviorProcessJobRequest)
    {
        auto&& behaviorContext = m_app->GetBehaviorContext();
        auto behaviorClassEntry = behaviorContext->m_classes.find("ProcessJobRequest");
        EXPECT_TRUE(behaviorContext->m_classes.end() != behaviorClassEntry);
        AZ::BehaviorClass* behaviorClass = behaviorClassEntry->second;
        EXPECT_TRUE(IsBehaviorFlaggedForEditor(behaviorClass->m_attributes));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("sourceFile"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("watchFolder"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("fullPath"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("builderGuid"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("jobDescription"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("tempDirPath"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("platformInfo"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("sourceFileDependencyList"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("sourceFileUUID"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("jobId"));
        EXPECT_EQ(0, behaviorClass->m_constructors.size());

        EXPECT_TRUE(behaviorContext->m_classes.end() != behaviorContext->m_classes.find("AZStd::vector<SourceFileDependency, allocator>"));
    }

    TEST_F(AssetBehaviorContextTest, DetectBehaviorSourceFileDependency)
    {
        auto&& behaviorContext = m_app->GetBehaviorContext();
        auto behaviorClassEntry = behaviorContext->m_classes.find("SourceFileDependency");
        EXPECT_TRUE(behaviorContext->m_classes.end() != behaviorClassEntry);
        AZ::BehaviorClass* behaviorClass = behaviorClassEntry->second;
        EXPECT_TRUE(IsBehaviorFlaggedForEditor(behaviorClass->m_attributes));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("sourceFileDependencyPath"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("sourceFileDependencyUUID"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("sourceDependencyType"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("Absolute"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("Wildcards"));
        EXPECT_EQ(2, behaviorClass->m_constructors.size());
    }

    TEST_F(AssetBehaviorContextTest, DetectBehaviorAssetBuilderDesc)
    {
        auto&& behaviorContext = m_app->GetBehaviorContext();
        auto behaviorClassEntry = behaviorContext->m_classes.find("AssetBuilderDesc");
        EXPECT_TRUE(behaviorContext->m_classes.end() != behaviorClassEntry);
        AZ::BehaviorClass* behaviorClass = behaviorClassEntry->second;
        EXPECT_TRUE(IsBehaviorFlaggedForEditor(behaviorClass->m_attributes));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("analysisFingerprint"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("busId"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("flags"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("name"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("patterns"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("version"));
        EXPECT_EQ(1, behaviorClass->m_constructors.size());
    }

    TEST_F(AssetBehaviorContextTest, DetectBehaviorCreateJobsResponse)
    {
        auto&& behaviorContext = m_app->GetBehaviorContext();
        auto behaviorClassEntry = behaviorContext->m_classes.find("CreateJobsResponse");
        EXPECT_TRUE(behaviorContext->m_classes.end() != behaviorClassEntry);
        AZ::BehaviorClass* behaviorClass = behaviorClassEntry->second;
        EXPECT_TRUE(IsBehaviorFlaggedForEditor(behaviorClass->m_attributes));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("result"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("sourceFileDependencyList"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("createJobOutputs"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("ResultFailed"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("ResultShuttingDown"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("ResultSuccess"));
        EXPECT_EQ(0, behaviorClass->m_constructors.size());
    }

    TEST_F(AssetBehaviorContextTest, DetectBehaviorCreateJobsRequest)
    {
        auto&& behaviorContext = m_app->GetBehaviorContext();
        auto behaviorClassEntry = behaviorContext->m_classes.find("CreateJobsRequest");
        EXPECT_TRUE(behaviorContext->m_classes.end() != behaviorClassEntry);
        AZ::BehaviorClass* behaviorClass = behaviorClassEntry->second;
        EXPECT_TRUE(IsBehaviorFlaggedForEditor(behaviorClass->m_attributes));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("builderId"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("watchFolder"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("sourceFile"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("sourceFileUUID"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("enabledPlatforms"));
        EXPECT_EQ(0, behaviorClass->m_constructors.size());
    }

    TEST_F(AssetBehaviorContextTest, DetectBehaviorProductPathDependency)
    {
        auto&& behaviorContext = m_app->GetBehaviorContext();
        auto behaviorClassEntry = behaviorContext->m_classes.find("ProductPathDependency");
        EXPECT_TRUE(behaviorContext->m_classes.end() != behaviorClassEntry);
        AZ::BehaviorClass* behaviorClass = behaviorClassEntry->second;
        EXPECT_TRUE(IsBehaviorFlaggedForEditor(behaviorClass->m_attributes));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("dependencyPath"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("dependencyType"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("ProductFile"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("SourceFile"));
        EXPECT_EQ(0, behaviorClass->m_constructors.size());

        EXPECT_TRUE(behaviorContext->m_classes.end() != behaviorContext->m_classes.find("AZStd::vector<ProductPathDependency, allocator>"));
    }

    TEST_F(AssetBehaviorContextTest, DetectBehaviorProcessJobResponse)
    {
        auto&& behaviorContext = m_app->GetBehaviorContext();
        auto behaviorClassEntry = behaviorContext->m_classes.find("ProcessJobResponse");
        EXPECT_TRUE(behaviorContext->m_classes.end() != behaviorClassEntry);
        AZ::BehaviorClass* behaviorClass = behaviorClassEntry->second;
        EXPECT_TRUE(IsBehaviorFlaggedForEditor(behaviorClass->m_attributes));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("outputProducts"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("resultCode"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("requiresSubIdGeneration"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("sourcesToReprocess"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("Success"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("Failed"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("Crashed"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("Cancelled"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("NetworkIssue"));
        EXPECT_EQ(0, behaviorClass->m_constructors.size());
    }

    TEST_F(AssetBehaviorContextTest, DetectBehaviorRegisterBuilderResponse)
    {
        auto&& behaviorContext = m_app->GetBehaviorContext();
        auto behaviorClassEntry = behaviorContext->m_classes.find("RegisterBuilderResponse");
        EXPECT_TRUE(behaviorContext->m_classes.end() != behaviorClassEntry);
        AZ::BehaviorClass* behaviorClass = behaviorClassEntry->second;
        EXPECT_TRUE(IsBehaviorFlaggedForEditor(behaviorClass->m_attributes));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("assetBuilderDescList"));
        EXPECT_EQ(1, behaviorClass->m_constructors.size());

        EXPECT_TRUE(behaviorContext->m_classes.end() != behaviorContext->m_classes.find("AZStd::vector<AssetBuilderDesc, allocator>"));
    }

    TEST_F(AssetBehaviorContextTest, DetectBehaviorRegisterBuilderRequest)
    {
        auto&& behaviorContext = m_app->GetBehaviorContext();
        auto behaviorClassEntry = behaviorContext->m_classes.find("RegisterBuilderRequest");
        EXPECT_TRUE(behaviorContext->m_classes.end() != behaviorClassEntry);
        AZ::BehaviorClass* behaviorClass = behaviorClassEntry->second;
        EXPECT_TRUE(IsBehaviorFlaggedForEditor(behaviorClass->m_attributes));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("filePath"));
        EXPECT_EQ(0, behaviorClass->m_constructors.size());
    }

    TEST_F(AssetBehaviorContextTest, DetectBehaviorJobDependency)
    {
        auto&& behaviorContext = m_app->GetBehaviorContext();
        auto behaviorClassEntry = behaviorContext->m_classes.find("JobDependency");
        EXPECT_TRUE(behaviorContext->m_classes.end() != behaviorClassEntry);
        AZ::BehaviorClass* behaviorClass = behaviorClassEntry->second;
        EXPECT_TRUE(IsBehaviorFlaggedForEditor(behaviorClass->m_attributes));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("sourceFile"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("jobKey"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("platformIdentifier"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("type"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("Fingerprint"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("Order"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("OrderOnce"));
        EXPECT_EQ(0, behaviorClass->m_constructors.size());

        EXPECT_TRUE(behaviorContext->m_classes.end() != behaviorContext->m_classes.find("AZStd::vector<JobDependency, allocator>"));
    }

    TEST_F(AssetBehaviorContextTest, DetectBehaviorPlatformInfo)
    {
        auto&& behaviorContext = m_app->GetBehaviorContext();
        auto behaviorClassEntry = behaviorContext->m_classes.find("PlatformInfo");
        EXPECT_TRUE(behaviorContext->m_classes.end() != behaviorClassEntry);
        AZ::BehaviorClass* behaviorClass = behaviorClassEntry->second;
        EXPECT_TRUE(IsBehaviorFlaggedForEditor(behaviorClass->m_attributes));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("identifier"));
        EXPECT_TRUE(behaviorClass->m_properties.end() != behaviorClass->m_properties.find("tags"));
        EXPECT_EQ(0, behaviorClass->m_constructors.size());

        EXPECT_TRUE(behaviorContext->m_classes.end() != behaviorContext->m_classes.find("AZStd::vector<PlatformInfo, allocator>"));
    }

    template <typename T>
    bool EnumClassReadUpdateTest(AZ::BehaviorProperty* behaviorProperty, AZ::BehaviorObject& instance, T value)
    {
        T enumClassTypeValue = {};
        EXPECT_TRUE(behaviorProperty->m_setter->Invoke(instance, value));
        EXPECT_TRUE(behaviorProperty->m_getter->InvokeResult(enumClassTypeValue, instance));
        EXPECT_EQ(value, enumClassTypeValue);
        return value == enumClassTypeValue;
    }

    TEST_F(AssetBehaviorContextTest, DISABLED_EnumClass_ProductPathDependencyType_Accessible)
    {
        using namespace AssetBuilderSDK;

        auto&& behaviorContext = m_app->GetBehaviorContext();
        auto&& behaviorClassEntry = behaviorContext->m_classes.find("ProductPathDependency");
        auto&& behaviorClass = behaviorClassEntry->second;
        auto&& behaviorProperty = behaviorClass->m_properties["dependencyType"];
        auto instance = behaviorClass->Create();

        EXPECT_TRUE(EnumClassReadUpdateTest(behaviorProperty, instance, ProductPathDependencyType::ProductFile));
        EXPECT_TRUE(EnumClassReadUpdateTest(behaviorProperty, instance, ProductPathDependencyType::SourceFile));

        behaviorClass->Destroy(instance);
    }

    TEST_F(AssetBehaviorContextTest, DISABLED_EnumClass_AssetBuilderPatternPatternType_Accessible)
    {
        using namespace AssetBuilderSDK;

        auto&& behaviorContext = m_app->GetBehaviorContext();
        auto&& behaviorClassEntry = behaviorContext->m_classes.find("AssetBuilderPattern");
        auto&& behaviorClass = behaviorClassEntry->second;
        auto&& behaviorProperty = behaviorClass->m_properties["type"];
        auto instance = behaviorClass->Create();

        EXPECT_TRUE(EnumClassReadUpdateTest(behaviorProperty, instance, AssetBuilderPattern::PatternType::Wildcard));
        EXPECT_TRUE(EnumClassReadUpdateTest(behaviorProperty, instance, AssetBuilderPattern::PatternType::Regex));

        behaviorClass->Destroy(instance);
    }

    TEST_F(AssetBehaviorContextTest, DISABLED_EnumClass_ProcessJobResponse_Accessible)
    {
        using namespace AssetBuilderSDK;

        auto&& behaviorContext = m_app->GetBehaviorContext();
        auto&& behaviorClassEntry = behaviorContext->m_classes.find("ProcessJobResponse");
        auto&& behaviorClass = behaviorClassEntry->second;
        auto&& behaviorProperty = behaviorClass->m_properties["resultCode"];
        auto instance = behaviorClass->Create();

        EXPECT_TRUE(EnumClassReadUpdateTest(behaviorProperty, instance, ProcessJobResultCode::ProcessJobResult_Success));
        EXPECT_TRUE(EnumClassReadUpdateTest(behaviorProperty, instance, ProcessJobResultCode::ProcessJobResult_Failed));
        EXPECT_TRUE(EnumClassReadUpdateTest(behaviorProperty, instance, ProcessJobResultCode::ProcessJobResult_Crashed));
        EXPECT_TRUE(EnumClassReadUpdateTest(behaviorProperty, instance, ProcessJobResultCode::ProcessJobResult_Cancelled));
        EXPECT_TRUE(EnumClassReadUpdateTest(behaviorProperty, instance, ProcessJobResultCode::ProcessJobResult_NetworkIssue));

        behaviorClass->Destroy(instance);
    }

    TEST_F(AssetBehaviorContextTest, DISABLED_EnumClass_JobDependencyType_Accessible)
    {
        using namespace AssetBuilderSDK;

        auto&& behaviorContext = m_app->GetBehaviorContext();
        auto&& behaviorClassEntry = behaviorContext->m_classes.find("JobDependency");
        auto&& behaviorClass = behaviorClassEntry->second;
        auto&& behaviorProperty = behaviorClass->m_properties["type"];
        auto instance = behaviorClass->Create();

        EXPECT_TRUE(EnumClassReadUpdateTest(behaviorProperty, instance, JobDependencyType::Fingerprint));
        EXPECT_TRUE(EnumClassReadUpdateTest(behaviorProperty, instance, JobDependencyType::Order));
        EXPECT_TRUE(EnumClassReadUpdateTest(behaviorProperty, instance, JobDependencyType::OrderOnce));

        behaviorClass->Destroy(instance);
    }

    TEST_F(AssetBehaviorContextTest, DISABLED_EnumClass_CreateJobsResultCode_Accessible)
    {
        using namespace AssetBuilderSDK;

        auto&& behaviorContext = m_app->GetBehaviorContext();
        auto&& behaviorClassEntry = behaviorContext->m_classes.find("CreateJobsResponse");
        auto&& behaviorClass = behaviorClassEntry->second;
        auto&& behaviorProperty = behaviorClass->m_properties["result"];
        auto instance = behaviorClass->Create();

        EXPECT_TRUE(EnumClassReadUpdateTest(behaviorProperty, instance, CreateJobsResultCode::Failed));
        EXPECT_TRUE(EnumClassReadUpdateTest(behaviorProperty, instance, CreateJobsResultCode::ShuttingDown));
        EXPECT_TRUE(EnumClassReadUpdateTest(behaviorProperty, instance, CreateJobsResultCode::Success));

        behaviorClass->Destroy(instance);
    }

    TEST_F(AssetBehaviorContextTest, DISABLED_EnumClass_SourceFileDependency_Accessible)
    {
        using namespace AssetBuilderSDK;

        auto&& behaviorContext = m_app->GetBehaviorContext();
        auto&& behaviorClassEntry = behaviorContext->m_classes.find("SourceFileDependency");
        auto&& behaviorClass = behaviorClassEntry->second;
        auto&& behaviorProperty = behaviorClass->m_properties["sourceDependencyType"];
        auto instance = behaviorClass->Create();

        EXPECT_TRUE(EnumClassReadUpdateTest(behaviorProperty, instance, SourceFileDependency::SourceFileDependencyType::Absolute));
        EXPECT_TRUE(EnumClassReadUpdateTest(behaviorProperty, instance, SourceFileDependency::SourceFileDependencyType::Wildcards));

        behaviorClass->Destroy(instance);
    }
};

