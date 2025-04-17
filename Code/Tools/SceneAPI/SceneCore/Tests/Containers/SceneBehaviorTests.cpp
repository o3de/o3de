/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/ReflectionManager.h>
#include <AzCore/Math/MathReflection.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>

#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/MockIGraphObject.h>
#include <SceneAPI/SceneCore/Mocks/MockBehaviorUtils.h>

// the DLL entry point for SceneCore to reflect its behavior context
extern "C" AZ_DLL_EXPORT void ReflectBehavior(AZ::BehaviorContext* context);

// the DLL entry point for SceneCore to reflect its serialize context
extern "C" AZ_DLL_EXPORT void ReflectTypes(AZ::SerializeContext* context);

extern "C" AZ_DLL_EXPORT void CleanUpSceneCoreGenericClassInfo();

namespace AZ::SceneAPI::Containers
{
    class MockManifestRule : public DataTypes::IManifestObject
    {
    public:
        AZ_RTTI(MockManifestRule, "{D6F96B48-4E6F-4EE8-A5A3-959B76F90DA8}", IManifestObject);
        AZ_CLASS_ALLOCATOR(MockManifestRule, AZ::SystemAllocator);

        MockManifestRule() = default;

        MockManifestRule(double value)
            : m_value(value)
        {
        }

        double GetValue() const
        {
            return m_value;
        }

        void SetValue(double value)
        {
            m_value = value;
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<MockManifestRule, IManifestObject>()
                    ->Version(1)
                    ->Field("value", &MockManifestRule::m_value);
            }
        }

    private:
        double m_value = 0.0;
    };

    class MockIGraphObjectTester final
        : public DataTypes::IGraphObject
    {
    public:
        AZ_RTTI(MockIGraphObjectTester, "{E112D82D-D98C-4506-9495-1E4254FD6335}", DataTypes::IGraphObject);

        MockIGraphObjectTester() = default;
        ~MockIGraphObjectTester() override = default;
        void CloneAttributesFrom([[maybe_unused]] const IGraphObject* sourceObject) override {}

        int m_buffer[64] = {1,2,3};
        AZStd::string m_string {"test text buffer"};
    };

    struct MockBuilder final
    {
        AZ_TYPE_INFO(MockBuilder, "{ECF0FB2C-E5C0-4B89-993C-8511A7EF6894}");

        AZStd::unique_ptr<AZ::SceneAPI::Containers::Scene> m_scene;

        MockBuilder()
        {
            m_scene = AZStd::make_unique<AZ::SceneAPI::Containers::Scene>("unit_scene");
        }

        ~MockBuilder()
        {
            m_scene.reset();
        }

        void BuildSceneGraph()
        {
            m_scene->SetManifestFilename("manifest_filename");
            m_scene->SetSource("unit_source_filename", azrtti_typeid<AZ::SceneAPI::Containers::Scene>());

            auto& graph = m_scene->GetGraph();

            /*----------------------------\
            |            Root             |
            |         /       \           |
            |        |         |          |
            |        A         B          |
            |        |        /|\         |
            |        C       I J K        |
            |      / | \          \       |
            |     D  E  F          L      |
            |       / \                   |
            |      G   H                  |
            \----------------------------*/

            //Build up the graph
            const auto indexA = graph.AddChild(graph.GetRoot(), "A", AZStd::make_shared<DataTypes::MockIGraphObject>(1));
            const auto indexC = graph.AddChild(indexA, "C", AZStd::make_shared<DataTypes::MockIGraphObject>(3));
            const auto indexE = graph.AddChild(indexC, "E", AZStd::make_shared<DataTypes::MockIGraphObject>(4));
            graph.AddChild(indexC, "D", AZStd::make_shared<DataTypes::MockIGraphObject>(5));
            graph.AddChild(indexC, "F", AZStd::make_shared<DataTypes::MockIGraphObject>(6));
            graph.AddChild(indexE, "G", AZStd::make_shared<DataTypes::MockIGraphObject>(7));
            graph.AddChild(indexE, "H", AZStd::make_shared<DataTypes::MockIGraphObject>(8));
            const auto indexB = graph.AddChild(graph.GetRoot(), "B", AZStd::make_shared<DataTypes::MockIGraphObject>(2));
            const auto indexK = graph.AddChild(indexB, "K", AZStd::make_shared<DataTypes::MockIGraphObject>(2));
            graph.AddChild(indexB, "I", AZStd::make_shared<DataTypes::MockIGraphObject>(9));
            graph.AddChild(indexB, "J", AZStd::make_shared<DataTypes::MockIGraphObject>(10));
            graph.AddChild(indexK, "L", AZStd::make_shared<MockIGraphObjectTester>());

            m_scene->GetManifest().AddEntry(AZStd::make_shared<MockManifestRule>(0.1));
            m_scene->GetManifest().AddEntry(AZStd::make_shared<MockManifestRule>(2.3));
            m_scene->GetManifest().AddEntry(AZStd::make_shared<MockManifestRule>(4.5));
        }

        static void Reflect(ReflectContext* context)
        {
            BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<MockBuilder>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Module, "scene")
                    ->Method("BuildSceneGraph", [](MockBuilder& self)
                    {
                        return self.BuildSceneGraph();
                    })
                    ->Method("GetScene", [](MockBuilder& self)
                    {
                        return self.m_scene.get();
                    });

            }
        }
    };

    class SceneGraphBehaviorTest
        : public ::testing::Test
    {
    public:
        void SetUp() override
        {
            m_behaviorContext = AZStd::make_unique<AZ::BehaviorContext>();
            ReflectBehavior(m_behaviorContext.get());
        }

        void TearDown() override
        {
            m_behaviorContext.reset();
        }

        AZ::BehaviorClass* GetBehaviorClass(const AZ::TypeId& behaviorClassType)
        {
            auto entry = m_behaviorContext->m_typeToClassMap.find(behaviorClassType);
            return (entry != m_behaviorContext->m_typeToClassMap.end()) ? entry->second : nullptr;
        }

        AZ::BehaviorProperty* GetBehaviorProperty(AZ::BehaviorClass& behaviorClass, AZStd::string_view propertyName)
        {
            auto entry = behaviorClass.m_properties.find(propertyName);
            return (entry != behaviorClass.m_properties.end()) ? entry->second : nullptr;
        }

        bool HasBehaviorClass(const AZ::TypeId& behaviorClassType)
        {
            return GetBehaviorClass(behaviorClassType) != nullptr;
        }

        bool HasProperty(AZ::BehaviorClass& behaviorClass, AZStd::string_view propertyName, const AZ::TypeId& propertyClassType)
        {
            AZ::BehaviorProperty* behaviorProperty = GetBehaviorProperty(behaviorClass, propertyName);
            if (behaviorProperty)
            {
                return behaviorProperty->m_getter->GetResult()->m_typeId == propertyClassType;
            }
            return false;
        }

        using ArgList = AZStd::vector<AZ::TypeId>;

        bool HasMethodWithInput(AZ::BehaviorClass& behaviorClass, AZStd::string_view methodName, const ArgList& input)
        {
            auto entry = behaviorClass.m_methods.find(methodName);
            if (entry == behaviorClass.m_methods.end())
            {
                return false;
            }
            AZ::BehaviorMethod* method = entry->second;

            const size_t methodArgsCount = method->IsMember() ? method->GetNumArguments() - 1 : method->GetNumArguments();
            if (input.size() != methodArgsCount)
            {
                return false;
            }

            for (size_t argIndex = 0; argIndex < input.size(); ++argIndex)
            {
                const size_t thisPointerOffset = method->IsMember() ? 1 : 0;
                const auto argType = method->GetArgument(argIndex + thisPointerOffset)->m_typeId;
                const auto inputType = input[argIndex];
                if (inputType != argType)
                {
                    return false;
                }
            }
            return true;
        }

        bool HasMethodWithOutput(AZ::BehaviorClass& behaviorClass, AZStd::string_view methodName, const AZ::TypeId& output, const ArgList& input)
        {
            auto entry = behaviorClass.m_methods.find(methodName);
            if (entry == behaviorClass.m_methods.end())
            {
                return false;
            }
            AZ::BehaviorMethod* method = entry->second;
            if (method->HasResult())
            {
                if (method->GetResult()->m_typeId != output)
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
            return HasMethodWithInput(behaviorClass, methodName, input);
        }

        AZStd::unique_ptr<AZ::BehaviorContext> m_behaviorContext;
    };

    TEST_F(SceneGraphBehaviorTest, SceneClass_BehaviorContext_Exists)
    {
        EXPECT_TRUE(HasBehaviorClass(azrtti_typeid<AZ::SceneAPI::Containers::Scene>()));
    }

    TEST_F(SceneGraphBehaviorTest, SceneClass_BehaviorContext_HasExpectedProperties)
    {
        AZ::BehaviorClass* behaviorClass = GetBehaviorClass(azrtti_typeid<AZ::SceneAPI::Containers::Scene>());
        ASSERT_NE(nullptr, behaviorClass);
        EXPECT_TRUE(HasProperty(*behaviorClass, "name", azrtti_typeid<AZStd::string>()));
        EXPECT_TRUE(HasProperty(*behaviorClass, "manifestFilename", azrtti_typeid<AZStd::string>()));
        EXPECT_TRUE(HasProperty(*behaviorClass, "sourceFilename", azrtti_typeid<AZStd::string>()));
        EXPECT_TRUE(HasProperty(*behaviorClass, "sourceGuid", azrtti_typeid<AZ::Uuid>()));
        EXPECT_TRUE(HasProperty(*behaviorClass, "graph", azrtti_typeid<AZ::SceneAPI::Containers::SceneGraph>()));
        EXPECT_TRUE(HasProperty(*behaviorClass, "manifest", azrtti_typeid<AZ::SceneAPI::Containers::SceneManifest>()));
    }

    TEST_F(SceneGraphBehaviorTest, SceneGraphClass_BehaviorContext_Exists)
    {
        EXPECT_TRUE(HasBehaviorClass(azrtti_typeid<AZ::SceneAPI::Containers::SceneGraph>()));
        EXPECT_TRUE(HasBehaviorClass(azrtti_typeid<AZ::SceneAPI::Containers::SceneGraph::NodeIndex>()));
        EXPECT_TRUE(HasBehaviorClass(azrtti_typeid<AZ::SceneAPI::Containers::SceneGraph::Name>()));
    }

    TEST_F(SceneGraphBehaviorTest, SceneGraphClass_BehaviorContext_HasExpectedProperties)
    {
        using namespace AZ::SceneAPI::Containers;

        AZ::BehaviorClass* behaviorClass = GetBehaviorClass(azrtti_typeid<SceneGraph>());
        ASSERT_NE(nullptr, behaviorClass);
        EXPECT_TRUE(HasMethodWithOutput(*behaviorClass, "GetNodeName", azrtti_typeid<SceneGraph::Name>(), { azrtti_typeid<SceneGraph::NodeIndex>() }));
        EXPECT_TRUE(HasMethodWithOutput(*behaviorClass, "GetRoot", azrtti_typeid<SceneGraph::NodeIndex>(), {}));
        EXPECT_TRUE(HasMethodWithOutput(*behaviorClass, "HasNodeContent", azrtti_typeid<bool>(), { azrtti_typeid<SceneGraph::NodeIndex>() }));
        EXPECT_TRUE(HasMethodWithOutput(*behaviorClass, "HasNodeSibling", azrtti_typeid<bool>(), { azrtti_typeid<SceneGraph::NodeIndex>() }));
        EXPECT_TRUE(HasMethodWithOutput(*behaviorClass, "HasNodeChild", azrtti_typeid<bool>(), { azrtti_typeid<SceneGraph::NodeIndex>() }));
        EXPECT_TRUE(HasMethodWithOutput(*behaviorClass, "HasNodeParent", azrtti_typeid<bool>(), { azrtti_typeid<SceneGraph::NodeIndex>() }));
        EXPECT_TRUE(HasMethodWithOutput(*behaviorClass, "IsNodeEndPoint", azrtti_typeid<bool>(), { azrtti_typeid<SceneGraph::NodeIndex>() }));
        EXPECT_TRUE(HasMethodWithOutput(*behaviorClass, "GetNodeCount", azrtti_typeid<size_t>(), {}));
        EXPECT_TRUE(HasMethodWithOutput(
            *behaviorClass,
            "GetNodeParent",
            azrtti_typeid<SceneGraph::NodeIndex>(),
            { azrtti_typeid<SceneGraph>(), azrtti_typeid<SceneGraph::NodeIndex>() }
        ));
        EXPECT_TRUE(HasMethodWithOutput(
            *behaviorClass,
            "GetNodeSibling",
            azrtti_typeid<SceneGraph::NodeIndex>(),
            { azrtti_typeid<SceneGraph>(), azrtti_typeid<SceneGraph::NodeIndex>() }
        ));
        EXPECT_TRUE(HasMethodWithOutput(
            *behaviorClass,
            "GetNodeChild",
            azrtti_typeid<SceneGraph::NodeIndex>(),
            { azrtti_typeid<SceneGraph>(), azrtti_typeid<SceneGraph::NodeIndex>() }
        ));
        EXPECT_TRUE(HasMethodWithOutput(
            *behaviorClass,
            "FindWithPath",
            azrtti_typeid<SceneGraph::NodeIndex>(),
            { azrtti_typeid<SceneGraph>(), azrtti_typeid<AZStd::string>() }
        ));
        EXPECT_TRUE(HasMethodWithOutput(
            *behaviorClass,
            "FindWithRootAndPath",
            azrtti_typeid<SceneGraph::NodeIndex>(),
            { azrtti_typeid<SceneGraph>(), azrtti_typeid<SceneGraph::NodeIndex>(), azrtti_typeid<AZStd::string>() }
        ));
    }

    TEST_F(SceneGraphBehaviorTest, SceneGraphNodeIndexClass_BehaviorContext_HasExpectedProperties)
    {
        using namespace AZ::SceneAPI::Containers;

        AZ::BehaviorClass* behaviorClass = GetBehaviorClass(azrtti_typeid<SceneGraph::NodeIndex>());
        ASSERT_NE(nullptr, behaviorClass);
        EXPECT_TRUE(HasMethodWithOutput(*behaviorClass, "AsNumber", azrtti_typeid<uint32_t>(), {}));
        EXPECT_TRUE(HasMethodWithOutput(*behaviorClass, "Distance", azrtti_typeid<AZ::s32>(), { azrtti_typeid<SceneGraph::NodeIndex>() }));
        EXPECT_TRUE(HasMethodWithOutput(*behaviorClass, "IsValid", azrtti_typeid<bool>(), {}));
        EXPECT_TRUE(HasMethodWithOutput(*behaviorClass, "Equal", azrtti_typeid<bool>(), { azrtti_typeid<SceneGraph::NodeIndex>() }));
    }

    TEST_F(SceneGraphBehaviorTest, SceneGraphNameClass_BehaviorContext_HasExpectedProperties)
    {
        using namespace AZ::SceneAPI::Containers;

        AZ::BehaviorClass* behaviorClass = GetBehaviorClass(azrtti_typeid<SceneGraph::Name>());
        ASSERT_NE(nullptr, behaviorClass);
        EXPECT_TRUE(HasMethodWithOutput(*behaviorClass, "GetPath", azrtti_typeid<const char*>(), {}));
        EXPECT_TRUE(HasMethodWithOutput(*behaviorClass, "GetName", azrtti_typeid<const char*>(), {}));
    }

    class MockSceneComponentApplication
        : public AZ::ComponentApplicationBus::Handler
    {
    public:
        MockSceneComponentApplication()
        {
            AZ::ComponentApplicationBus::Handler::BusConnect();
            AZ::Interface<AZ::ComponentApplicationRequests>::Register(this);
        }

        ~MockSceneComponentApplication()
        {
            AZ::Interface<AZ::ComponentApplicationRequests>::Unregister(this);
            AZ::ComponentApplicationBus::Handler::BusDisconnect();
        }

        MOCK_METHOD1(FindEntity, AZ::Entity* (const AZ::EntityId&));
        MOCK_METHOD1(AddEntity, bool(AZ::Entity*));
        MOCK_METHOD0(Destroy, void());
        MOCK_METHOD1(RegisterComponentDescriptor, void(const AZ::ComponentDescriptor*));
        MOCK_METHOD1(UnregisterComponentDescriptor, void(const AZ::ComponentDescriptor*));
        MOCK_METHOD1(RegisterEntityAddedEventHandler, void(AZ::EntityAddedEvent::Handler&));
        MOCK_METHOD1(RegisterEntityRemovedEventHandler, void(AZ::EntityRemovedEvent::Handler&));
        MOCK_METHOD1(RegisterEntityActivatedEventHandler, void(AZ::EntityActivatedEvent::Handler&));
        MOCK_METHOD1(RegisterEntityDeactivatedEventHandler, void(AZ::EntityDeactivatedEvent::Handler&));
        MOCK_METHOD1(SignalEntityActivated, void(AZ::Entity*));
        MOCK_METHOD1(SignalEntityDeactivated, void(AZ::Entity*));
        MOCK_METHOD1(RemoveEntity, bool(AZ::Entity*));
        MOCK_METHOD1(DeleteEntity, bool(const AZ::EntityId&));
        MOCK_METHOD1(GetEntityName, AZStd::string(const AZ::EntityId&));
        MOCK_METHOD1(EnumerateEntities, void(const ComponentApplicationRequests::EntityCallback&));
        MOCK_METHOD0(GetApplication, AZ::ComponentApplication* ());
        MOCK_METHOD0(GetSerializeContext, AZ::SerializeContext* ());
        MOCK_METHOD0(GetJsonRegistrationContext, AZ::JsonRegistrationContext* ());
        MOCK_METHOD0(GetBehaviorContext, AZ::BehaviorContext* ());
        MOCK_CONST_METHOD0(GetEngineRoot, const char*());
        MOCK_CONST_METHOD0(GetExecutableFolder, const char* ());
        MOCK_CONST_METHOD1(QueryApplicationType, void(AZ::ApplicationTypeQuery&));
    };

    class MockEditorPythonConsoleInterface final
        : public AzToolsFramework::EditorPythonConsoleInterface
    {
    public:
        MockEditorPythonConsoleInterface()
        {
            AZ::Interface<AzToolsFramework::EditorPythonConsoleInterface>::Register(this);
        }

        ~MockEditorPythonConsoleInterface()
        {
            AZ::Interface<AzToolsFramework::EditorPythonConsoleInterface>::Unregister(this);
        }

        MOCK_CONST_METHOD1(GetModuleList, void(AZStd::vector<AZStd::string_view>&));
        MOCK_CONST_METHOD1(GetGlobalFunctionList, void(GlobalFunctionCollection&));
        MOCK_METHOD1(FetchPythonTypeName, AZStd::string(const AZ::BehaviorParameter&));
    };

    //
    // SceneGraphBehaviorScriptTest
    //
    class SceneGraphBehaviorScriptTest
        : public UnitTest::LeakDetectionFixture
    {
    public:
        AZStd::unique_ptr<MockSceneComponentApplication> m_componentApplication;
        AZStd::unique_ptr<MockEditorPythonConsoleInterface> m_editorPythonConsoleInterface;
        AZStd::unique_ptr<AZ::ScriptContext> m_scriptContext;
        AZStd::unique_ptr<AZ::BehaviorContext> m_behaviorContext;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;

        static void TestExpectTrue(bool value)
        {
            EXPECT_TRUE(value);
        }

        static void TestExpectEquals(AZ::s64 lhs, AZ::s64 rhs)
        {
            EXPECT_EQ(lhs, rhs);
        }

        static void ReflectTestTypes(AZ::ReflectContext* context)
        {
            AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<DataTypes::MockIGraphObject>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Module, "scene.graph.test")
                    ->Method("GetId", [](const DataTypes::MockIGraphObject& self)
                    {
                        return self.m_id;
                    })
                    ->Method("SetId", [](DataTypes::MockIGraphObject& self, int value)
                    {
                        self.m_id = value;
                    })
                    ->Method("AddAndSet", [](DataTypes::MockIGraphObject& self, int lhs, int rhs)
                    {
                        self.m_id = lhs + rhs;
                    })
                    ->Property("id", BehaviorValueProperty(&AZ::SceneAPI::DataTypes::MockIGraphObject::m_id));

                behaviorContext->Class<MockIGraphObjectTester>("MockIGraphObjectTester")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Module, "scene.graph.test")
                    ->Method("GetBigValue", [](const MockIGraphObjectTester& self)
                    {
                        return *(&self);
                    })
                    ->Method("GetViaAddress", [](const MockIGraphObjectTester& self)
                    {
                        return &self.m_buffer[0];
                    })
                    ->Method("GetViaReference", [](const MockIGraphObjectTester& self) -> const AZStd::string&
                    {
                        return self.m_string;
                    })
                    ->Method("GetIndex", [](const MockIGraphObjectTester& self, int index)
                    {
                        return self.m_buffer[index];
                    });
            }
        }

        void SetUp() override
        {
            UnitTest::LeakDetectionFixture::SetUp();

            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_serializeContext->RegisterGenericType<AZStd::string>();

            m_behaviorContext = AZStd::make_unique<AZ::BehaviorContext>();
            m_behaviorContext->Method("TestExpectTrue", &TestExpectTrue);
            m_behaviorContext->Method("TestExpectEquals", &TestExpectEquals);

            AZ::MathReflect(m_behaviorContext.get());
            ReflectBehavior(m_behaviorContext.get());
            ReflectTestTypes(m_behaviorContext.get());
            MockBuilder::Reflect(m_behaviorContext.get());
            UnitTest::ScopeForUnitTest(m_behaviorContext->m_classes.find("Scene")->second->m_attributes);
            UnitTest::ScopeForUnitTest(m_behaviorContext->m_classes.find("ExportProduct")->second->m_attributes);
            UnitTest::ScopeForUnitTest(m_behaviorContext->m_classes.find("ExportProductList")->second->m_attributes);
            UnitTest::ScopeForUnitTest(m_behaviorContext->m_classes.find("GraphObjectProxy")->second->m_attributes);
            UnitTest::ScopeForUnitTest(m_behaviorContext->m_classes.find("PythonBehaviorInfo")->second->m_attributes);

            m_scriptContext = AZStd::make_unique<AZ::ScriptContext>();
            m_scriptContext->BindTo(m_behaviorContext.get());

            m_componentApplication = AZStd::make_unique<::testing::NiceMock<MockSceneComponentApplication>>();

            ON_CALL(*m_componentApplication, GetBehaviorContext())
                .WillByDefault(::testing::Invoke([this]()
                    {
                        return this->m_behaviorContext.get();
                    }));

            ON_CALL(*m_componentApplication, GetSerializeContext())
                .WillByDefault(::testing::Invoke([this]()
                    {
                        return this->m_serializeContext.get();
                    }));

            m_editorPythonConsoleInterface = AZStd::make_unique<MockEditorPythonConsoleInterface>();
        }

        void SetupEditorPythonConsoleInterface()
        {
            EXPECT_CALL(*m_editorPythonConsoleInterface, FetchPythonTypeName(::testing::_))
                .Times(6)
                .WillRepeatedly(::testing::Invoke([](const AZ::BehaviorParameter&) {return "int"; }));
        }

        void TearDown() override
        {
            m_scriptContext.reset();
            m_serializeContext.reset();
            m_behaviorContext.reset();

            UnitTest::LeakDetectionFixture::TearDown();
        }

        void ExpectExecute(AZStd::string_view script)
        {
            EXPECT_TRUE(m_scriptContext->Execute(script.data()));
        }
    };

    TEST_F(SceneGraphBehaviorScriptTest, Scene_ScriptContext_Access)
    {
        ExpectExecute("builder = MockBuilder()");
        ExpectExecute("builder:BuildSceneGraph()");
        ExpectExecute("scene = builder:GetScene()");
        ExpectExecute("TestExpectTrue(scene ~= nil)");
        ExpectExecute("TestExpectTrue(scene.name == 'unit_scene')");
        ExpectExecute("TestExpectTrue(scene.manifestFilename == 'manifest_filename')");
        ExpectExecute("TestExpectTrue(scene.sourceFilename == 'unit_source_filename')");
        ExpectExecute("TestExpectTrue(tostring(scene.sourceGuid) == '{1F2E6142-B0D8-42C6-A6E5-CD726DAA9EF0}')");
        ExpectExecute("TestExpectTrue(scene:GetOriginalSceneOrientation() == Scene.SceneOrientation_YUp)");
    }

    TEST_F(SceneGraphBehaviorScriptTest, SceneGraph_ScriptContext_AccessMockNodes)
    {
        ExpectExecute("builder = MockBuilder()");
        ExpectExecute("builder:BuildSceneGraph()");
        ExpectExecute("scene = builder:GetScene()");

        // instance methods
        ExpectExecute("TestExpectTrue(scene.graph ~= nil)");
        ExpectExecute("TestExpectTrue(scene.graph:GetRoot():IsValid())");
        ExpectExecute("TestExpectEquals(scene.graph:GetNodeCount(), 13)");
        ExpectExecute("nodeRoot = scene.graph:GetRoot()");
        ExpectExecute("nodeA = scene.graph:GetNodeChild(nodeRoot); TestExpectTrue(nodeA:IsValid())");
        ExpectExecute("TestExpectTrue(scene.graph:HasNodeContent(nodeA))");
        ExpectExecute("nodeC = scene.graph:GetNodeChild(nodeA); TestExpectTrue(nodeC:IsValid())");
        ExpectExecute("nodeNameC = scene.graph:GetNodeName(nodeC); TestExpectTrue(nodeNameC ~= nil)");
        ExpectExecute("nodeE = scene.graph:GetNodeChild(nodeC); TestExpectTrue(nodeE:IsValid())");
        ExpectExecute("TestExpectTrue(scene.graph:HasNodeSibling(nodeE))");
        ExpectExecute("TestExpectTrue(scene.graph:HasNodeChild(nodeE))");
        ExpectExecute("TestExpectTrue(scene.graph:HasNodeParent(nodeE))");
        ExpectExecute("nodeG = scene.graph:GetNodeChild(nodeE); TestExpectTrue(nodeG:IsValid())");
        ExpectExecute("TestExpectTrue(scene.graph:GetNodeParent(nodeG) == nodeE)");
        ExpectExecute("nodeH = scene.graph:GetNodeSibling(nodeG); TestExpectTrue(nodeH:IsValid())");
        ExpectExecute("TestExpectTrue(scene.graph:GetNodeName(nodeH):GetPath() == 'A.C.E.H')");
        ExpectExecute("nodeB = scene.graph:GetNodeSibling(nodeA); TestExpectTrue(nodeB:IsValid())");
        ExpectExecute("nodeK = scene.graph:GetNodeChild(nodeB); TestExpectTrue(nodeK:IsValid())");
        ExpectExecute("TestExpectTrue(scene.graph:FindWithPath('B.K') == nodeK)");
        ExpectExecute("nodeL = scene.graph:GetNodeChild(nodeK); TestExpectTrue(nodeL:IsValid())");
        ExpectExecute("TestExpectTrue(scene.graph:FindWithRootAndPath(nodeK, 'L') == nodeL)");

        // static methods
        ExpectExecute("TestExpectTrue(scene.graph.IsValidName('A'))");
        ExpectExecute("TestExpectTrue(scene.graph.GetNodeSeperationCharacter() == string.byte('.'))");
    }

    TEST_F(SceneGraphBehaviorScriptTest, SceneGraphNodeIndex_ScriptContext_AccessMockNodes)
    {
        ExpectExecute("builder = MockBuilder()");
        ExpectExecute("builder:BuildSceneGraph()");
        ExpectExecute("scene = builder:GetScene()");
        ExpectExecute("nodeA = scene.graph:GetNodeChild(scene.graph:GetRoot())");
        ExpectExecute("TestExpectTrue(nodeA:IsValid())");
        ExpectExecute("TestExpectEquals(nodeA:AsNumber(), 1)");
        ExpectExecute("TestExpectEquals(scene.graph:GetRoot():Distance(nodeA), 1)");
        ExpectExecute("TestExpectEquals(nodeA:Distance(scene.graph:GetRoot()), -1)");
        ExpectExecute("TestExpectTrue(nodeA == scene.graph:FindWithPath('A'))");
    }

    TEST_F(SceneGraphBehaviorScriptTest, SceneGraphName_ScriptContext_AccessMockNodes)
    {
        ExpectExecute("builder = MockBuilder()");
        ExpectExecute("builder:BuildSceneGraph()");
        ExpectExecute("scene = builder:GetScene()");
        ExpectExecute("nodeG = scene.graph:FindWithPath('A.C.E.G')");
        ExpectExecute("nodeNameG = scene.graph:GetNodeName(nodeG)");
        ExpectExecute("TestExpectTrue(nodeNameG:GetPath() == 'A.C.E.G')");
        ExpectExecute("TestExpectTrue(nodeNameG:GetName() == 'G')");
    }

    TEST_F(SceneGraphBehaviorScriptTest, SceneGraphIGraphNode_ScriptContext_AccessMockNodes)
    {
        ExpectExecute("builder = MockBuilder()");
        ExpectExecute("builder:BuildSceneGraph()");
        ExpectExecute("scene = builder:GetScene()");
        ExpectExecute("nodeG = scene.graph:FindWithPath('A.C.E.G')");
        ExpectExecute("proxy = scene.graph:GetNodeContent(nodeG)");
        ExpectExecute("TestExpectTrue(proxy:CastWithTypeName('MockIGraphObject'))");
        ExpectExecute("value = proxy:Invoke('GetId', vector_any())");
        ExpectExecute("TestExpectEquals(value, 7)");
        ExpectExecute("setIdArgs = vector_any(); setIdArgs:push_back(8);");
        ExpectExecute("proxy:Invoke('SetId', setIdArgs)");
        ExpectExecute("value = proxy:Invoke('GetId', vector_any())");
        ExpectExecute("TestExpectEquals(value, 8)");
        ExpectExecute("addArgs = vector_any(); addArgs:push_back(8); addArgs:push_back(9)");
        ExpectExecute("proxy:Invoke('AddAndSet', addArgs)");
        ExpectExecute("value = proxy:Invoke('GetId', vector_any())");
        ExpectExecute("TestExpectEquals(value, 17)");
    }

    TEST_F(SceneGraphBehaviorScriptTest, SceneGraphIGraphNode_GraphObjectProxy_InvokeGetBigValue)
    {
        ExpectExecute("builder = MockBuilder()");
        ExpectExecute("builder:BuildSceneGraph()");
        ExpectExecute("scene = builder:GetScene()");
        ExpectExecute("node = scene.graph:FindWithPath('B.K.L')");
        ExpectExecute("proxy = scene.graph:GetNodeContent(node)");
        ExpectExecute("proxy:CastWithTypeName('MockIGraphObjectTester')");
        ExpectExecute("value = proxy:Invoke('GetBigValue', vector_any())");
        ExpectExecute("TestExpectTrue(value == false)");
    }

    TEST_F(SceneGraphBehaviorScriptTest, SceneGraphIGraphNode_GraphObjectProxy_InvokeGetViaAddress)
    {
        ExpectExecute("builder = MockBuilder()");
        ExpectExecute("builder:BuildSceneGraph()");
        ExpectExecute("scene = builder:GetScene()");
        ExpectExecute("node = scene.graph:FindWithPath('B.K.L')");
        ExpectExecute("proxy = scene.graph:GetNodeContent(node)");
        ExpectExecute("proxy:CastWithTypeName('MockIGraphObjectTester')");
        ExpectExecute("value = proxy:Invoke('GetViaAddress', vector_any())");
        ExpectExecute("TestExpectTrue(value ~= nil)");
    }

    TEST_F(SceneGraphBehaviorScriptTest, SceneGraphIGraphNode_GraphObjectProxy_InvokeGetViaReference)
    {
        ExpectExecute("builder = MockBuilder()");
        ExpectExecute("builder:BuildSceneGraph()");
        ExpectExecute("scene = builder:GetScene()");
        ExpectExecute("node = scene.graph:FindWithPath('B.K.L')");
        ExpectExecute("proxy = scene.graph:GetNodeContent(node)");
        ExpectExecute("proxy:CastWithTypeName('MockIGraphObjectTester')");
        ExpectExecute("value = proxy:Invoke('GetViaReference', vector_any())");
        ExpectExecute("TestExpectTrue(value ~= nil)");
        ExpectExecute("TestExpectTrue(value == 'test text buffer')");
    }

    TEST_F(SceneGraphBehaviorScriptTest, SceneGraphIGraphNode_GraphObjectProxy_InvokeGetIndex)
    {
        ExpectExecute("builder = MockBuilder()");
        ExpectExecute("builder:BuildSceneGraph()");
        ExpectExecute("scene = builder:GetScene()");
        ExpectExecute("node = scene.graph:FindWithPath('B.K.L')");
        ExpectExecute("proxy = scene.graph:GetNodeContent(node)");
        ExpectExecute("proxy:CastWithTypeName('MockIGraphObjectTester')");
        ExpectExecute("args = vector_any(); args:push_back(1);");
        ExpectExecute("value = proxy:Invoke('GetIndex', args)");
        ExpectExecute("TestExpectTrue(value == 2)");
    }

    TEST_F(SceneGraphBehaviorScriptTest, GraphObjectProxy_GetClassInfo_Loads)
    {
        SetupEditorPythonConsoleInterface();

        ExpectExecute("builder = MockBuilder()");
        ExpectExecute("builder:BuildSceneGraph()");
        ExpectExecute("scene = builder:GetScene()");
        ExpectExecute("nodeG = scene.graph:FindWithPath('A.C.E.G')");
        ExpectExecute("proxy = scene.graph:GetNodeContent(nodeG)");
        ExpectExecute("TestExpectTrue(proxy:CastWithTypeName('MockIGraphObject'))");
        ExpectExecute("info = proxy:GetClassInfo()");
        ExpectExecute("TestExpectTrue(info ~= nil)");
    }

    TEST_F(SceneGraphBehaviorScriptTest, GraphObjectProxy_GetClassInfo_CorrectFormats)
    {
        SetupEditorPythonConsoleInterface();

        ExpectExecute("builder = MockBuilder()");
        ExpectExecute("builder:BuildSceneGraph()");
        ExpectExecute("scene = builder:GetScene()");
        ExpectExecute("nodeG = scene.graph:FindWithPath('A.C.E.G')");
        ExpectExecute("proxy = scene.graph:GetNodeContent(nodeG)");
        ExpectExecute("TestExpectTrue(proxy:CastWithTypeName('MockIGraphObject'))");
        ExpectExecute("info = proxy:GetClassInfo()");
        ExpectExecute("TestExpectTrue(info.className == 'MockIGraphObject')");
        ExpectExecute("TestExpectTrue(info.classUuid == '{66A082CC-851D-4E1F-ABBD-45B58A216CFA}')");
        ExpectExecute("TestExpectTrue(info.methodList[1] == 'def GetId(self) -> int')");
        ExpectExecute("TestExpectTrue(info.methodList[2] == 'def SetId(self, arg1: int) -> None')");
        ExpectExecute("TestExpectTrue(info.methodList[3] == 'def AddAndSet(self, arg1: int, arg2: int) -> None')");
    }

    TEST_F(SceneGraphBehaviorScriptTest, ExportProduct_ExpectedClassesAndFields_Work)
    {
        ExpectExecute("mockAssetType = Uuid.CreateString('{B7AD6A54-963F-4F0F-A70E-1CFC0364BE6B}')");
        ExpectExecute("exportProduct = ExportProduct()");
        ExpectExecute("exportProduct.filename = 'some/file.name'");
        ExpectExecute("exportProduct.sourceId = Uuid.CreateString('{A19F5FDB-C5FB-478F-A0B0-B697D2C10DB5}')");
        ExpectExecute("exportProduct.assetType = mockAssetType");
        ExpectExecute("exportProduct.subId = 10101");
        ExpectExecute("TestExpectEquals(exportProduct.subId, 10101)");
        ExpectExecute("TestExpectEquals(exportProduct.productDependencies:GetSize(), 0)");

        ExpectExecute("exportProductDep = ExportProduct()");
        ExpectExecute("exportProductDep.filename = 'some/file.dep'");
        ExpectExecute("exportProductDep.sourceId = Uuid.CreateString('{A19F5FDB-C5FB-478F-A0B0-B697D2C10DB5}')");
        ExpectExecute("exportProductDep.assetType = mockAssetType");
        ExpectExecute("exportProductDep.subId = 2");

        ExpectExecute("exportProductList = ExportProductList()");
        ExpectExecute("exportProductList:AddProduct(exportProduct)");
        ExpectExecute("exportProductList:AddProduct(exportProductDep)");
        ExpectExecute("productList = exportProductList:GetProducts()");
        ExpectExecute("TestExpectEquals(productList:GetSize(), 2)");
        ExpectExecute("exportProductList:AddDependencyToProduct(exportProduct.filename, exportProductDep)");
        ExpectExecute("TestExpectEquals(productList:Front().productDependencies:GetSize(), 1)");
    }

    TEST_F(SceneGraphBehaviorScriptTest, GraphObjectProxy_Fetch_GetsValue)
    {
        ExpectExecute("builder = MockBuilder()");
        ExpectExecute("builder:BuildSceneGraph()");
        ExpectExecute("scene = builder:GetScene()");
        ExpectExecute("nodeG = scene.graph:FindWithPath('A.C.E.G')");
        ExpectExecute("proxy = scene.graph:GetNodeContent(nodeG)");
        ExpectExecute("TestExpectTrue(proxy:CastWithTypeName('MockIGraphObject'))");
        ExpectExecute("id = proxy:Fetch('id')");
        ExpectExecute("TestExpectEquals(id, 7)");
    }

    TEST_F(SceneGraphBehaviorScriptTest, GraphObjectProxy_GetClassInfo_HasProperties)
    {
        SetupEditorPythonConsoleInterface();

        ExpectExecute("builder = MockBuilder()");
        ExpectExecute("builder:BuildSceneGraph()");
        ExpectExecute("scene = builder:GetScene()");
        ExpectExecute("nodeG = scene.graph:FindWithPath('A.C.E.G')");
        ExpectExecute("proxy = scene.graph:GetNodeContent(nodeG)");
        ExpectExecute("TestExpectTrue(proxy:CastWithTypeName('MockIGraphObject'))");
        ExpectExecute("info = proxy:GetClassInfo()");
        ExpectExecute("TestExpectTrue(info.propertyList[1] == 'id(int)->int')");
    }

    //
    // SceneManifestBehaviorScriptTest is meant to test the script abilities of the SceneManifest
    //
    class SceneManifestBehaviorScriptTest
        : public UnitTest::LeakDetectionFixture
    {
    public:
        AZStd::unique_ptr<MockSceneComponentApplication> m_componentApplication;
        AZStd::unique_ptr<AZ::ScriptContext> m_scriptContext;
        AZStd::unique_ptr<AZ::BehaviorContext> m_behaviorContext;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_jsonRegistrationContext;
        AZStd::string_view m_jsonMockData = R"JSON('{"values":[{"$type":"MockManifestRule","value":0.1},{"$type":"MockManifestRule","value":2.3},{"$type":"MockManifestRule","value":4.5}]}')JSON";

        static void TestAssertTrue(bool value)
        {
            EXPECT_TRUE(value);
        }

        void SetUp() override
        {
            UnitTest::LeakDetectionFixture::SetUp();

            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            MockBuilder::Reflect(m_serializeContext.get());
            MockManifestRule::Reflect(m_serializeContext.get());
            ReflectTypes(m_serializeContext.get());

            m_behaviorContext = AZStd::make_unique<AZ::BehaviorContext>();
            m_behaviorContext->Method("TestAssertTrue", &TestAssertTrue);
            AZ::MathReflect(m_behaviorContext.get());
            MockBuilder::Reflect(m_behaviorContext.get());
            MockManifestRule::Reflect(m_behaviorContext.get());
            ReflectBehavior(m_behaviorContext.get());
            UnitTest::ScopeForUnitTest(m_behaviorContext->m_classes.find("Scene")->second->m_attributes);
            UnitTest::ScopeForUnitTest(m_behaviorContext->m_classes.find("ExportProduct")->second->m_attributes);
            UnitTest::ScopeForUnitTest(m_behaviorContext->m_classes.find("ExportProductList")->second->m_attributes);

            m_jsonRegistrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();
            AZ::JsonSystemComponent::Reflect(m_jsonRegistrationContext.get());

            m_scriptContext = AZStd::make_unique<AZ::ScriptContext>();
            m_scriptContext->BindTo(m_behaviorContext.get());

            m_componentApplication = AZStd::make_unique<::testing::NiceMock<MockSceneComponentApplication>>();

            ON_CALL(*m_componentApplication, GetBehaviorContext()).WillByDefault(::testing::Invoke([this]()
                {
                    return this->m_behaviorContext.get();
                }));

            ON_CALL(*m_componentApplication, GetSerializeContext()).WillByDefault(::testing::Invoke([this]()
                {
                    return this->m_serializeContext.get();
                }));

            ON_CALL(*m_componentApplication, GetJsonRegistrationContext()).WillByDefault(::testing::Invoke([this]()
                {
                    return this->m_jsonRegistrationContext.get();
                }));
        }

        void TearDown() override
        {
            m_jsonRegistrationContext->EnableRemoveReflection();
            AZ::JsonSystemComponent::Reflect(m_jsonRegistrationContext.get());
            m_jsonRegistrationContext->DisableRemoveReflection();

            m_jsonRegistrationContext.reset();
            m_serializeContext.reset();
            m_scriptContext.reset();
            m_behaviorContext.reset();
            m_componentApplication.reset();

            CleanUpSceneCoreGenericClassInfo();

            UnitTest::LeakDetectionFixture::TearDown();
        }

        void ExpectExecute(AZStd::string_view script)
        {
            EXPECT_TRUE(m_scriptContext->Execute(script.data()));
        }
    };

    TEST_F(SceneManifestBehaviorScriptTest, SceneManifest_ScriptContext_GetDefaultJSON)
    {
        ExpectExecute("builder = MockBuilder()");
        ExpectExecute("scene = builder:GetScene()");
        ExpectExecute("manifest = scene.manifest:ExportToJson()");
        ExpectExecute(R"JSON(TestAssertTrue(manifest == '{}'))JSON");
    }

    TEST_F(SceneManifestBehaviorScriptTest, SceneManifest_ScriptContext_GetComplexJSON)
    {
        ExpectExecute("builder = MockBuilder()");
        ExpectExecute("scene = builder:GetScene()");
        ExpectExecute("builder:BuildSceneGraph()");
        ExpectExecute("manifest = scene.manifest:ExportToJson()");
        auto read = AZStd::fixed_string<1024>::format("TestAssertTrue(manifest == %s)", m_jsonMockData.data());
        ExpectExecute(read);
    }

    TEST_F(SceneManifestBehaviorScriptTest, SceneManifest_ScriptContext_SetComplexJSON)
    {
        ExpectExecute("builder = MockBuilder()");
        ExpectExecute("scene = builder:GetScene()");
        ExpectExecute("manifest = scene.manifest:ExportToJson()");
        ExpectExecute(R"JSON(TestAssertTrue(manifest == '{}'))JSON");
        auto load = AZStd::fixed_string<1024>::format("TestAssertTrue(scene.manifest:ImportFromJson(%s))", m_jsonMockData.data());
        ExpectExecute(load);
        ExpectExecute("manifest = scene.manifest:ExportToJson()");
        auto read = AZStd::fixed_string<1024>::format("TestAssertTrue(manifest == %s)", m_jsonMockData.data());
        ExpectExecute(read);
    }

} // namespace AZ::SceneAPI::Containers
