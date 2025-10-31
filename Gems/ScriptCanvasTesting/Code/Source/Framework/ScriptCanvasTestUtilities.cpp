/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Asset/EditorAssetSystemComponent.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <Editor/Framework/ScriptCanvasGraphUtilities.h>
#include <Editor/Framework/ScriptCanvasTraceUtilities.h>
#include <Framework/ScriptCanvasTestNodes.h>
#include <Framework/ScriptCanvasTestUtilities.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/RuntimeAssetHandler.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Libraries/Core/Method.h>

namespace ScriptCanvasTestUtilitiesCPP
{
    const char* k_defaultExtension = "scriptcanvas";
    const char* k_scriptEventExtension = "scriptevents";
    const char* k_unitTestDirPathRelative = "@gemroot:ScriptCanvasTesting@/Assets/ScriptCanvas/UnitTests";
}

namespace ScriptCanvasTests
{
    const char* GetUnitTestDirPathRelative()
    {
        return ScriptCanvasTestUtilitiesCPP::k_unitTestDirPathRelative;
    }

    void ExpectParse(AZStd::string_view graphPath)
    {
        using namespace ScriptCanvas;

        AZ_TEST_START_TRACE_SUPPRESSION;
        const AZStd::string filePath = AZStd::string::format("%s/%s.%s", ScriptCanvasTestUtilitiesCPP::k_unitTestDirPathRelative, graphPath.data(), ScriptCanvasTestUtilitiesCPP::k_defaultExtension);

        ScriptCanvasEditor::RunGraphSpec runGraphSpec;
        runGraphSpec.graphPath = filePath;
        runGraphSpec.dirPath = ScriptCanvasTestUtilitiesCPP::k_unitTestDirPathRelative;
        runGraphSpec.runSpec.processOnly = true;
        runGraphSpec.runSpec.execution = ExecutionMode::Interpreted;
        auto reporters = ScriptCanvasEditor::RunGraph(runGraphSpec);
        ScriptCanvasTests::VerifyReporter(reporters.front());
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
    }

    void ExpectParseError(AZStd::string_view graphPath)
    {
        using namespace ScriptCanvas;

        AZ_TEST_START_TRACE_SUPPRESSION;
        const AZStd::string filePath = AZStd::string::format("%s/%s.%s", ScriptCanvasTestUtilitiesCPP::k_unitTestDirPathRelative, graphPath.data(), ScriptCanvasTestUtilitiesCPP::k_defaultExtension);
        ScriptCanvasEditor::RunGraphSpec runGraphSpec;
        runGraphSpec.runSpec.processOnly = true;
        runGraphSpec.graphPath = filePath;
        runGraphSpec.dirPath = ScriptCanvasTestUtilitiesCPP::k_unitTestDirPathRelative;
        runGraphSpec.runSpec.execution = ExecutionMode::Interpreted;
        auto reporters = ScriptCanvasEditor::RunGraph(runGraphSpec);
        reporters.front().MarkExpectParseError();
        ScriptCanvasTests::VerifyReporter(reporters.front());
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
    }

    AZStd::string_view GetGraphNameFromPath(AZStd::string_view graphPath)
    {
        graphPath.remove_prefix(AZStd::min(graphPath.find_last_of("\\/") + 1, graphPath.size()));
        return graphPath;
    }

    void VerifyReporter(const ScriptCanvasEditor::Reporter& reporter)
    {
        using namespace ScriptCanvas;


        if (!reporter.IsGraphLoaded())
        {
            ADD_FAILURE() << "Graph was not successfully loaded.\n" << reporter.GetFilePath().c_str();
        }
        else if (reporter.ExpectsParseError())
        {
            if (!reporter.IsParseAttemptMade())
            {
                ADD_FAILURE() << "Expected a parse error but the graph never attempted to be parsed\n" << reporter.GetFilePath().c_str();
            }
            else if (reporter.IsCompiled())
            {
                ADD_FAILURE() << "Expected a parse error but graph compiled successfully\n" << reporter.GetFilePath().c_str();
            }
        }
        else if (!reporter.IsCompiled())
        {
            ADD_FAILURE() << "Graph failed to compile\n" << reporter.GetFilePath().c_str();
        }
        else if (reporter.IsReportFinished())
        {
            bool reportCheckpoints = false;

            if (!reporter.IsProcessOnly())
            {
                const auto& successes = reporter.GetSuccess();
                for (const auto& success : successes)
                {
                    SUCCEED() << success.c_str();
                }

                if (!reporter.IsActivated())
                {
                    ADD_FAILURE() << "Graph did not activate\n" << reporter.GetFilePath().c_str();
                }

                if (!reporter.IsDeactivated())
                {
                    ADD_FAILURE() << "Graph did not deactivate\n" << reporter.GetFilePath().c_str();
                    reportCheckpoints = true;
                }

                if (!reporter.ExpectsRuntimeFailure())
                {
                    if (!reporter.IsComplete())
                    {
                        ADD_FAILURE() << "Graph was not marked complete\n" << reporter.GetFilePath().c_str();
                        reportCheckpoints = true;
                    }

                    if (!reporter.IsErrorFree())
                    {
                        ADD_FAILURE() << "Graph execution had errors\n" << reporter.GetFilePath().c_str();
                        reportCheckpoints = true;

                        const auto& failures = reporter.GetFailure();
                        for (const auto& failure : failures)
                        {
                            ADD_FAILURE() << failure.c_str();
                        }
                    }
                }
                else
                {
                    if (reporter.IsErrorFree())
                    {
                        ADD_FAILURE() << "Graph expected error, but didn't report any\n" << reporter.GetFilePath().c_str();
                        reportCheckpoints = true;
                    }
                }
            }

            if (reportCheckpoints && !reporter.IsProcessOnly())
            {
                const auto& checkpoints = reporter.GetCheckpoints();
                if (checkpoints.empty())
                {
                    ADD_FAILURE() << "No checkpoints or other unit test nodes found, using them can help parse graph test failures\n" << reporter.GetFilePath().c_str();
                }
                else
                {
                    AZStd::string checkpointPath = "Checkpoint Path:\n";
                    int i = 0;

                    for (const auto& checkpoint : checkpoints)
                    {
                        checkpointPath += AZStd::string::format("%2d: %s\n", ++i, checkpoint.c_str()).c_str();
                    }

                    ADD_FAILURE() << checkpointPath.c_str();
                }
            }
            else
            {
                const Execution::PerformanceTrackingReport& performance = reporter.GetPerformanceReport();

                if (reporter.GetExecutionMode() == ExecutionMode::Interpreted)
                {
                    std::cerr << "[INTERPRETED] ";
                }
                else
                {
                    std::cerr << "[     NATIVE] ";
                }

                std::cerr << AZStd::string::format
                    (" Parse: %4.2f ms, Translate: %4.2f ms\n"
                    , reporter.GetParseDuration() / 1000.0
                    , reporter.GetTranslateDuration() / 1000.0).c_str();

                double ready = aznumeric_caster(performance.timing.initializationTime);
                double instant = aznumeric_caster(performance.timing.executionTime);
                double latent = aznumeric_caster(performance.timing.latentTime);
                double total = aznumeric_caster(performance.timing.totalTime);

                std::cerr << "[ INITIALIZE] " << AZStd::string::format("%7.3f ms \n", ready / 1000.0).c_str();

                std::cerr << "[  EXECUTION] " << AZStd::string::format("%7.3f ms \n", instant / 1000.0).c_str();

                std::cerr << "[     LATENT] " << AZStd::string::format("%7.3f ms \n", latent / 1000.0).c_str();

                std::cerr << "[      TOTAL] " << AZStd::string::format("%7.3f ms ", total / 1000.0).c_str();

                switch (reporter.GetExecutionConfiguration())
                {
                case ScriptCanvasEditor::ExecutionConfiguration::Debug:
                    std::cerr << "[  DEBUG] ";
                    break;
                case ScriptCanvasEditor::ExecutionConfiguration::Performance:
                    std::cerr << "[PERFORM] ";
                    break;
                case ScriptCanvasEditor::ExecutionConfiguration::Release:
                    std::cerr << "[RELEASE] ";
                    break;
                case ScriptCanvasEditor::ExecutionConfiguration::Traced:
                    std::cerr << "[ TRACED] ";
                    break;
                }
                std::cerr << "\n";
            }
        }
        else
        {
            ADD_FAILURE() << "Graph report did not finish\n" << reporter.GetFilePath().c_str();
        }
    }

    void RunUnitTestGraph(AZStd::string_view graphPath)
    {
        ScriptCanvasTests::RunUnitTestGraph(graphPath, ScriptCanvasEditor::RunSpec());
    }

    void RunUnitTestGraph(AZStd::string_view graphPath, ScriptCanvas::ExecutionMode execution)
    {
        ScriptCanvasEditor::RunSpec spec;
        spec.execution = execution;
        ScriptCanvasTests::RunUnitTestGraph(graphPath, spec);
    }

    void RunUnitTestGraph(AZStd::string_view graphPath, ScriptCanvas::ExecutionMode execution, const ScriptCanvasEditor::DurationSpec& duration)
    {
        ScriptCanvasEditor::RunSpec runSpec;
        runSpec.execution = execution;
        runSpec.duration = duration;
        RunUnitTestGraph(graphPath, runSpec);
    }

    void RunUnitTestGraph(AZStd::string_view graphPath, ScriptCanvas::ExecutionMode execution, AZStd::string_view dependentScriptEvent)
    {
        AZ::Data::AssetType assetType(azrtti_typeid<ScriptEvents::ScriptEventsAsset>());
        if (auto scriptEventAssetHandler = AZ::Data::AssetManager::Instance().GetHandler(assetType))
        {
            const AZStd::string fullPath = AZStd::string::format("%s/%s.%s", ScriptCanvasTestUtilitiesCPP::k_unitTestDirPathRelative, dependentScriptEvent.data(), ScriptCanvasTestUtilitiesCPP::k_scriptEventExtension);

            // Preload dependent scriptevent asset for testing
            AZStd::shared_ptr<AZ::Data::AssetDataStream> assetDataStream = AZStd::make_shared<AZ::Data::AssetDataStream>();
            // Read in the data from a file to a buffer, then hand ownership of the buffer over to the assetDataStream
            {
                AZ::IO::FileIOStream stream(fullPath.c_str(), AZ::IO::OpenMode::ModeRead);
                if (!AZ::IO::RetryOpenStream(stream))
                {
                    ADD_FAILURE() << AZStd::string::format("CreateJobs for \"%s\" failed because the source file could not be opened.", fullPath.c_str()).data();
                    return;
                }
                AZStd::vector<AZ::u8> fileBuffer(stream.GetLength());
                size_t bytesRead = stream.Read(fileBuffer.size(), fileBuffer.data());
                if (bytesRead != stream.GetLength())
                {
                    ADD_FAILURE() << AZStd::string::format("CreateJobs for \"%s\" failed because the source file could not be read.", fullPath.c_str()).data();
                    return;
                }
                assetDataStream->Open(AZStd::move(fileBuffer));
            }

            AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset;
            const AZStd::string hintPath = AZStd::string::format("scriptcanvas/unittests/%s.%s", dependentScriptEvent.data(), ScriptCanvasTestUtilitiesCPP::k_scriptEventExtension);
            asset.Create(AZ::Data::AssetId(AZ::Uuid::CreateName(hintPath.data())));

            if (scriptEventAssetHandler->LoadAssetDataFromStream(asset, assetDataStream, nullptr) != AZ::Data::AssetHandler::LoadResult::LoadComplete)
            {
                ADD_FAILURE() << AZStd::string::format("Failed to load ScriptEvent asset: %s", fullPath.data()).data();
                return;
            }
            scriptEventAssetHandler->InitAsset(asset, true, false);

            ScriptCanvasEditor::RunSpec spec;
            spec.execution = execution;
            ScriptCanvasTests::RunUnitTestGraph(graphPath, spec);
        }
        else
        {
            ADD_FAILURE() << "ScriptEvent asset handler is missing.";
        }
    }

    void RunUnitTestGraph(AZStd::string_view graphPath, const ScriptCanvasEditor::DurationSpec& duration)
    {
        ScriptCanvasEditor::RunSpec spec;
        spec.duration = duration;
        ScriptCanvasTests::RunUnitTestGraph(graphPath, ScriptCanvas::ExecutionMode::Interpreted, duration);
    }

    void RunUnitTestGraph(AZStd::string_view graphPath, const ScriptCanvasEditor::RunSpec& runSpec)
    {
        const AZStd::string filePath = AZStd::string::format("%s/%s.%s", ScriptCanvasTestUtilitiesCPP::k_unitTestDirPathRelative, graphPath.data(), ScriptCanvasTestUtilitiesCPP::k_defaultExtension);

        ScriptCanvasEditor::RunGraphSpec runGraphSpec;
        runGraphSpec.graphPath = filePath;
        runGraphSpec.dirPath = ScriptCanvasTestUtilitiesCPP::k_unitTestDirPathRelative;
        runGraphSpec.runSpec = runSpec;

        AZ_TEST_START_TRACE_SUPPRESSION;

        const ScriptCanvasEditor::Reporters reporters = ScriptCanvasEditor::RunGraph(runGraphSpec);

        for (const auto& reporter : reporters)
        {
            ScriptCanvasTests::VerifyReporter(reporter);
        }

        if (reporters.size() == 2)
        {
            EXPECT_EQ(reporters.front(), reporters.back());
        }
        else if (reporters.size() > 2)
        {
            for (size_t lhs(0), rhs(1); rhs != reporters.size(); ++lhs, ++rhs)
            {
                EXPECT_EQ(reporters[lhs], reporters[rhs]);
            }
        }

        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
    }

    void RunUnitTestGraphMixed(AZStd::string_view graphPath, const ScriptCanvasEditor::DurationSpec& duration)
    {
        using namespace ScriptCanvas;

        AZ_TEST_START_TRACE_SUPPRESSION;

        ScriptCanvasEditor::RunGraphSpec runGraphSpec;
        runGraphSpec.graphPath = graphPath;
        runGraphSpec.dirPath = ScriptCanvasTestUtilitiesCPP::k_unitTestDirPathRelative;
        runGraphSpec.runSpec.duration = duration;

        runGraphSpec.runSpec.execution = ExecutionMode::Interpreted;
        const ScriptCanvasEditor::Reporter reporterIterpreted0 = RunGraph(runGraphSpec).front();
        runGraphSpec.runSpec.execution = ExecutionMode::Native;
        const ScriptCanvasEditor::Reporter reporterNative0 = RunGraph(runGraphSpec).front();
        runGraphSpec.runSpec.execution = ExecutionMode::Interpreted;
        const ScriptCanvasEditor::Reporter reporterIterpreted1 = RunGraph(runGraphSpec).front();
        runGraphSpec.runSpec.execution = ExecutionMode::Native;
        const ScriptCanvasEditor::Reporter reporterNative1 = RunGraph(runGraphSpec).front();

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
        
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
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

    AZ::EntityId CreateClassFunctionNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId, AZStd::string_view className, AZStd::string_view methodName)
    {
        using namespace ScriptCanvas;

        ScriptCanvas::NamespacePath emptyNamespaces;

        AZ::Entity* splitEntity{ aznew AZ::Entity };
        splitEntity->Init();
        AZ::EntityId methodNodeID{ splitEntity->GetId() };
        SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, methodNodeID, scriptCanvasId, ScriptCanvas::Nodes::Core::Method::RTTI_Type());
        ScriptCanvas::Nodes::Core::Method* methodNode(nullptr);
        SystemRequestBus::BroadcastResult(methodNode, &SystemRequests::GetNode<ScriptCanvas::Nodes::Core::Method>, methodNodeID);
        EXPECT_TRUE(methodNode != nullptr);
        methodNode->InitializeBehaviorMethod(emptyNamespaces, className, methodName, ScriptCanvas::PropertyStatus::None);
        return methodNodeID;
    }

    AZStd::string SlotDescriptorToString(ScriptCanvas::SlotDescriptor descriptor)
    {
        using namespace ScriptCanvas;
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

    void DumpSlots([[maybe_unused]] const ScriptCanvas::Node& node)
    {
#if defined(AZ_ENABLE_TRACING)
        const auto& nodeslots = node.GetSlots();

        for (const auto& nodeslot : nodeslots)
        {
            AZ_TracePrintf("ScriptCanvasTest", "'%s':%s\n", nodeslot.GetName().data(), SlotDescriptorToString(nodeslot.GetDescriptor()).c_str());
        }
#endif
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

    //AZ::Data::AssetId UnitTestEntityContext::CurrentlyInstantiatingSlice()
    //{
    //    return AZ::Data::AssetId();
    //}

    //bool UnitTestEntityContext::HandleRootEntityReloadedFromStream(AZ::Entity*, bool, AZ::SliceComponent::EntityIdToEntityIdMap*)
    //{
    //    return true;
    //}

    //AZ::SliceComponent* UnitTestEntityContext::GetRootSlice()
    //{
    //    return {};
    //}

    //const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& UnitTestEntityContext::GetLoadedEntityIdMap()
    //{
    //    return m_unitTestEntityIdMap;
    //}

    //AzFramework::SliceInstantiationTicket UnitTestEntityContext::InstantiateSlice(const AZ::Data::Asset<AZ::Data::AssetData>&,
    //    const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper&, const AZ::Data::AssetFilterCB&)
    //{
    //    return AzFramework::SliceInstantiationTicket();
    //}

    //AZ::SliceComponent::SliceInstanceAddress UnitTestEntityContext::CloneSliceInstance(
    //    AZ::SliceComponent::SliceInstanceAddress, AZ::SliceComponent::EntityIdToEntityIdMap&)
    //{
    //    return AZ::SliceComponent::SliceInstanceAddress();
    //}

    //void UnitTestEntityContext::CancelSliceInstantiation(const AzFramework::SliceInstantiationTicket&)
    //{
    //}

    //AzFramework::SliceInstantiationTicket UnitTestEntityContext::GenerateSliceInstantiationTicket()
    //{
    //    return AzFramework::SliceInstantiationTicket();
    //}

    //void UnitTestEntityContext::SetIsDynamic(bool)
    //{
    //}

    //const AzFramework::RootSliceAsset& UnitTestEntityContext::GetRootAsset() const
    //{
    //    return m_unitTestRootAsset;
    //}

} // ScriptCanvasTests
