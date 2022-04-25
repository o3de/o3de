/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/TestEnvironment.h>
#include <Source/GraphModelSystemComponent.h>

namespace GraphModelIntegrationTest
{
    // TestGraphContext
    TestGraphContext::TestGraphContext()
        : GraphModel::GraphContext("GraphModelIntegrationTest", ".nodeTest", {})
    {
        // Construct basic data types
        const AZ::Uuid stringTypeUuid = azrtti_typeid<AZStd::string>();
        const AZ::Uuid entityIdTypeUuid = azrtti_typeid<AZ::EntityId>();
        m_dataTypes.push_back(AZStd::make_shared<GraphModel::DataType>(TestDataTypeEnum::TestDataTypeEnum_String, stringTypeUuid, AZStd::any(AZStd::string("")), "String", "AZStd::string"));
        m_dataTypes.push_back(AZStd::make_shared<GraphModel::DataType>(TestDataTypeEnum::TestDataTypeEnum_EntityId, entityIdTypeUuid, AZStd::any(AZ::EntityId()), "EntityId", "AZ::EntityId"));
    }

    // TestNode
    void TestNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<TestNode, GraphModel::Node>()
                ->Version(0)
                ;
        }
    }

    TestNode::TestNode(GraphModel::GraphPtr graph, AZStd::shared_ptr<TestGraphContext> graphContext)
        : GraphModel::Node(graph)
        , m_graphContext(graphContext)
    {
        RegisterSlots();
        CreateSlotData();
    }

    const char* TestNode::GetTitle() const
    {
        return "TestNode";
    }

    void TestNode::RegisterSlots()
    {
        GraphModel::DataTypePtr stringDataType = m_graphContext->GetDataType(TestDataTypeEnum::TestDataTypeEnum_String);

        RegisterSlot(GraphModel::SlotDefinition::CreateInputData(
            TEST_STRING_INPUT_ID,
            "Test Input",
            stringDataType,
            stringDataType->GetDefaultValue(),
            "A test input slot for String data type"));

        RegisterSlot(GraphModel::SlotDefinition::CreateOutputData(
            TEST_STRING_OUTPUT_ID,
            "Test Output",
            stringDataType,
            "A test output slot for String data type"));

        RegisterSlot(GraphModel::SlotDefinition::CreateInputEvent(
            TEST_EVENT_INPUT_ID,
            "Event In",
            "A test input event slot"));

        RegisterSlot(GraphModel::SlotDefinition::CreateOutputEvent(
            TEST_EVENT_OUTPUT_ID,
            "Event Out",
            "A test output event slot"));
    }

    // ExtendableSlotsNode
    void ExtendableSlotsNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ExtendableSlotsNode, GraphModel::Node>()
                ->Version(0)
                ;
        }
    }

    ExtendableSlotsNode::ExtendableSlotsNode(GraphModel::GraphPtr graph, AZStd::shared_ptr<TestGraphContext> graphContext)
        : GraphModel::Node(graph)
        , m_graphContext(graphContext)
    {
        RegisterSlots();
        CreateSlotData();
    }

    const char* ExtendableSlotsNode::GetTitle() const
    {
        return "ExtendableSlotsNode";
    }

    void ExtendableSlotsNode::RegisterSlots()
    {
        GraphModel::DataTypePtr stringDataType = m_graphContext->GetDataType(TestDataTypeEnum::TestDataTypeEnum_String);

        GraphModel::ExtendableSlotConfiguration inputDataSlotConfig;
        inputDataSlotConfig.m_minimumSlots = 0;
        inputDataSlotConfig.m_maximumSlots = 2;
        inputDataSlotConfig.m_addButtonLabel = "Add String Input";
        inputDataSlotConfig.m_addButtonTooltip = "Add a test string input";
        RegisterSlot(GraphModel::SlotDefinition::CreateInputData(
            TEST_STRING_INPUT_ID,
            "Test Input",
            stringDataType,
            stringDataType->GetDefaultValue(),
            "An extendable input slot for String data type"
            , &inputDataSlotConfig));

        GraphModel::ExtendableSlotConfiguration outputDataSlotConfig;
        outputDataSlotConfig.m_addButtonLabel = "Add String Output";
        outputDataSlotConfig.m_addButtonTooltip = "Add a test string output";
        RegisterSlot(GraphModel::SlotDefinition::CreateOutputData(
            TEST_STRING_OUTPUT_ID,
            "Test Output",
            stringDataType,
            "An extendable output slot for String data type",
            &outputDataSlotConfig));

        GraphModel::ExtendableSlotConfiguration inputEventSlotConfig;
        inputEventSlotConfig.m_addButtonLabel = "Add Input Event";
        inputEventSlotConfig.m_addButtonTooltip = "Add a test event input";
        RegisterSlot(GraphModel::SlotDefinition::CreateInputEvent(
            TEST_EVENT_INPUT_ID,
            "Test Input Event",
            "An extendable input event"
            , &inputEventSlotConfig));

        GraphModel::ExtendableSlotConfiguration outputEventSlotConfig;
        outputEventSlotConfig.m_addButtonLabel = "Add Output Event";
        outputEventSlotConfig.m_addButtonTooltip = "Add a test event output";
        outputEventSlotConfig.m_minimumSlots = 3;
        outputEventSlotConfig.m_maximumSlots = 4;
        RegisterSlot(GraphModel::SlotDefinition::CreateOutputEvent(
            TEST_EVENT_OUTPUT_ID,
            "Test Output Event",
            "An extendable output event"
            , &outputEventSlotConfig));
    }

    // BadNode
    void BadNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<BadNode, GraphModel::Node>()
                ->Version(0)
                ;
        }
    }

    BadNode::BadNode(GraphModel::GraphPtr graph, AZStd::shared_ptr<TestGraphContext> graphContext)
        : GraphModel::Node(graph)
        , m_graphContext(graphContext)
    {
        RegisterSlots();
        CreateSlotData();
    }

    const char* BadNode::GetTitle() const
    {
        return "BadNode";
    }

    void BadNode::RegisterSlots()
    {
        GraphModel::DataTypePtr stringDataType = m_graphContext->GetDataType(TestDataTypeEnum::TestDataTypeEnum_String);

        // This will result in an invalid configuration since the minimum is greater than the maximum
        GraphModel::ExtendableSlotConfiguration inputDataSlotConfig;
        inputDataSlotConfig.m_minimumSlots = 5;
        inputDataSlotConfig.m_maximumSlots = 1;
        inputDataSlotConfig.m_addButtonLabel = "Add String Input";
        inputDataSlotConfig.m_addButtonTooltip = "Add a test string input";
        RegisterSlot(GraphModel::SlotDefinition::CreateInputData(
            TEST_STRING_INPUT_ID,
            "Test Input",
            stringDataType,
            stringDataType->GetDefaultValue(),
            "An extendable input slot for String data type"
            , &inputDataSlotConfig));
    }

    // GraphModelTestEnvironment
    void GraphModelTestEnvironment::SetupEnvironment()
    {
        // Setup a system allocator
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create();

        // Create application and descriptor
        m_application = aznew AZ::ComponentApplication;
        AZ::ComponentApplication::Descriptor appDesc;
        appDesc.m_useExistingAllocator = true;

        // Create basic system entity
        AZ::ComponentApplication::StartupParameters startupParams;
        m_systemEntity = m_application->Create(appDesc, startupParams);
        m_systemEntity->AddComponent(aznew AZ::MemoryComponent());
        m_systemEntity->AddComponent(aznew AZ::AssetManagerComponent());
        m_systemEntity->AddComponent(aznew AZ::JobManagerComponent());
        m_systemEntity->AddComponent(aznew AZ::StreamerComponent());
        m_systemEntity->AddComponent(aznew GraphModel::GraphModelSystemComponent());

        // Register descriptor for the GraphModelSystemComponent
        m_application->RegisterComponentDescriptor(GraphModel::GraphModelSystemComponent::CreateDescriptor());

        // Register descriptors for our mock components
        m_application->RegisterComponentDescriptor(MockGraphCanvasServices::MockNodeComponent::CreateDescriptor());
        m_application->RegisterComponentDescriptor(MockGraphCanvasServices::MockSlotComponent::CreateDescriptor());
        m_application->RegisterComponentDescriptor(MockGraphCanvasServices::MockDataSlotComponent::CreateDescriptor());
        m_application->RegisterComponentDescriptor(MockGraphCanvasServices::MockExecutionSlotComponent::CreateDescriptor());
        m_application->RegisterComponentDescriptor(MockGraphCanvasServices::MockExtenderSlotComponent::CreateDescriptor());
        m_application->RegisterComponentDescriptor(MockGraphCanvasServices::MockGraphCanvasSystemComponent::CreateDescriptor());

        // Register our mock GraphCanvasSystemComponent
        m_systemEntity->AddComponent(aznew MockGraphCanvasServices::MockGraphCanvasSystemComponent());

        m_systemEntity->Init();
        m_systemEntity->Activate();
    }

    void GraphModelTestEnvironment::TeardownEnvironment()
    {
        delete m_application;

        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }
}
