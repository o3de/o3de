/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PrefabBuilderTests.h>
#include <PrefabGroup/tests/PrefabBehaviorTestMocks.h>
#include <PrefabGroup/IPrefabGroup.h>
#include <PrefabGroup/PrefabGroup.h>
#include <PrefabGroup/PrefabGroupBehavior.h>
#include <PrefabGroup/DefaultProceduralPrefab.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/MockIGraphObject.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>

namespace UnitTest
{
    struct DefaultProceduralPrefabGroupTests
        : public PrefabBuilderTests
    {
        void SetUp() override
        {
            using namespace AZ::SceneAPI;
            PrefabBuilderTests::SetUp();

            SceneData::PrefabGroup::Reflect(m_app.GetSerializeContext());
            SceneData::PrefabGroup::Reflect(m_app.GetBehaviorContext());
            DefaultProceduralPrefabGroup::Reflect(m_app.GetBehaviorContext());
            m_app.GetBehaviorContext()->Method("TestExpectTrue", &TestExpectTrue);
            m_app.GetBehaviorContext()->Method("TestEqualNumbers", &TestEqualNumbers);
            m_app.GetBehaviorContext()->Method("TestEqualStrings", &TestEqualStrings);
            ScopeForUnitTest(m_app.GetBehaviorContext()->m_ebuses.find("PrefabGroupNotificationBus")->second->m_attributes);
            ScopeForUnitTest(m_app.GetBehaviorContext()->m_ebuses.find("PrefabGroupEventBus")->second->m_attributes);

            m_editorMeshComponentHelper = AZStd::make_unique<AZ::Render::EditorMeshComponentHelper>();
            m_editorMeshComponentHelper->Reflect(m_app.GetSerializeContext());
            m_editorMeshComponentHelper->Reflect(m_app.GetBehaviorContext());

            AZ::SceneAPI::Containers::Scene::Reflect(m_app.GetBehaviorContext());

            m_scriptContext = AZStd::make_unique<AZ::ScriptContext>();
            m_scriptContext->BindTo(m_app.GetBehaviorContext());
        }

        void TearDown() override
        {
            m_editorMeshComponentHelper.reset();
            m_scriptContext.reset();
            PrefabBuilderTests::TearDown();
        }

        void ScopeForUnitTest(AZ::AttributeArray& attributes)
        {
            using namespace AZ::Script;
            attributes.erase(AZStd::remove_if(attributes.begin(), attributes.end(), [](const AZ::AttributePair& pair)
            {
                return pair.first == Attributes::Scope;
            }));
            auto* attributeData = aznew AZ::Edit::AttributeData<Attributes::ScopeFlags>(Attributes::ScopeFlags::Common);
            attributes.push_back(AZStd::make_pair(Attributes::Scope, attributeData));
        }

        void ExpectExecute(AZStd::string_view script)
        {
            EXPECT_TRUE(m_scriptContext->Execute(script.data()));
        }

        static void TestExpectTrue(bool value)
        {
            EXPECT_TRUE(value);
        }

        static void TestEqualNumbers(AZ::s64 lhs, AZ::s64 rhs)
        {
            EXPECT_EQ(lhs, rhs);
        }

        static void TestEqualStrings(AZStd::string_view lhs, AZStd::string_view rhs)
        {
            EXPECT_STRCASEEQ(lhs.data(), rhs.data());
        }

        AZStd::unique_ptr<AZ::ScriptContext> m_scriptContext;
        AZStd::unique_ptr<AZ::Render::EditorMeshComponentHelper> m_editorMeshComponentHelper;
    };

    TEST_F(DefaultProceduralPrefabGroupTests, ScripContext_PrefabGroupNotificationBus_ClassExists)
    {
        ExpectExecute("handler = PrefabGroupNotificationBus.Connect({})");
        ExpectExecute("TestExpectTrue(handler ~= nil)");
    }

    TEST_F(DefaultProceduralPrefabGroupTests, ScripContext_PrefabGroupEventBus_ClassApiExists)
    {
        ExpectExecute("TestExpectTrue(PrefabGroupEventBus.Broadcast.GeneratePrefabGroupManifestUpdates ~= nil)");
    }

    TEST_F(DefaultProceduralPrefabGroupTests, PrefabGroupEventBus_GeneratePrefabGroupManifestUpdates_HasProceduralMeshGroupRule)
    {
        AZ::SceneAPI::DefaultProceduralPrefabGroup defaultProceduralPrefabGroup;

        auto scene = CreateMockScene();
        AZStd::optional<AZ::SceneAPI::PrefabGroupRequests::ManifestUpdates> manifestUpdates;
        AZ::SceneAPI::PrefabGroupEventBus::BroadcastResult(
            manifestUpdates,
            &AZ::SceneAPI::PrefabGroupEventBus::Events::GeneratePrefabGroupManifestUpdates,
            *scene.get());

        ASSERT_TRUE(manifestUpdates.has_value());

        bool hasProceduralMeshGroupRule = false;
        for (const auto& manifestUpdate : manifestUpdates.value())
        {
            auto* meshGroup = azrtti_cast<AZ::SceneAPI::DataTypes::IMeshGroup*>(manifestUpdate.get());
            if (meshGroup)
            {
                const auto proceduralMeshGroupRule =
                    meshGroup->GetRuleContainer().FindFirstByType<AZ::SceneAPI::SceneData::ProceduralMeshGroupRule>();

                if (proceduralMeshGroupRule)
                {
                    hasProceduralMeshGroupRule = true;
                    break;
                }
            }
        }

        EXPECT_TRUE(hasProceduralMeshGroupRule);
    }
}
