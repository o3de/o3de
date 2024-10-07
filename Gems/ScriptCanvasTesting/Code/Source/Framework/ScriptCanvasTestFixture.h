/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzTest/AzTest.h>

#include <Nodes/BehaviorContextObjectTestNode.h>
#include <Nodes/TestAutoGenFunctions.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/SlotConfigurationDefaults.h>
#include <ScriptCanvas/ScriptCanvasGem.h>
#include <ScriptCanvas/SystemComponent.h>
#include <ScriptCanvas/Variable/GraphVariableManagerComponent.h>

#include "EntityRefTests.h"
#include "ScriptCanvasTestApplication.h"
#include "ScriptCanvasTestBus.h"
#include "ScriptCanvasTestNodes.h"
#include "ScriptCanvasTestUtilities.h"
#include <AutoGen/ScriptCanvasAutoGenRegistry.h>

#define SC_EXPECT_DOUBLE_EQ(candidate, reference) EXPECT_NEAR(candidate, reference, 0.001)
#define SC_EXPECT_FLOAT_EQ(candidate, reference) EXPECT_NEAR(candidate, reference, 0.001f)

namespace ScriptCanvasTests
{

    class ScriptCanvasTestFixture
        : public ::testing::Test
        //, protected NodeAccessor
    {
    public:
        static AZStd::atomic_bool s_asyncOperationActive;

    protected:

        static ScriptCanvasTests::Application* s_application;

        static void SetUpTestCase()
        {
            s_asyncOperationActive = false;

            if (s_application == nullptr)
            {
                AZ::ComponentApplication::StartupParameters appStartup;
                s_application = aznew ScriptCanvasTests::Application();

                {
                    ScriptCanvasEditor::TraceSuppressionBus::Broadcast(&ScriptCanvasEditor::TraceSuppressionRequests::SuppressPrintf, true);
                    AZ::ComponentApplication::Descriptor descriptor;
                    descriptor.m_useExistingAllocator = true;

                    AZ::DynamicModuleDescriptor dynamicModuleDescriptor;
                    dynamicModuleDescriptor.m_dynamicLibraryPath = "GraphCanvas.Editor";
                    descriptor.m_modules.push_back(dynamicModuleDescriptor);
                    dynamicModuleDescriptor.m_dynamicLibraryPath = "ScriptCanvas.Editor";
                    descriptor.m_modules.push_back(dynamicModuleDescriptor);
                    dynamicModuleDescriptor.m_dynamicLibraryPath = "ExpressionEvaluation";
                    descriptor.m_modules.push_back(dynamicModuleDescriptor);
                    dynamicModuleDescriptor.m_dynamicLibraryPath = "ScriptEvents";
                    descriptor.m_modules.push_back(dynamicModuleDescriptor);
                    s_application->Start(descriptor, appStartup);
                    // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
                    // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
                    // in the unit tests.
                    AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
                    ScriptCanvasEditor::TraceSuppressionBus::Broadcast(&ScriptCanvasEditor::TraceSuppressionRequests::SuppressPrintf, false);
                }
            }

            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "SC unit tests require filehandling");

            s_setupSucceeded = fileIO->GetAlias("@engroot@") != nullptr;
            // Set the @gemroot:<gem-name> alias for active gems
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            if (settingsRegistry)
            {
                AZ::Test::AddActiveGem("ScriptCanvasTesting", *settingsRegistry, fileIO);
                AZ::Test::AddActiveGem("GraphCanvas", *settingsRegistry, fileIO);
                AZ::Test::AddActiveGem("ScriptCanvas", *settingsRegistry, fileIO);
                AZ::Test::AddActiveGem("ScriptEvents", *settingsRegistry, fileIO);
                AZ::Test::AddActiveGem("ExpressionEvaluation", *settingsRegistry, fileIO);
            }

            AZ::TickBus::AllowFunctionQueuing(true);

            auto m_serializeContext = s_application->GetSerializeContext();
            auto m_behaviorContext = s_application->GetBehaviorContext();

            for (AZ::ReflectContext* context :
                {static_cast<AZ::ReflectContext*>(m_serializeContext), static_cast<AZ::ReflectContext*>(m_behaviorContext)})
            {
                ScriptCanvasTesting::Reflect(context);
                ScriptCanvasTestingNodes::BehaviorContextObjectTest::Reflect(context);
                TestNodeableObject::Reflect(context);
                TestBaseClass::Reflect(context);
                TestSubClass::Reflect(context);
                ScriptUnitTestEventHandler::Reflect(context);
            }

            ScriptCanvasModel::Instance().Init();

        }

        static void TearDownTestCase()
        {
            // don't hang on to dangling assets
            AZ::Data::AssetManager::Instance().DispatchEvents();

            if (s_application)
            {
                delete s_application;
                s_application = nullptr;
            }

            s_leakDetection.CheckAllocatorsForLeaks();
        }

        template<class T>
        void RegisterComponentDescriptor()
        {
            AZ::ComponentDescriptor* descriptor = T::CreateDescriptor();

            auto insertResult = m_descriptors.insert(descriptor);

            if (insertResult.second)
            {
                GetApplication()->RegisterComponentDescriptor(descriptor);
            }
        }

        void SetUp() override
        {
            ASSERT_TRUE(s_setupSucceeded) << "ScriptCanvasTestFixture set up failed, unit tests can't work properly";
            m_serializeContext = s_application->GetSerializeContext();
            m_behaviorContext = s_application->GetBehaviorContext();
            AZ_Assert(AZ::IO::FileIOBase::GetInstance(), "File IO was not properly installed");

            RegisterComponentDescriptor<TestNodes::TestResult>();
            RegisterComponentDescriptor<TestNodes::ConfigurableUnitTestNode>();

            m_numericVectorType = ScriptCanvas::Data::Type::BehaviorContextObject(azrtti_typeid<AZStd::vector<ScriptCanvas::Data::NumberType>>());
            m_stringToNumberMapType = ScriptCanvas::Data::Type::BehaviorContextObject(azrtti_typeid<AZStd::unordered_map<ScriptCanvas::Data::StringType, ScriptCanvas::Data::NumberType>>());

            m_dataSlotConfigurationType = ScriptCanvas::Data::Type::BehaviorContextObject(azrtti_typeid<ScriptCanvas::DataSlotConfiguration>());

            m_baseClassType = ScriptCanvas::Data::Type::BehaviorContextObject(azrtti_typeid<TestBaseClass>());
            m_subClassType = ScriptCanvas::Data::Type::BehaviorContextObject(azrtti_typeid<TestSubClass>());
        }

        void TearDown() override
        {
            delete m_graph;
            m_graph = nullptr;

            ASSERT_TRUE(s_setupSucceeded) << "ScriptCanvasTestFixture set up failed, unit tests can't work properly";

            for (AZ::ComponentDescriptor* componentDescriptor : m_descriptors)
            {
                GetApplication()->UnregisterComponentDescriptor(componentDescriptor);
            }

            m_descriptors.clear();
        }

        ScriptCanvas::Graph* CreateGraph()
        {
            AZ_Assert(!m_graph, "Only one graph should be created per test.");
            m_graph = aznew ScriptCanvas::Graph();
            m_graph->Init();
            return m_graph;
        }

        ScriptCanvasEditor::EditorGraph* CreateEditorGraph()
        {
            AZ_Assert(!m_graph, "Only one graph should be created per test.");
            m_graph = aznew ScriptCanvasEditor::EditorGraph();
            m_graph->Init();
            return static_cast<ScriptCanvasEditor::EditorGraph*>(m_graph);
        }

        TestNodes::ConfigurableUnitTestNode* CreateConfigurableNode(AZStd::string entityName = "ConfigurableNodeEntity")
        {
            AZ::Entity* configurableNodeEntity = new AZ::Entity(entityName.c_str());
            auto configurableNode = configurableNodeEntity->CreateComponent<TestNodes::ConfigurableUnitTestNode>();

            AZ_Assert(m_graph, "A graph must be created before any nodes are created.");

            if (!m_graph)
            {
                return nullptr;
            }

            ScriptCanvas::ScriptCanvasId scriptCanvasId = m_graph->GetScriptCanvasId();
            configurableNodeEntity->CreateComponent<ScriptCanvas::GraphVariableManagerComponent>(scriptCanvasId);

            configurableNodeEntity->Init();

            m_graph->Activate();

            m_graph->AddNode(configurableNodeEntity->GetId());

            return configurableNode;
        }

        void ReportErrors(ScriptCanvas::Graph* graph, bool expectErrors = false, bool expectIrrecoverableErrors = false)
        {
            AZ_UNUSED(graph);
            AZ_UNUSED(expectErrors);
            AZ_UNUSED(expectIrrecoverableErrors);
        }

        void TestConnectionBetween(ScriptCanvas::Endpoint sourceEndpoint, ScriptCanvas::Endpoint targetEndpoint, bool isValid = true)
        {
            EXPECT_EQ(m_graph->CanConnectionExistBetween(sourceEndpoint, targetEndpoint).IsSuccess(), isValid);
            EXPECT_EQ(m_graph->CanConnectionExistBetween(targetEndpoint, sourceEndpoint).IsSuccess(), isValid);

            EXPECT_EQ(m_graph->CanCreateConnectionBetween(sourceEndpoint, targetEndpoint).IsSuccess(), isValid);
            EXPECT_EQ(m_graph->CanCreateConnectionBetween(targetEndpoint, sourceEndpoint).IsSuccess(), isValid);

            if (isValid)
            {
                EXPECT_TRUE(m_graph->ConnectByEndpoint(sourceEndpoint, targetEndpoint));
            }
        }

        void TestIsConnectionPossible(ScriptCanvas::Endpoint sourceEndpoint, ScriptCanvas::Endpoint targetEndpoint, bool isValid = true)
        {
            EXPECT_EQ(m_graph->CanConnectionExistBetween(sourceEndpoint, targetEndpoint).IsSuccess(), isValid);
            EXPECT_EQ(m_graph->CanConnectionExistBetween(targetEndpoint, sourceEndpoint).IsSuccess(), isValid);

            EXPECT_EQ(m_graph->CanCreateConnectionBetween(sourceEndpoint, targetEndpoint).IsSuccess(), isValid);
            EXPECT_EQ(m_graph->CanCreateConnectionBetween(targetEndpoint, sourceEndpoint).IsSuccess(), isValid);
        }

        // Test if there is an existing connection between the provided endpoints
        void TestIsConnectionBetween(const ScriptCanvas::Endpoint& sourceEndpoint, const ScriptCanvas::Endpoint& targetEndpoint, bool isValid = true)
        {
            AZ::Entity* ent;

            EXPECT_EQ(m_graph->FindConnection(ent, sourceEndpoint, targetEndpoint), isValid);
        }

        // Tests implicit connections between nodes by connecting and disconnecting every data source and data slot while checking to make
        // sure that a connection is maintained between the source and target execution slots as long as at least one set of source and target
        // data slots are connected, and that no other execution out slots are connected to the target execution slot
        void TestAllImplicitConnections(
            ScriptCanvasEditor::EditorGraph* editorGraph,
            AZStd::vector<ScriptCanvas::Endpoint> sourceDataSlots,
            AZStd::vector<ScriptCanvas::Endpoint> targetDataSlots,
            ScriptCanvas::Endpoint sourceExecSlot,
            ScriptCanvas::Endpoint targetExecSlot,
            AZStd::vector<ScriptCanvas::Endpoint> allExecutionOutSlots)
        {
            // Connect all of the data slots
            for (auto sourceDataSlot : sourceDataSlots)
            {
                for (auto targetDataSlot : targetDataSlots)
                {
                    TestConnectionBetween(sourceDataSlot, targetDataSlot, true);
                    editorGraph->UpdateCorrespondingImplicitConnection(sourceDataSlot, targetDataSlot);

                    // Ensure the implicit connection exists
                    TestIsConnectionBetween(sourceExecSlot, targetExecSlot, true);
                    for (auto otherExecSlot : allExecutionOutSlots)
                    {
                        if (otherExecSlot.GetSlotId() != sourceExecSlot.GetSlotId())
                        {
                            // Ensure that no implicit connections exist between any of the other execution out slots and the target
                            // execution slot
                            TestIsConnectionBetween(otherExecSlot, targetExecSlot, false);
                        }
                    }
                }
            }
            // Disconnect all of the data slots
            for (int i = 0; i < sourceDataSlots.size(); i++)
            {
                for (int j = 0; j < targetDataSlots.size(); j++)
                {
                    editorGraph->DisconnectByEndpoint(sourceDataSlots[i], targetDataSlots[j]);
                    editorGraph->UpdateCorrespondingImplicitConnection(sourceDataSlots[i], targetDataSlots[j]);

                    // Ensure the implicit connection exists only if this is not the last data connection. If it is, then ensure that
                    // no implicit connection exists
                    TestIsConnectionBetween(sourceExecSlot, targetExecSlot, (i < sourceDataSlots.size() - 1 || j < targetDataSlots.size() - 1));
                    for (auto otherExecSlot : allExecutionOutSlots)
                    {
                        if (otherExecSlot.GetSlotId() != sourceExecSlot.GetSlotId())
                        {
                            // Ensure that no implicit connections exist between any of the other execution out slots and the target
                            // execution slot
                            TestIsConnectionBetween(otherExecSlot, targetExecSlot, false);
                        }
                    }
                }
            }
        }

        void CreateExecutionFlowBetween(AZStd::vector<TestNodes::ConfigurableUnitTestNode*> unitTestNodes)
        {
            ScriptCanvas::Slot* previousOutSlot = nullptr;

            for (TestNodes::ConfigurableUnitTestNode* testNode : unitTestNodes)
            {
                {
                    ScriptCanvas::ExecutionSlotConfiguration inputSlot = ScriptCanvas::CommonSlots::GeneralInSlot();
                    ScriptCanvas::Slot* slot = testNode->AddTestingSlot(inputSlot);

                    if (slot && previousOutSlot)
                    {
                        TestConnectionBetween(previousOutSlot->GetEndpoint(), slot->GetEndpoint());
                    }
                }

                {
                    ScriptCanvas::ExecutionSlotConfiguration outputSlot = ScriptCanvas::CommonSlots::GeneralOutSlot();
                    previousOutSlot = testNode->AddTestingSlot(outputSlot);
                }
            }
        }

        AZStd::vector< ScriptCanvas::Data::Type > GetContainerDataTypes() const
        {
            return { m_numericVectorType, m_stringToNumberMapType };
        }

        ScriptCanvas::Data::Type GetRandomContainerType() const
        {
            AZStd::vector< ScriptCanvas::Data::Type > containerTypes = GetContainerDataTypes();

            // We have no types to randomize. Just return.
            if (containerTypes.empty())
            {
                return m_numericVectorType;
            }

            int randomIndex = rand() % containerTypes.size();

            ScriptCanvas::Data::Type randomType = containerTypes[randomIndex];
            AZ_TracePrintf("ScriptCanvasTestFixture", "RandomContainerType: %s\n", randomType.GetAZType().ToString<AZStd::string>().c_str());
            return randomType;
        }

        AZStd::vector< ScriptCanvas::Data::Type > GetPrimitiveTypes() const
        {
            return{
                ScriptCanvas::Data::Type::AABB(),
                ScriptCanvas::Data::Type::Boolean(),
                ScriptCanvas::Data::Type::Color(),
                ScriptCanvas::Data::Type::CRC(),
                ScriptCanvas::Data::Type::EntityID(),
                ScriptCanvas::Data::Type::Matrix3x3(),
                ScriptCanvas::Data::Type::Matrix4x4(),
                ScriptCanvas::Data::Type::Number(),
                ScriptCanvas::Data::Type::OBB(),
                ScriptCanvas::Data::Type::Plane(),
                ScriptCanvas::Data::Type::Quaternion(),
                ScriptCanvas::Data::Type::String(),
                ScriptCanvas::Data::Type::Transform(),
                ScriptCanvas::Data::Type::Vector2(),
                ScriptCanvas::Data::Type::Vector3(),
                ScriptCanvas::Data::Type::Vector4()
            };
        }

        ScriptCanvas::Data::Type GetRandomPrimitiveType() const
        {
            AZStd::vector< ScriptCanvas::Data::Type > primitiveTypes = GetPrimitiveTypes();

            // We have no types to randomize. Just return.
            if (primitiveTypes.empty())
            {
                return ScriptCanvas::Data::Type::Number();
            }

            int randomIndex = rand() % primitiveTypes.size();

            ScriptCanvas::Data::Type randomType = primitiveTypes[randomIndex];
            AZ_TracePrintf("ScriptCanvasTestFixture", "RandomPrimitiveType: %s\n", randomType.GetAZType().ToString<AZStd::string>().c_str());
            return randomType;
        }

        AZStd::vector< ScriptCanvas::Data::Type > GetBehaviorObjectTypes() const
        {
            return {
                m_dataSlotConfigurationType
            };
        }

        ScriptCanvas::Data::Type GetRandomObjectType() const
        {
            AZStd::vector< ScriptCanvas::Data::Type > objectTypes = GetBehaviorObjectTypes();

            // We have no types to randomize. Just return.
            if (objectTypes.empty())
            {
                return m_dataSlotConfigurationType;
            }

            int randomIndex = rand() % objectTypes.size();

            ScriptCanvas::Data::Type randomType = objectTypes[randomIndex];
            AZ_TracePrintf("ScriptCanvasTestFixture", "RandomObjectType: %s\n", randomType.GetAZType().ToString<AZStd::string>().c_str());
            return randomType;
        }

        AZStd::vector< ScriptCanvas::Data::Type > GetTypes() const
        {
            auto primitives = GetPrimitiveTypes();
            auto containers = GetContainerDataTypes();
            auto objects = GetBehaviorObjectTypes();

            primitives.reserve(containers.size() + objects.size());

            primitives.insert(primitives.end(), containers.begin(), containers.end());
            primitives.insert(primitives.end(), objects.begin(), objects.end());

            return primitives;
        }

        ScriptCanvas::Data::Type GetRandomType() const
        {
            AZStd::vector< ScriptCanvas::Data::Type > types = GetTypes();

            // We have no types to randomize. Just return.
            if (types.empty())
            {
                return m_dataSlotConfigurationType;
            }

            int randomIndex = rand() % types.size();

            ScriptCanvas::Data::Type randomType = types[randomIndex];
            AZ_TracePrintf("ScriptCanvasTestFixture", "RandomType: %s\n", randomType.GetAZType().ToString<AZStd::string>().c_str());
            return randomType;
        }

        AZStd::string GenerateSlotName()
        {
            AZStd::string slotName = AZStd::string::format("Slot %i", m_slotCounter);
            ++m_slotCounter;

            return slotName;
        }

        AZ::SerializeContext* m_serializeContext;
        AZ::BehaviorContext* m_behaviorContext;
        UnitTestEntityContext m_entityContext;

        // Really big(visually) data types just storing here for ease of use in situations.
        ScriptCanvas::Data::Type m_numericVectorType;
        ScriptCanvas::Data::Type m_stringToNumberMapType;

        ScriptCanvas::Data::Type m_dataSlotConfigurationType;

        ScriptCanvas::Data::Type m_baseClassType;
        ScriptCanvas::Data::Type m_subClassType;

        ScriptCanvas::Graph* m_graph = nullptr;

        int m_slotCounter = 0;

    protected:
        static ScriptCanvasTests::Application* GetApplication() { return s_application; }

    private:

        static bool s_setupSucceeded;
        static inline UnitTest::LeakDetectionBase s_leakDetection{};

        AZStd::unordered_set< AZ::ComponentDescriptor* > m_descriptors;

    };
}
