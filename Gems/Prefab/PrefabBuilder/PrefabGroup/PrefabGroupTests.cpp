/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PrefabBuilderTests.h>
#include <PrefabGroup/IPrefabGroup.h>
#include <PrefabGroup/PrefabGroup.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzFramework/FileFunc/FileFunc.h>

namespace UnitTest
{
    TEST_F(PrefabBuilderTests, PrefabGroup_FindsRequiredReflection_True)
    {
        using namespace AZ::SceneAPI;
        auto* serializeContext = m_app.GetSerializeContext();
        ASSERT_NE(nullptr, serializeContext);
        SceneData::PrefabGroup::Reflect(serializeContext);
        SceneData::PrefabGroup::Reflect(m_app.GetJsonRegistrationContext());
        ASSERT_NE(nullptr, serializeContext->FindClassData(azrtti_typeid<DataTypes::IPrefabGroup>()));
        ASSERT_NE(nullptr, serializeContext->FindClassData(azrtti_typeid<SceneData::PrefabGroup>()));

        auto findElementWithName = [](const AZ::SerializeContext::ClassData* classData, const char* name)
        {
            auto it = AZStd::find_if(classData->m_elements.begin(), classData->m_elements.end(), [name](const auto& element)
            {
                return strcmp(element.m_name, name) == 0;
            });
            return it != classData->m_elements.end();
        };

        auto* prefabGroupClassData = serializeContext->FindClassData(azrtti_typeid<SceneData::PrefabGroup>());
        EXPECT_TRUE(findElementWithName(prefabGroupClassData, "name"));
        EXPECT_TRUE(findElementWithName(prefabGroupClassData, "nodeSelectionList"));
        EXPECT_TRUE(findElementWithName(prefabGroupClassData, "rules"));
        EXPECT_TRUE(findElementWithName(prefabGroupClassData, "id"));
        EXPECT_TRUE(findElementWithName(prefabGroupClassData, "prefabDomData"));

        m_app.GetJsonRegistrationContext()->EnableRemoveReflection();
        SceneData::PrefabGroup::Reflect(m_app.GetJsonRegistrationContext());
    }

    TEST_F(PrefabBuilderTests, PrefabGroup_JsonWithPrefabArbitraryPrefab_Works)
    {
        namespace JSR = AZ::JsonSerializationResult;
        using namespace AZ::SceneAPI;
        auto* serializeContext = m_app.GetSerializeContext();
        ASSERT_NE(nullptr, serializeContext);
        SceneData::PrefabGroup::Reflect(serializeContext);
        AZ::Prefab::ProceduralPrefabAsset::Reflect(serializeContext);
        SceneData::PrefabGroup::Reflect(m_app.GetJsonRegistrationContext());
        AZ::Prefab::ProceduralPrefabAsset::Reflect(m_app.GetJsonRegistrationContext());

        // fill out a PrefabGroup using JSON
        AZStd::string_view input = R"JSON(
        {
            "name" : "tester",
            "id" : "{49698DBC-B447-49EF-9B56-25BB29342AFB}",
            "prefabDomData" : {"foo": "bar"}
        })JSON";

        rapidjson::Document document;
        document.Parse<rapidjson::kParseCommentsFlag>(input.data(), input.size());
        ASSERT_FALSE(document.HasParseError());

        SceneData::PrefabGroup instancePrefabGroup;
        EXPECT_EQ(AZ::JsonSerialization::Load(instancePrefabGroup, document).GetOutcome(), JSR::Outcomes::PartialDefaults);

        ASSERT_TRUE(instancePrefabGroup.GetPrefabDomRef().has_value());
        const AzToolsFramework::Prefab::PrefabDom& dom = instancePrefabGroup.GetPrefabDomRef().value();
        EXPECT_TRUE(dom.IsObject());
        EXPECT_TRUE(dom.GetObject().HasMember("foo"));
        EXPECT_STREQ(dom.GetObject().FindMember("foo")->value.GetString(), "bar");
        EXPECT_STREQ(instancePrefabGroup.GetName().c_str(), "tester");
        EXPECT_STREQ(instancePrefabGroup.GetId().ToString<AZStd::string>().c_str(), "{49698DBC-B447-49EF-9B56-25BB29342AFB}");

        m_app.GetJsonRegistrationContext()->EnableRemoveReflection();
        SceneData::PrefabGroup::Reflect(m_app.GetJsonRegistrationContext());
        AZ::Prefab::ProceduralPrefabAsset::Reflect(m_app.GetJsonRegistrationContext());
    }

    TEST_F(PrefabBuilderTests, PrefabGroup_InvalidPrefabJson_Detected)
    {
        using namespace AZ::SceneAPI;

        AZStd::string_view input = R"JSON(
        {
            bad json that will not parse
        })JSON";

        rapidjson::Document document;
        document.Parse<rapidjson::kParseCommentsFlag>(input.data(), input.size());

        SceneData::PrefabGroup prefabGroup;
        prefabGroup.SetId(AZ::Uuid::CreateRandom());
        prefabGroup.SetName("tester");
        prefabGroup.SetPrefabDom(AZStd::move(document));

        const AzToolsFramework::Prefab::PrefabDom& dom = prefabGroup.GetPrefabDomRef().value();
        EXPECT_TRUE(dom.IsNull());
        EXPECT_STREQ("tester", prefabGroup.GetName().c_str());
    }

    struct PrefabBuilderBehaviorTests
        : public PrefabBuilderTests
    {
        void SetUp() override
        {
            using namespace AZ::SceneAPI;

            PrefabBuilderTests::SetUp();
            SceneData::PrefabGroup::Reflect(m_app.GetSerializeContext());
            SceneData::PrefabGroup::Reflect(m_app.GetBehaviorContext());
            SceneData::PrefabGroup::Reflect(m_app.GetJsonRegistrationContext());
            m_scriptContext = AZStd::make_unique<AZ::ScriptContext>();
            m_scriptContext->BindTo(m_app.GetBehaviorContext());
        }

        void TearDown() override
        {
            using namespace AZ::SceneAPI;
            m_app.GetJsonRegistrationContext()->EnableRemoveReflection();
            SceneData::PrefabGroup::Reflect(m_app.GetJsonRegistrationContext());

            m_scriptContext.reset();
            PrefabBuilderTests::TearDown();
        }

        void ExpectExecute(AZStd::string_view script)
        {
            EXPECT_TRUE(m_scriptContext->Execute(script.data()));
        }

        AZStd::unique_ptr<AZ::ScriptContext> m_scriptContext;
    };

    TEST_F(PrefabBuilderBehaviorTests, PrefabGroup_PrefabGroupClass_Exists)
    {
        ExpectExecute("group = PrefabGroup()");
        ExpectExecute("assert(group)");
        ExpectExecute("assert(group.name)");
        ExpectExecute("assert(group.id)");
        ExpectExecute("assert(group.prefabDomData)");
    }

    TEST_F(PrefabBuilderBehaviorTests, PrefabGroup_PrefabGroupAssignment_Works)
    {
        ExpectExecute("group = PrefabGroup()");
        ExpectExecute("group.name = 'tester'");
        ExpectExecute("group.id = Uuid.CreateString('{AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE}', 0)");
        ExpectExecute("group.prefabDomData = '{\"foo\": \"bar\"}'");
        ExpectExecute("assert(group.name == 'tester')");
        ExpectExecute("assert(tostring(group.id) == '{AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE}')");
        ExpectExecute("assert(group.prefabDomData == '{\\n    \"foo\": \"bar\"\\n}')");
    }
}
