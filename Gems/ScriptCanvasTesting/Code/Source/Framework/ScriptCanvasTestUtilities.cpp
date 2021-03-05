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

#include <Editor/Framework/ScriptCanvasTraceUtilities.h>
#include <Framework/ScriptCanvasTestUtilities.h>
#include <Framework/ScriptCanvasTestNodes.h>
#include <Editor/Framework/ScriptCanvasGraphUtilities.h>
#include <Asset/EditorAssetSystemComponent.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/FileIOEventBus.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/RuntimeAssetHandler.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>

namespace ScriptCanvasTestUtilitiesCPP
{
    const char* k_defaultExtension = "scriptcanvas";
    const char* k_unitTestDirPathRelative = "@engroot@/Gems/ScriptCanvasTesting/Assets/ScriptCanvas/UnitTests";
} // namespace ScriptCanvasTestUtilitiesCPP

namespace ScriptCanvasTests
{
    using namespace ScriptCanvas;

#define SC_CORE_UNIT_TEST_DIR "@exefolder@/LY_SC_UnitTest_ScriptCanvas_CoreCPP_Temporary"
#define SC_CORE_UNIT_TEST_NAME "serializationTest.scriptcanvas_compiled"
    const char* k_tempCoreAssetDir = SC_CORE_UNIT_TEST_DIR;
    const char* k_tempCoreAssetName = SC_CORE_UNIT_TEST_NAME;
    const char* k_tempCoreAssetPath = SC_CORE_UNIT_TEST_DIR "/" SC_CORE_UNIT_TEST_NAME;
#undef SC_CORE_UNIT_TEST_DIR
#undef SC_CORE_UNIT_TEST_NAME

    AZStd::string_view GetGraphNameFromPath(AZStd::string_view graphPath)
    {
        graphPath.remove_prefix(AZStd::min(graphPath.find_last_of("\\/") + 1, graphPath.size()));
        return graphPath;
    }

    void VerifyReporter(const ScriptCanvasEditor::Reporter& reporter)
    {
        if (!reporter.GetScriptCanvasId().IsValid())
        {
            ADD_FAILURE() << "Graph is not valid";
        }
        else if (reporter.IsReportFinished())
        {
            bool reportCheckpoints = false;

            const auto& successes = reporter.GetSuccess();
            for (const auto& success : successes)
            {
                SUCCEED() << success.data();
            }

            if (!reporter.IsActivated())
            {
                ADD_FAILURE() << "Graph did not activate";
            }

            if (!reporter.IsDeactivated())
            {
                ADD_FAILURE() << "Graph did not deactivate";
                reportCheckpoints = true;
            }

            if (!reporter.IsErrorFree())
            {
                ADD_FAILURE() << "Graph had errors";
                reportCheckpoints = true;
            }

            const auto& failures = reporter.GetFailure();
            for (const auto& failure : failures)
            {
                ADD_FAILURE() << failure.data();
            }

            if (!reporter.IsComplete())
            {
                ADD_FAILURE() << "Graph was not marked complete";
                reportCheckpoints = true;
            }

            if (reportCheckpoints)
            {
                const auto& checkpoints = reporter.GetCheckpoints();
                if (checkpoints.empty())
                {
                    ADD_FAILURE() << "No checkpoints or other unit test nodes found, using them can help parse graph test failures";
                }
                else
                {
                    AZStd::string checkpointPath = "Checkpoint Path:\n";
                    int i = 0;

                    for (const auto& checkpoint : checkpoints)
                    {
                        checkpointPath += AZStd::string::format("%2d: %s\n", ++i, checkpoint.data()).data();
                    }

                    ADD_FAILURE() << checkpointPath.data();
                }
            }
        }
        else
        {
            ADD_FAILURE() << "Graph report did not finish";
        }
    }

    void RunUnitTestGraph(AZStd::string_view graphPath, ExecutionMode execution, const ScriptCanvasEditor::DurationSpec& duration)
    {
        const AZStd::string filePath = AZStd::string::format("%s/%s.%s", ScriptCanvasTestUtilitiesCPP::k_unitTestDirPathRelative, graphPath.data(), ScriptCanvasTestUtilitiesCPP::k_defaultExtension);
        const ScriptCanvasEditor::Reporter reporter = ScriptCanvasEditor::RunGraph(filePath, execution, duration);
        ScriptCanvasTests::VerifyReporter(reporter);
    }

    void RunUnitTestGraph(AZStd::string_view graphPath, const ScriptCanvasEditor::DurationSpec& duration)
    {
        ScriptCanvasTests::RunUnitTestGraph(graphPath, ExecutionMode::Interpreted, duration);
    }

    void RunUnitTestGraph(AZStd::string_view graphPath, ExecutionMode execution)
    {
        ScriptCanvasTests::RunUnitTestGraph(graphPath, execution, ScriptCanvasEditor::DurationSpec());
    }

    void RunUnitTestGraph(AZStd::string_view graphPath)
    {
        ScriptCanvasTests::RunUnitTestGraph(graphPath, ScriptCanvasEditor::DurationSpec());
    }

    void RunUnitTestGraphMixed(AZStd::string_view graphPath, const ScriptCanvasEditor::DurationSpec& duration)
    {
        const ScriptCanvasEditor::Reporter reporterIterpreted0 = RunGraph(graphPath, ExecutionMode::Interpreted, duration);
        const ScriptCanvasEditor::Reporter reporterNative0 = RunGraph(graphPath, ExecutionMode::Native, duration);
        const ScriptCanvasEditor::Reporter reporterIterpreted1 = RunGraph(graphPath, ExecutionMode::Interpreted, duration);
        const ScriptCanvasEditor::Reporter reporterNative1 = RunGraph(graphPath, ExecutionMode::Native, duration);

        VerifyReporter(reporterIterpreted0);
        VerifyReporter(reporterNative0);

        EXPECT_TRUE(reporterIterpreted0.IsActivated());
        EXPECT_TRUE(reporterIterpreted1.IsComplete());
        EXPECT_TRUE(reporterIterpreted0.IsErrorFree());

        EXPECT_EQ(reporterNative0, reporterIterpreted0);
        EXPECT_EQ(reporterNative0, reporterIterpreted1);
        EXPECT_EQ(reporterNative1, reporterIterpreted0);
        EXPECT_EQ(reporterNative1, reporterIterpreted1);

        EXPECT_EQ(reporterNative0, reporterNative1);
        EXPECT_EQ(reporterIterpreted0, reporterIterpreted1);
    }

    ScriptCanvasEditor::LoadTestGraphResult LoadTestGraph(AZStd::string_view graphPath)
    {
        const AZStd::string filePath = AZStd::string::format("%s/%s.%s", ScriptCanvasTestUtilitiesCPP::k_unitTestDirPathRelative, graphPath.data(), ScriptCanvasTestUtilitiesCPP::k_defaultExtension);
        AZ::Outcome< AZ::Data::Asset<ScriptCanvas::RuntimeAsset>, AZStd::string> assetOutcome = AZ::Failure(AZStd::string("asset creation failed"));
        ScriptCanvasEditor::EditorAssetConversionBus::BroadcastResult(assetOutcome, &ScriptCanvasEditor::EditorAssetConversionBusTraits::CreateRuntimeAsset, filePath);

        if (assetOutcome.IsSuccess())
        {
            ScriptCanvasEditor::LoadTestGraphResult result;
            result.m_asset = assetOutcome.GetValue();
            result.m_graphEntity = AZStd::make_unique<AZ::Entity>("Loaded Graph");
            result.m_graph = result.m_graphEntity->CreateComponent<ScriptCanvas::RuntimeComponent>(result.m_asset);
            return result;
        }

        return {};
    }

    void RunUnitTestGraphMixed(AZStd::string_view graphPath)
    {
        RunUnitTestGraphMixed(graphPath, ScriptCanvasEditor::DurationSpec());
    }

    TestBehaviorContextObject TestBehaviorContextObject::MaxReturnByValue(TestBehaviorContextObject lhs, TestBehaviorContextObject rhs)
    {
        return lhs.GetValue() >= rhs.GetValue() ? lhs : rhs;
    }

    const TestBehaviorContextObject* TestBehaviorContextObject::MaxReturnByPointer(const TestBehaviorContextObject* lhs, const TestBehaviorContextObject* rhs)
    {
        return (lhs && rhs && lhs->GetValue() >= rhs->GetValue()) ? lhs : rhs;
    }

    const TestBehaviorContextObject& TestBehaviorContextObject::MaxReturnByReference(const TestBehaviorContextObject& lhs, const TestBehaviorContextObject& rhs)
    {
        return lhs.GetValue() >= rhs.GetValue() ? lhs : rhs;
    }

    int TestBehaviorContextObject::MaxReturnByValueInteger(int lhs, int rhs)
    {
        return lhs >= rhs ? lhs : rhs;
    }

    const int* TestBehaviorContextObject::MaxReturnByPointerInteger(const int* lhs, const int* rhs)
    {
        return (lhs && rhs && (*lhs) >= (*rhs)) ? lhs : rhs;
    }

    const int& TestBehaviorContextObject::MaxReturnByReferenceInteger(const int& lhs, const int& rhs)
    {
        return lhs >= rhs ? lhs : rhs;
    }

    static void TestBehaviorContextObjectGenericConstructor(TestBehaviorContextObject* thisPtr)
    {
        new (thisPtr) TestBehaviorContextObject();
        thisPtr->SetValue(0);
    }

    void TestBehaviorContextObject::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<TestBehaviorContextObject>()
                ->Version(0)
                ->Field("m_value", &TestBehaviorContextObject::m_value)
                ->Field("isNormalized", &TestBehaviorContextObject::m_isNormalized)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
        {
            behaviorContext->Class<TestBehaviorContextObject>("TestBehaviorContextObject")
                ->Attribute(AZ::Script::Attributes::ConstructorOverride, &TestBehaviorContextObjectGenericConstructor)
                ->Attribute(AZ::Script::Attributes::GenericConstructorOverride, &TestBehaviorContextObjectGenericConstructor)
                ->Method("In", &TestBehaviorContextObject::GetValue)
                ->Method("Out", &TestBehaviorContextObject::SetValue)
                ->Method("Normalize", &TestBehaviorContextObject::Normalize)
                ->Method("IsNormalized", &TestBehaviorContextObject::IsNormalized)
                ->Method("Denormalize", &TestBehaviorContextObject::Denormalize)
                ->Method("MaxReturnByValue", &TestBehaviorContextObject::MaxReturnByValue)
                ->Method("MaxReturnByPointer", &TestBehaviorContextObject::MaxReturnByPointer)
                ->Method("MaxReturnByReference", &TestBehaviorContextObject::MaxReturnByReference)
                ->Method("MaxReturnByValueInteger", &TestBehaviorContextObject::MaxReturnByValueInteger)
                ->Method("MaxReturnByPointerInteger", &TestBehaviorContextObject::MaxReturnByPointerInteger)
                ->Method("MaxReturnByReferenceInteger", &TestBehaviorContextObject::MaxReturnByReferenceInteger)
                ->Method<bool (TestBehaviorContextObject::*)(const TestBehaviorContextObject&) const>("LessThan", &TestBehaviorContextObject::operator<)
                ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::LessThan)
                ->Method<bool (TestBehaviorContextObject::*)(const TestBehaviorContextObject&) const>("LessEqualThan", &TestBehaviorContextObject::operator<=)
                ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::LessEqualThan)
                ->Method<bool (TestBehaviorContextObject::*)(const TestBehaviorContextObject&) const>("Equal", &TestBehaviorContextObject::operator==)
                ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                ;
        }
    }

    TestBehaviorContextObject MaxReturnByValue(TestBehaviorContextObject lhs, TestBehaviorContextObject rhs)
    {
        return lhs.GetValue() >= rhs.GetValue() ? lhs : rhs;
    }

    const TestBehaviorContextObject* MaxReturnByPointer(const TestBehaviorContextObject* lhs, const TestBehaviorContextObject* rhs)
    {
        return (lhs && rhs && (lhs->GetValue() >= rhs->GetValue())) ? lhs : rhs;
    }

    const TestBehaviorContextObject& MaxReturnByReference(const TestBehaviorContextObject& lhs, const TestBehaviorContextObject& rhs)
    {
        return lhs.GetValue() >= rhs.GetValue() ? lhs : rhs;
    }

    AZ::u32 TestBehaviorContextObject::s_createdCount = 0;
    AZ::u32 TestBehaviorContextObject::s_destroyedCount = 0;

    ScriptCanvas::Nodes::Core::BehaviorContextObjectNode* CreateTestObjectNode(const AZ::EntityId& graphUniqueId, AZ::EntityId& entityOut, const AZ::Uuid& objectTypeID)
    {
        ScriptCanvas::Nodes::Core::BehaviorContextObjectNode* node = CreateTestNode<ScriptCanvas::Nodes::Core::BehaviorContextObjectNode>(graphUniqueId, entityOut);
        EXPECT_TRUE(node != nullptr);
        node->InitializeObject(objectTypeID);
        return node;
    }

    AZ::EntityId CreateClassFunctionNode(const ScriptCanvasId& scriptCanvasId, AZStd::string_view className, AZStd::string_view methodName)
    {
        using namespace ScriptCanvas;

        ScriptCanvas::Namespaces emptyNamespaces;

        AZ::Entity* splitEntity{ aznew AZ::Entity };
        splitEntity->Init();
        AZ::EntityId methodNodeID{ splitEntity->GetId() };
        SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, methodNodeID, scriptCanvasId, Nodes::Core::Method::RTTI_Type());
        Nodes::Core::Method* methodNode(nullptr);
        SystemRequestBus::BroadcastResult(methodNode, &SystemRequests::GetNode<Nodes::Core::Method>, methodNodeID);
        EXPECT_TRUE(methodNode != nullptr);
        methodNode->InitializeClassOrBus(emptyNamespaces, className, methodName);
        return methodNodeID;
    }

    ScriptCanvas::Node* CreateDataNodeByType(const ScriptCanvasId& scriptCanvasId, const ScriptCanvas::Data::Type& type, AZ::EntityId& nodeIDout)
    {
        using namespace ScriptCanvas;

        Node* node(nullptr);

        switch (type.GetType())
        {
        case Data::eType::AABB:
            node = CreateTestNode<Nodes::Math::AABB>(scriptCanvasId, nodeIDout);
            break;

        case Data::eType::BehaviorContextObject:
        {
            auto objectNode = CreateTestNode<Nodes::Core::BehaviorContextObjectNode>(scriptCanvasId, nodeIDout);
            objectNode->InitializeObject(type);
            node = objectNode;
        }
        break;

        case Data::eType::Boolean:
            node = CreateTestNode<Nodes::Logic::Boolean>(scriptCanvasId, nodeIDout);
            break;

        case Data::eType::Color:
            node = CreateTestNode<Nodes::Math::Color>(scriptCanvasId, nodeIDout);
            break;

        case Data::eType::CRC:
            node = CreateTestNode<Nodes::Math::CRC>(scriptCanvasId, nodeIDout);
            break;

        case Data::eType::EntityID:
            node = CreateTestNode<Nodes::Entity::EntityRef>(scriptCanvasId, nodeIDout);
            break;

        case Data::eType::Matrix3x3:
            node = CreateTestNode<Nodes::Math::Matrix3x3>(scriptCanvasId, nodeIDout);
            break;

        case Data::eType::Matrix4x4:
            node = CreateTestNode<Nodes::Math::Matrix4x4>(scriptCanvasId, nodeIDout);
            break;

        case Data::eType::Number:
            node = CreateTestNode<Nodes::Math::Number>(scriptCanvasId, nodeIDout);
            break;

        case Data::eType::OBB:
            node = CreateTestNode<Nodes::Math::OBB>(scriptCanvasId, nodeIDout);
            break;

        case Data::eType::Plane:
            node = CreateTestNode<Nodes::Math::Plane>(scriptCanvasId, nodeIDout);
            break;

        case Data::eType::Quaternion:
            node = CreateTestNode<Nodes::Math::Quaternion>(scriptCanvasId, nodeIDout);
            break;

        case Data::eType::String:
            node = CreateTestNode<Nodes::Core::String>(scriptCanvasId, nodeIDout);
            break;

        case Data::eType::Transform:
            node = CreateTestNode<Nodes::Math::Transform>(scriptCanvasId, nodeIDout);
            break;

        case Data::eType::Vector2:
            node = CreateTestNode<Nodes::Math::Vector2>(scriptCanvasId, nodeIDout);
            break;

        case Data::eType::Vector3:
            node = CreateTestNode<Nodes::Math::Vector3>(scriptCanvasId, nodeIDout);
            break;

        case Data::eType::Vector4:
            node = CreateTestNode<Nodes::Math::Vector4>(scriptCanvasId, nodeIDout);
            break;

        default:
            AZ_Error("ScriptCanvas", false, "unsupported data type");
        }

        node->SetExecutionType(ExecutionType::Runtime);

        return node;
    }

    AZStd::string SlotDescriptorToString(ScriptCanvas::SlotDescriptor descriptor)
    {
        AZStd::string name;

        switch (descriptor.m_slotType)
        {
        case SlotTypeDescriptor::Data:
            name.append("Data");
            break;
        case SlotTypeDescriptor::Execution:
            name.append("Execution");
            break;
        default:
            break;
        }

        switch (descriptor.m_connectionType)
        {
        case ConnectionType::Input:
            name.append("In");
            break;
        case ConnectionType::Output:
            name.append("Out");
            break;
        default:
            break;
        }

        return name;
    }

    void DumpSlots(const ScriptCanvas::Node& node)
    {
        const auto& nodeslots = node.GetSlots();

        for (const auto& nodeslot : nodeslots)
        {
            AZ_TracePrintf("ScriptCanvasTest", "'%s':%s\n", nodeslot.GetName().data(), SlotDescriptorToString(nodeslot.GetDescriptor()).c_str());
        }
    }

    bool Connect(ScriptCanvas::Graph& graph, const AZ::EntityId& fromNodeID, const char* fromSlotName, const AZ::EntityId& toNodeID, const char* toSlotName, bool dumpSlotsOnFailure /*= true*/)
    {
        using namespace ScriptCanvas;
        AZ::Entity* fromNode{};
        AZ::ComponentApplicationBus::BroadcastResult(fromNode, &AZ::ComponentApplicationBus::Events::FindEntity, fromNodeID);
        AZ::Entity* toNode{};
        AZ::ComponentApplicationBus::BroadcastResult(toNode, &AZ::ComponentApplicationBus::Events::FindEntity, toNodeID);
        if (fromNode && toNode)
        {
            Node* from = AZ::EntityUtils::FindFirstDerivedComponent<Node>(fromNode);
            Node* to = AZ::EntityUtils::FindFirstDerivedComponent<Node>(toNode);

            auto fromSlotId = from->GetSlotId(fromSlotName);
            auto toSlotId = to->GetSlotId(toSlotName);

            if (graph.Connect(fromNodeID, fromSlotId, toNodeID, toSlotId))
            {
                return true;
            }
            else if (dumpSlotsOnFailure)
            {
                AZ_TracePrintf("ScriptCanvasTest", "Slots from:\n");
                DumpSlots(*from);
                AZ_TracePrintf("ScriptCanvasTest", "\nSlots to:\n");
                DumpSlots(*to);
            }
        }

        return false;
    }

    AZ::Data::Asset<ScriptCanvas::RuntimeAsset> CreateRuntimeAsset(ScriptCanvas::Graph* graph)
    {
        using namespace ScriptCanvas;
        RuntimeAssetHandler runtimeAssetHandler;
        AZ::Data::Asset<RuntimeAsset> runtimeAsset;
        runtimeAsset.Create(AZ::Uuid::CreateRandom());

        AZ::SerializeContext* serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();
        serializeContext->CloneObjectInplace(runtimeAsset.Get()->GetData().m_graphData, graph->GetGraphDataConst());

        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> graphToAssetEntityIdMap{ { graph->GetScriptCanvasId(), ScriptCanvas::UniqueId } };
        AZ::IdUtils::Remapper<AZ::EntityId>::GenerateNewIdsAndFixRefs(&runtimeAsset.Get()->GetData().m_graphData, graphToAssetEntityIdMap, serializeContext);

        return runtimeAsset;
    }

    AZ::Entity* UnitTestEntityContext::CreateEntity(const char* name)
    {
        auto entity = aznew AZ::Entity(name);
        AddEntity(entity);
        return entity;
    }

    void UnitTestEntityContext::AddEntity(AZ::Entity* entity)
    {
        AddEntity(entity->GetId());
    }

    void UnitTestEntityContext::AddEntity(AZ::EntityId entityId)
    {
        AZ_Assert(!AzFramework::EntityIdContextQueryBus::FindFirstHandler(entityId), "Entity already belongs to a context.");
        m_unitTestEntityIdMap.emplace(entityId, entityId);
        AzFramework::EntityIdContextQueryBus::MultiHandler::BusConnect(entityId);
    }

    void UnitTestEntityContext::RemoveEntity(AZ::EntityId entityId)
    {
        auto foundIt = m_unitTestEntityIdMap.find(entityId);
        if (foundIt != m_unitTestEntityIdMap.end())
        {
            AzFramework::EntityIdContextQueryBus::MultiHandler::BusDisconnect(entityId);
            m_unitTestEntityIdMap.erase(foundIt);
        }
    }

    void UnitTestEntityContext::ActivateEntity(AZ::EntityId entityId)
    {
        if (IsOwnedByThisContext(entityId))
        {
            AZ::Entity* entity{};
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
            if (entity)
            {
                if (entity->GetState() == AZ::Entity::State::Constructed)
                {
                    entity->Init();
                }
                if (entity->GetState() == AZ::Entity::State::Init)
                {
                    entity->Activate();
                }
            }
        }
    }

    void UnitTestEntityContext::DeactivateEntity(AZ::EntityId entityId)
    {
        if (IsOwnedByThisContext(entityId))
        {
            AZ::Entity* entity{};
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
            if (entity)
            {
                if (entity->GetState() == AZ::Entity::State::Active)
                {
                    entity->Deactivate();
                }
                else if (entity->GetState() == AZ::Entity::State::Activating)
                {
                    // Queue Deactivation to next frame
                    AZ::SystemTickBus::QueueFunction(&AZ::Entity::Activate, entity);
                }
            }
        }
    }

    bool UnitTestEntityContext::DestroyEntity(AZ::Entity* entity)
    {
        if (entity)
        {
            auto foundIt = m_unitTestEntityIdMap.find(entity->GetId());
            if (foundIt != m_unitTestEntityIdMap.end())
            {
                AzFramework::EntityIdContextQueryBus::MultiHandler::BusDisconnect(entity->GetId());
                m_unitTestEntityIdMap.erase(foundIt);
                delete entity;
                return true;
            }
        }

        return false;
    }

    bool UnitTestEntityContext::DestroyEntityById(AZ::EntityId entityId)
    {
        AZ::Entity* entity{};
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
        return DestroyEntity(entity);
    }

    void UnitTestEntityContext::ResetContext()
    {
        AzFramework::EntityIdContextQueryBus::MultiHandler::BusDisconnect();
        m_unitTestEntityIdMap.clear();
    }

    AZ::EntityId UnitTestEntityContext::FindLoadedEntityIdMapping(const AZ::EntityId& staticId) const
    {
        auto idIter = m_unitTestEntityIdMap.find(staticId);

        if (idIter != m_unitTestEntityIdMap.end())
        {
            return idIter->second;
        }

        return AZ::EntityId();
    }

    AZ::Entity* UnitTestEntityContext::CloneEntity(const AZ::Entity& sourceEntity)
    {
        if (!IsOwnedByThisContext(sourceEntity.GetId()))
        {
            AZ_Warning("Script Canvas", false, "Entity %s does not belong to the unit test entity context.", sourceEntity.GetName().data());
            return {};
        }

        AZ::SerializeContext* serializeContext{};
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ::Entity* cloneEntity = serializeContext->CloneObject(&sourceEntity);
        if (cloneEntity)
        {
            cloneEntity->SetId(AZ::Entity::MakeId());
            AddEntity(cloneEntity);
        }

        return cloneEntity;
    }

    AZ::Data::AssetId UnitTestEntityContext::CurrentlyInstantiatingSlice()
    {
        return AZ::Data::AssetId();
    }

    bool UnitTestEntityContext::HandleRootEntityReloadedFromStream(AZ::Entity*, bool, AZ::SliceComponent::EntityIdToEntityIdMap*)
    {
        return true;
    }

    AZ::SliceComponent* UnitTestEntityContext::GetRootSlice()
    {
        return {};
    }

    const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& UnitTestEntityContext::GetLoadedEntityIdMap()
    {
        return m_unitTestEntityIdMap;
    }

    AzFramework::SliceInstantiationTicket UnitTestEntityContext::InstantiateSlice(const AZ::Data::Asset<AZ::Data::AssetData>&,
        const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper&, const AZ::Data::AssetFilterCB&)
    {
        return AzFramework::SliceInstantiationTicket();
    }

    AZ::SliceComponent::SliceInstanceAddress UnitTestEntityContext::CloneSliceInstance(
        AZ::SliceComponent::SliceInstanceAddress, AZ::SliceComponent::EntityIdToEntityIdMap&)
    {
        return AZ::SliceComponent::SliceInstanceAddress();
    }

    void UnitTestEntityContext::CancelSliceInstantiation(const AzFramework::SliceInstantiationTicket&)
    {
    }

    AzFramework::SliceInstantiationTicket UnitTestEntityContext::GenerateSliceInstantiationTicket()
    {
        return AzFramework::SliceInstantiationTicket();
    }

    void UnitTestEntityContext::SetIsDynamic(bool)
    {
    }

    const AzFramework::RootSliceAsset& UnitTestEntityContext::GetRootAsset() const
    {
        return m_unitTestRootAsset;
    }

} // ScriptCanvasTests
