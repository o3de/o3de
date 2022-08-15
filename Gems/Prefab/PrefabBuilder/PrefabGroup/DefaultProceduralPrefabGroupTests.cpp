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
#include <PrefabGroup/PrefabGroupBehavior.h>
#include <PrefabGroup/DefaultProceduralPrefab.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/MockIGraphObject.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ITransform.h>


#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IScriptProcessorRule.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneData/ReflectionRegistrar.h>
#include <SceneAPI/SceneData/Rules/CoordinateSystemRule.h>
#include <SceneAPI/SceneData/Behaviors/ScriptProcessorRuleBehavior.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>

// TODO: move all the test files to /tests folder
// TODO: move mocks to header/cpp 

#ifndef MOCKEDITORMESHCOMPONENT
#define MOCKEDITORMESHCOMPONENT
// a mock AZ::Render::EditorMeshComponent
namespace AZ::Render
{
    struct EditorMeshComponent
        : public AZ::Component
    {
        AZ_COMPONENT(EditorMeshComponent, "{DCE68F6E-2E16-4CB4-A834-B6C2F900A7E9}");
        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<AZ::Render::EditorMeshComponent>();
            }

            if (BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorMeshComponent>("AZ::Render::EditorMeshComponent");
            }

        }
    };

    struct EditorMeshComponentHelper
        : public AZ::ComponentDescriptorHelper<EditorMeshComponent>
    {
        AZ_CLASS_ALLOCATOR(EditorMeshComponentHelper, AZ::SystemAllocator, 0);

        void Reflect(AZ::ReflectContext* reflection) const override
        {
            AZ::Render::EditorMeshComponent::Reflect(reflection);
        }
    };
}
#endif MOCKEDITORMESHCOMPONENT

namespace UnitTest
{
    struct MockTransform
        : public AZ::SceneAPI::DataTypes::ITransform
    {
        AZ::Matrix3x4 m_matrix = AZ::Matrix3x4::CreateIdentity();

        AZ::Matrix3x4& GetMatrix() override
        {
            return m_matrix;
        }

        const AZ::Matrix3x4& GetMatrix() const
        {
            return m_matrix;
        }
    };

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

        static AZStd::unique_ptr<AZ::SceneAPI::Containers::Scene> BuildMockScene()
        {
            using namespace AZ::SceneAPI;
            using namespace AZ::SceneAPI::DataTypes;
            using namespace AZ::SceneData::GraphData;

            /*---------------------------------------\
                        Root
                         |
                         1
                         |
                         2
                       /   \
                ------3m    7
               /  /  /        \
              6  5  4t         8m-------
                                \   \   \
                                 9t 10  11
            \---------------------------------------*/

            auto scene = AZStd::make_unique<AZ::SceneAPI::Containers::Scene>("mock_scene");
            auto root = scene->GetGraph().GetRoot();
            auto index1 = scene->GetGraph().AddChild(root, "1", AZStd::make_shared<MockIGraphObject>(1));
            auto index2 = scene->GetGraph().AddChild(index1, "2", AZStd::make_shared<MockIGraphObject>(2));
            auto index3 = scene->GetGraph().AddChild(index2, "3", AZStd::make_shared<MeshData>());
            auto index4 = scene->GetGraph().AddChild(index3, "4", AZStd::make_shared<MockTransform>());
            auto index5 = scene->GetGraph().AddChild(index3, "5", AZStd::make_shared<MockIGraphObject>(5));
            auto index6 = scene->GetGraph().AddChild(index3, "6", AZStd::make_shared<MockIGraphObject>(6));
            auto index7 = scene->GetGraph().AddChild(index2, "7", AZStd::make_shared<MockIGraphObject>(7));
            auto index8 = scene->GetGraph().AddChild(index7, "8", AZStd::make_shared<MeshData>());
            auto index9 = scene->GetGraph().AddChild(index8, "9", AZStd::make_shared<MockTransform>());
            auto index10 = scene->GetGraph().AddChild(index8, "10", AZStd::make_shared<MockIGraphObject>(10));
            auto index11 = scene->GetGraph().AddChild(index8, "11", AZStd::make_shared<MockIGraphObject>(11));

            scene->GetGraph().MakeEndPoint(index4);
            scene->GetGraph().MakeEndPoint(index5);
            scene->GetGraph().MakeEndPoint(index6);
            scene->GetGraph().MakeEndPoint(index9);
            scene->GetGraph().MakeEndPoint(index10);
            scene->GetGraph().MakeEndPoint(index11);

            scene->SetSource("filename", AZ::Uuid::CreateName(".fake"));

            return AZStd::move(scene);
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

    TEST_F(DefaultProceduralPrefabGroupTests, PrefabGroupEventBus_GeneratePrefabGroupManifestUpdates_Works)
    {
        AZ::SceneAPI::DefaultProceduralPrefabGroup defaultProceduralPrefabGroup;

        auto scene = BuildMockScene();
        AZStd::optional<AZ::SceneAPI::PrefabGroupEvents::ManifestUpdates> result;
        AZ::SceneAPI::PrefabGroupEventBus::BroadcastResult(
            result,
            &AZ::SceneAPI::PrefabGroupEventBus::Events::GeneratePrefabGroupManifestUpdates,
            *scene.get());

        ASSERT_TRUE(result.has_value());
        ASSERT_FALSE(result.value().empty());
    }
}
