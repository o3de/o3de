/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <Entity/EntityUtilityComponent.h>

#include <Prefab/PrefabTestComponent.h>
#include <Prefab/PrefabTestDomUtils.h>
#include <Prefab/PrefabTestFixture.h>
#include <Prefab/Procedural/ProceduralPrefabAsset.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzToolsFramework/Prefab/Template/Template.h>

namespace UnitTest
{
    class ProceduralPrefabAssetTest
        : public PrefabTestFixture
    {
        void SetUpEditorFixtureImpl() override
        {
            AZ::ComponentApplicationRequests* componentApplicationRequests = AZ::Interface<AZ::ComponentApplicationRequests>::Get();
            ASSERT_NE(componentApplicationRequests, nullptr);

            auto* behaviorContext = componentApplicationRequests->GetBehaviorContext();
            ASSERT_NE(behaviorContext, nullptr);

            auto* jsonRegistrationContext = componentApplicationRequests->GetJsonRegistrationContext();
            ASSERT_NE(jsonRegistrationContext, nullptr);

            auto* serializeContext = componentApplicationRequests->GetSerializeContext();
            ASSERT_NE(serializeContext, nullptr);

            AZ::Prefab::ProceduralPrefabAsset::Reflect(serializeContext);
            AZ::Prefab::ProceduralPrefabAsset::Reflect(behaviorContext);
            AZ::Prefab::ProceduralPrefabAsset::Reflect(jsonRegistrationContext);
        }

        void TearDownEditorFixtureImpl() override
        {
            AZ::ComponentApplicationRequests* componentApplicationRequests = AZ::Interface<AZ::ComponentApplicationRequests>::Get();
            componentApplicationRequests->GetJsonRegistrationContext()->EnableRemoveReflection();
            AZ::Prefab::ProceduralPrefabAsset::Reflect(componentApplicationRequests->GetJsonRegistrationContext());
        }
    };

    TEST_F(ProceduralPrefabAssetTest, ReflectContext_AccessMethods_Works)
    {
        AZ::ComponentApplicationRequests* componentApplicationRequests = AZ::Interface<AZ::ComponentApplicationRequests>::Get();

        auto* serializeContext = componentApplicationRequests->GetSerializeContext();
        EXPECT_TRUE(!serializeContext->CreateAny(azrtti_typeid<AZ::Prefab::ProceduralPrefabAsset>()).empty());
        EXPECT_TRUE(!serializeContext->CreateAny(azrtti_typeid<AZ::Prefab::PrefabDomData>()).empty());

        auto* jsonRegistrationContext = componentApplicationRequests->GetJsonRegistrationContext();
        EXPECT_TRUE(jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::Prefab::PrefabDomDataJsonSerializer>()));
    }

    TEST_F(ProceduralPrefabAssetTest, ProceduralPrefabAsset_AccessMethods_Works)
    {
        const auto templateId = TemplateId(1);
        const auto prefabString = "fake.prefab";

        AZ::Prefab::ProceduralPrefabAsset asset{};
        asset.SetTemplateId(templateId);
        EXPECT_EQ(asset.GetTemplateId(), templateId);

        asset.SetTemplateName(prefabString);
        EXPECT_EQ(asset.GetTemplateName(), prefabString);
    }

    TEST_F(ProceduralPrefabAssetTest, PrefabDomData_AccessMethods_Works)
    {
        AzToolsFramework::Prefab::PrefabDom dom;
        dom.SetObject();
        dom.AddMember("boolValue", true, dom.GetAllocator());

        AZ::Prefab::PrefabDomData prefabDomData;
        prefabDomData.CopyValue(dom);

        const AzToolsFramework::Prefab::PrefabDom& result = prefabDomData.GetValue();
        EXPECT_TRUE(result.HasMember("boolValue"));
        EXPECT_TRUE(result.FindMember("boolValue")->value.GetBool());
    }

    TEST_F(ProceduralPrefabAssetTest, PrefabDomDataJsonSerializer_Load_Works)
    {
        AZ::Prefab::PrefabDomData prefabDomData;

        AzToolsFramework::Prefab::PrefabDom dom;
        dom.SetObject();
        dom.AddMember("member", "value", dom.GetAllocator());

        AZ::Prefab::PrefabDomDataJsonSerializer prefabDomDataJsonSerializer;
        AZ::JsonDeserializerSettings settings;
        settings.m_reporting = [](auto, auto, auto)
        {
            AZ::JsonSerializationResult::ResultCode result(AZ::JsonSerializationResult::Tasks::ReadField);
            return result;
        };
        AZ::JsonDeserializerContext context{ settings };

        auto result = prefabDomDataJsonSerializer.Load(&prefabDomData, azrtti_typeid(prefabDomData), dom, context);
        EXPECT_EQ(result.GetResultCode().GetOutcome(), AZ::JsonSerializationResult::Outcomes::DefaultsUsed);
        EXPECT_TRUE(prefabDomData.GetValue().HasMember("member"));
        EXPECT_STREQ(prefabDomData.GetValue().FindMember("member")->value.GetString(), "value");
    }

    TEST_F(ProceduralPrefabAssetTest, PrefabDomDataJsonSerializer_Store_Works)
    {
        AzToolsFramework::Prefab::PrefabDom dom;
        dom.SetObject();
        dom.AddMember("member", "value", dom.GetAllocator());

        AZ::Prefab::PrefabDomData prefabDomData;
        prefabDomData.CopyValue(dom);

        AZ::Prefab::PrefabDomDataJsonSerializer prefabDomDataJsonSerializer;
        AzToolsFramework::Prefab::PrefabDom outputValue;
        AZ::JsonSerializerSettings settings;
        settings.m_reporting = [](auto, auto, auto)
        {
            AZ::JsonSerializationResult::ResultCode result(AZ::JsonSerializationResult::Tasks::WriteValue);
            return result;
        };
        AZ::JsonSerializerContext context{ settings, outputValue.GetAllocator() };
        auto result = prefabDomDataJsonSerializer.Store(outputValue, &prefabDomData, nullptr, azrtti_typeid(prefabDomData), context);
        EXPECT_EQ(result.GetResultCode().GetOutcome(), AZ::JsonSerializationResult::Outcomes::DefaultsUsed);
        EXPECT_TRUE(outputValue.HasMember("member"));
        EXPECT_STREQ(outputValue.FindMember("member")->value.GetString(), "value");
    }

    TEST_F(ProceduralPrefabAssetTest, Template_IsProcPrefab_DefaultsToNotProcPrefab)
    {
        AzToolsFramework::Prefab::PrefabDom dom;
        dom.SetObject();
        dom.AddMember("Source", "foo.prefab", dom.GetAllocator());
        AzToolsFramework::Prefab::Template fooTemplate("foo", AZStd::move(dom));
        EXPECT_FALSE(fooTemplate.IsProcedural());
    }

    TEST_F(ProceduralPrefabAssetTest, Template_IsProcPrefab_DomDrivesFlagToTrue)
    {
        AzToolsFramework::Prefab::PrefabDom dom;
        dom.SetObject();
        dom.AddMember("Source", "foo.procprefab", dom.GetAllocator());
        AzToolsFramework::Prefab::Template fooTemplate("foo", AZStd::move(dom));
        EXPECT_TRUE(fooTemplate.IsProcedural());
        // the second time should use the cached version of the flag
        EXPECT_TRUE(fooTemplate.IsProcedural());
    }

    TEST_F(ProceduralPrefabAssetTest, Template_IsProcPrefab_FailsWithNoSource)
    {
        AzToolsFramework::Prefab::PrefabDom dom;
        dom.SetObject();
        AzToolsFramework::Prefab::Template fooTemplate("foo", AZStd::move(dom));
        EXPECT_FALSE(fooTemplate.IsProcedural());
    }
}
