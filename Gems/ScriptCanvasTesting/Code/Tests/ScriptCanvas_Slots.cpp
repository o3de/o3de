/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */



#include <Source/Framework/ScriptCanvasTestFixture.h>
#include <Source/Framework/ScriptCanvasTestUtilities.h>
#include <Source/Framework/ScriptCanvasTestNodes.h>

#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/std/containers/vector.h>

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/RuntimeAssetHandler.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Core/SlotConfigurationDefaults.h>

using namespace ScriptCanvasTests;
using namespace TestNodes;

// Test General Descriptor Functionality
TEST_F(ScriptCanvasTestFixture, SlotDescriptors_General)
{
    using namespace ScriptCanvas;
    
    SlotDescriptor dataIn = SlotDescriptors::DataIn();
    EXPECT_TRUE(dataIn.IsData());
    EXPECT_FALSE(dataIn.IsExecution());
    EXPECT_TRUE(dataIn.IsInput());    
    EXPECT_FALSE(dataIn.IsOutput());

    SlotDescriptor dataOut = SlotDescriptors::DataOut();
    EXPECT_TRUE(dataOut.IsData());
    EXPECT_FALSE(dataOut.IsExecution());
    EXPECT_FALSE(dataOut.IsInput());
    EXPECT_TRUE(dataOut.IsOutput());

    SlotDescriptor executionIn = SlotDescriptors::ExecutionIn();
    EXPECT_FALSE(executionIn.IsData());
    EXPECT_TRUE(executionIn.IsExecution());
    EXPECT_TRUE(executionIn.IsInput());
    EXPECT_FALSE(executionIn.IsOutput());

    SlotDescriptor executionOut = SlotDescriptors::ExecutionOut();    
    EXPECT_FALSE(executionOut.IsData());
    EXPECT_TRUE(executionOut.IsExecution());
    EXPECT_FALSE(executionOut.IsInput());
    EXPECT_TRUE(executionOut.IsOutput());

    EXPECT_TRUE(dataIn == dataIn);
    EXPECT_FALSE(dataIn != dataIn);
    EXPECT_TRUE(dataIn != dataOut);

    EXPECT_TRUE(executionIn == executionIn);
    EXPECT_FALSE(executionIn != executionIn);
    EXPECT_TRUE(executionIn != executionOut);

    // Test connectability between all of the different descriptors.
    for (SlotDescriptor baseDescriptor : { dataIn, dataOut, executionIn, executionOut })
    {
        AZStd::vector< SlotDescriptor > connectableDescriptors;
        AZStd::vector< SlotDescriptor > unconnectableDescriptors;

        if (baseDescriptor == dataIn)
        {
            connectableDescriptors = { dataOut };
            unconnectableDescriptors = { dataIn, executionIn, executionOut };
        }
        else if (baseDescriptor == dataOut)
        {
            connectableDescriptors = { dataIn };
            unconnectableDescriptors = { dataOut, executionIn, executionOut };
        }
        else if (baseDescriptor == executionIn)
        {
            connectableDescriptors = { executionOut };
            unconnectableDescriptors = { dataIn, dataOut, executionIn };
        }
        else if (baseDescriptor == executionOut)
        {
            connectableDescriptors = { executionIn };
            unconnectableDescriptors = { dataIn, dataOut, executionOut };
        }

        for (SlotDescriptor testDescriptor : connectableDescriptors)
        {
            EXPECT_TRUE(baseDescriptor.CanConnectTo(testDescriptor));
        }

        for (SlotDescriptor testDescriptor : unconnectableDescriptors)
        {
            EXPECT_FALSE(baseDescriptor.CanConnectTo(testDescriptor));
        }
    }
}

// Basic acid test of all of the slot creations. Bare bones test of basic functionality
TEST_F(ScriptCanvasTestFixture, SlotCreation_GeneralCreation)
{
    using namespace ScriptCanvas;

    CreateGraph();

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();

    {
        Slot* inSlot = emptyNode->AddTestingSlot(CommonSlots::GeneralInSlot());

        EXPECT_TRUE(inSlot->IsExecution());
        EXPECT_FALSE(inSlot->IsData());
        EXPECT_FALSE(inSlot->IsDynamicSlot());

        EXPECT_TRUE(inSlot->IsInput());
        EXPECT_FALSE(inSlot->IsOutput());
    }

    {
        Slot* outSlot = emptyNode->AddTestingSlot(CommonSlots::GeneralOutSlot());

        EXPECT_TRUE(outSlot->IsExecution());
        EXPECT_FALSE(outSlot->IsData());
        EXPECT_FALSE(outSlot->IsDynamicSlot());

        EXPECT_FALSE(outSlot->IsInput());
        EXPECT_TRUE(outSlot->IsOutput());
    }

    {
        DataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = "DataIn";
        slotConfiguration.SetType(ScriptCanvas::Data::Type::Number());
        slotConfiguration.SetConnectionType(ConnectionType::Input);

        Slot* dataInSlot = emptyNode->AddTestingSlot(slotConfiguration);

        EXPECT_FALSE(dataInSlot->IsExecution());
        EXPECT_TRUE(dataInSlot->IsData());
        EXPECT_FALSE(dataInSlot->IsDynamicSlot());

        EXPECT_TRUE(dataInSlot->IsInput());
        EXPECT_FALSE(dataInSlot->IsOutput());
    }

    {
        DataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = "DataOut";
        slotConfiguration.SetType(ScriptCanvas::Data::Type::Number());
        slotConfiguration.SetConnectionType(ConnectionType::Output);

        Slot* dataOutSlot = emptyNode->AddTestingSlot(slotConfiguration);

        EXPECT_FALSE(dataOutSlot->IsExecution());
        EXPECT_TRUE(dataOutSlot->IsData());
        EXPECT_FALSE(dataOutSlot->IsDynamicSlot());

        EXPECT_FALSE(dataOutSlot->IsInput());
        EXPECT_TRUE(dataOutSlot->IsOutput());
    }

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = "DynamicIn";
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
        slotConfiguration.SetConnectionType(ConnectionType::Input);

        Slot* dynamicDataInSlot = emptyNode->AddTestingSlot(slotConfiguration);

        EXPECT_FALSE(dynamicDataInSlot->IsExecution());
        EXPECT_TRUE(dynamicDataInSlot->IsData());
        EXPECT_TRUE(dynamicDataInSlot->IsDynamicSlot());

        EXPECT_TRUE(dynamicDataInSlot->IsInput());
        EXPECT_FALSE(dynamicDataInSlot->IsOutput());
    }

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = "DynamicOut";
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
        slotConfiguration.SetConnectionType(ConnectionType::Output);

        Slot* dynamicDataOutSlot = emptyNode->AddTestingSlot(slotConfiguration);

        EXPECT_FALSE(dynamicDataOutSlot->IsExecution());
        EXPECT_TRUE(dynamicDataOutSlot->IsData());
        EXPECT_TRUE(dynamicDataOutSlot->IsDynamicSlot());

        EXPECT_FALSE(dynamicDataOutSlot->IsInput());
        EXPECT_TRUE(dynamicDataOutSlot->IsOutput());
    }
}

// More specific Unit Test for testing all of the configurations of DataSlots
TEST_F(ScriptCanvasTestFixture, SlotCreation_DataSlotCreation)
{
    using namespace ScriptCanvas;

    CreateGraph();

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();

    for (auto dataType : GetTypes())
    {
        {
            DataSlotConfiguration slotConfiguration;

            slotConfiguration.m_name = GenerateSlotName();
            slotConfiguration.SetType(dataType);
            slotConfiguration.SetConnectionType(ConnectionType::Input);

            Slot* dataInSlot = emptyNode->AddTestingSlot(slotConfiguration);

            EXPECT_FALSE(dataInSlot->IsExecution());
            EXPECT_TRUE(dataInSlot->IsData());
            EXPECT_TRUE(dataInSlot->IsTypeMatchFor(dataType));
            EXPECT_FALSE(dataInSlot->IsDynamicSlot());

            EXPECT_TRUE(dataInSlot->IsInput());
            EXPECT_FALSE(dataInSlot->IsOutput());
            
            const Datum* datum = emptyNode->FindDatum(dataInSlot->GetId());

            EXPECT_TRUE(datum != nullptr);

            if (datum)
            {
                EXPECT_TRUE(datum->IS_A(dataType));

                for (auto secondDataType : GetTypes())
                {
                    if (dataType == secondDataType)
                    {
                        continue;
                    }

                    EXPECT_FALSE(datum->IS_A(secondDataType));
                }
            }
        }

        {
            DataSlotConfiguration slotConfiguration;

            slotConfiguration.m_name = GenerateSlotName();
            slotConfiguration.SetType(dataType);
            slotConfiguration.SetConnectionType(ConnectionType::Output);

            Slot* dataOutSlot = emptyNode->AddTestingSlot(slotConfiguration);

            EXPECT_FALSE(dataOutSlot->IsExecution());
            EXPECT_TRUE(dataOutSlot->IsData());
            EXPECT_TRUE(dataOutSlot->IsTypeMatchFor(dataType));
            EXPECT_FALSE(dataOutSlot->IsDynamicSlot());

            EXPECT_FALSE(dataOutSlot->IsInput());
            EXPECT_TRUE(dataOutSlot->IsOutput());

            const Datum* datum = emptyNode->FindDatum(dataOutSlot->GetId());

            EXPECT_TRUE(datum == nullptr);
        }
    }
}

// Acid Test of connecting Execution slots to each other.
TEST_F(ScriptCanvasTestFixture, SlotConnecting_ExecutionBasic)
{
    using namespace ScriptCanvas;

    CreateGraph();
    ConfigurableUnitTestNode* inputNode = CreateConfigurableNode();
    ConfigurableUnitTestNode* outputNode = CreateConfigurableNode();

    Slot* outputSlot = outputNode->AddTestingSlot(CommonSlots::GeneralOutSlot());
    Slot* inputSlot = inputNode->AddTestingSlot(CommonSlots::GeneralInSlot());

    Endpoint outputEndpoint = Endpoint(outputNode->GetEntityId(), outputSlot->GetId());
    Endpoint inputEndpoint = Endpoint(inputNode->GetEntityId(), inputSlot->GetId());

    TestConnectionBetween(outputEndpoint, inputEndpoint, true);
}

// Test implicit connections against a simple node that has no slot execution map
TEST_F(ScriptCanvasTestFixture, SlotConnecting_ImplicitConnections)
{
    using namespace ScriptCanvas;

    auto editorGraph = CreateEditorGraph();

    // Node before node that creates implicit connections
    ConfigurableUnitTestNode* sourceNode = CreateConfigurableNode();
    Endpoint execOutSlot = sourceNode->AddTestingSlot(CommonSlots::Execution("Out", ConnectionType::Output))->GetEndpoint();
    Endpoint dataOutSlot1 = sourceNode->AddTestingSlot(CommonSlots::FloatData("out1", ConnectionType::Output))->GetEndpoint();
    Endpoint dataOutSlot2 = sourceNode->AddTestingSlot(CommonSlots::FloatData("out2", ConnectionType::Output))->GetEndpoint();

    // Node that creates implicit connections
    ConfigurableUnitTestNode* targetNode = CreateConfigurableNode();
    Endpoint implicitSlot = targetNode->AddTestingSlot(CommonSlots::Execution("In", ConnectionType::Input, false, true))->GetEndpoint();
    Endpoint dataInSlot1 = targetNode->AddTestingSlot(CommonSlots::FloatData("in1", ConnectionType::Input))->GetEndpoint();
    Endpoint dataInSlot2 = targetNode->AddTestingSlot(CommonSlots::FloatData("in2", ConnectionType::Input))->GetEndpoint();

    // Test the implicit connections between the two nodes
    TestAllImplicitConnections(editorGraph, { dataOutSlot1, dataOutSlot2 }, { dataInSlot1, dataInSlot2 }, execOutSlot, implicitSlot, { execOutSlot });
}

// Test implicit connections against a complex node that has a slot execution map
TEST_F(ScriptCanvasTestFixture, SlotConnecting_ImplicitConnectionsSlotExecutionMap)
{
    using namespace ScriptCanvas;

    auto editorGraph = CreateEditorGraph();

    // These vectors store each "set" of data output endpoints that correspond with one execution out
    AZStd::vector<Endpoint> dataOutEndpointSet1;
    AZStd::vector<Endpoint> dataOutEndpointSet2;
    AZStd::vector<Endpoint> dataOutEndpointSet3;
    AZStd::vector<Endpoint> dataOutEndpointSet4;
    AZStd::vector<Endpoint> dataOutEndpointSet5;

    // The data in slots for the compact node
    AZStd::vector<Endpoint> dataInEndpointSet;

    // Vector of every execution output pin. Used to make sure implicit connections are only being made when they need to
    AZStd::vector<Endpoint> executions;

    // Execution ins and outs for slot execution map
    SlotExecution::Ins ins;
    SlotExecution::Outs outs;


    // Node before the node that creates implicit connections
    ConfigurableUnitTestNode* sourceNode = CreateConfigurableNode();

    // Simple execution in mapped to one execution out with two corresponding data out slots
    SlotExecution::In in1;

    Endpoint execIn1 = sourceNode->AddTestingSlot(ExecutionSlotConfiguration("In1", ConnectionType::Input))->GetEndpoint();
    in1.slotId = execIn1.GetSlotId();

    SlotExecution::Out out1;

    Endpoint execOut1 = sourceNode->AddTestingSlot(ExecutionSlotConfiguration("Out1", ConnectionType::Output))->GetEndpoint();
    out1.slotId = execOut1.GetSlotId();
    executions.push_back(execOut1);

    Endpoint dataOut1a = sourceNode->AddTestingSlot(CommonSlots::FloatData("out1a", ConnectionType::Output))->GetEndpoint();
    out1.outputs.emplace_back(dataOut1a.GetSlotId());
    dataOutEndpointSet1.push_back(dataOut1a);

    Endpoint dataOut1b = sourceNode->AddTestingSlot(CommonSlots::FloatData("out1b", ConnectionType::Output))->GetEndpoint();
    out1.outputs.emplace_back(dataOut1b.GetSlotId());
    dataOutEndpointSet1.push_back(dataOut1b);

    in1.outs.emplace_back(out1);

    ins.push_back(in1);

    // Execution in mapped to two execution out slots. Each execution out slot has two corresponding data out slots
    SlotExecution::In in2;

    Endpoint execIn2 = sourceNode->AddTestingSlot(ExecutionSlotConfiguration("In2", ConnectionType::Input))->GetEndpoint();
    in2.slotId = execIn2.GetSlotId();

    SlotExecution::Out out2a;

    Endpoint execOut2a = sourceNode->AddTestingSlot(ExecutionSlotConfiguration("Out2a", ConnectionType::Output))->GetEndpoint();
    out2a.slotId = execOut2a.GetSlotId();
    executions.push_back(execOut2a);

    Endpoint dataOut2aa = sourceNode->AddTestingSlot(CommonSlots::FloatData("out2aa", ConnectionType::Output))->GetEndpoint();
    out2a.outputs.emplace_back(dataOut2aa.GetSlotId());
    dataOutEndpointSet2.push_back(dataOut2aa);

    Endpoint dataOut2ab = sourceNode->AddTestingSlot(CommonSlots::FloatData("out2ab", ConnectionType::Output))->GetEndpoint();
    out2a.outputs.emplace_back(dataOut2ab.GetSlotId());
    dataOutEndpointSet2.push_back(dataOut2ab);

    in2.outs.emplace_back(out2a);

    SlotExecution::Out out2b;

    Endpoint execOut2b = sourceNode->AddTestingSlot(ExecutionSlotConfiguration("Out2b", ConnectionType::Output))->GetEndpoint();
    out2b.slotId = execOut2b.GetSlotId();
    executions.push_back(execOut2b);

    Endpoint dataOut2ba = sourceNode->AddTestingSlot(CommonSlots::FloatData("out2ba", ConnectionType::Output))->GetEndpoint();
    out2b.outputs.emplace_back(dataOut2ba.GetSlotId());
    dataOutEndpointSet3.push_back(dataOut2ba);

    Endpoint dataOut2bb = sourceNode->AddTestingSlot(CommonSlots::FloatData("out2bb", ConnectionType::Output))->GetEndpoint();
    out2b.outputs.emplace_back(dataOut2bb.GetSlotId());
    dataOutEndpointSet3.push_back(dataOut2bb);

    in2.outs.emplace_back(out2b);

    ins.push_back(in2);

    // Simple execution in mapped to one execution out with one corresponding data out slot
    SlotExecution::In in3;

    Endpoint execIn3 = sourceNode->AddTestingSlot(ExecutionSlotConfiguration("In3", ConnectionType::Input))->GetEndpoint();
    in3.slotId = execIn3.GetSlotId();

    SlotExecution::Out out3;

    Endpoint execOut3 = sourceNode->AddTestingSlot(ExecutionSlotConfiguration("Out3", ConnectionType::Output))->GetEndpoint();
    out3.slotId = execOut3.GetSlotId();
    executions.push_back(execOut3);

    Endpoint dataOut3 = sourceNode->AddTestingSlot(CommonSlots::FloatData("out3", ConnectionType::Output))->GetEndpoint();
    out3.outputs.emplace_back(dataOut3.GetSlotId());
    dataOutEndpointSet4.push_back(dataOut3);

    in3.outs.emplace_back(out3);

    ins.push_back(in3);

    // Latent execution out slot with two corresponding data out slots
    SlotExecution::Out latOut1;

    Endpoint latExecOut1 =
        sourceNode->AddTestingSlot(CommonSlots::Execution("LatOut1", ConnectionType::Output, true))->GetEndpoint();
    latOut1.slotId = latExecOut1.GetSlotId();
    executions.push_back(latExecOut1);

    Endpoint latDataOut1a =
        sourceNode->AddTestingSlot(CommonSlots::FloatData("latOut1a", ConnectionType::Output, true))->GetEndpoint();
    latOut1.outputs.emplace_back(latDataOut1a.GetSlotId());
    dataOutEndpointSet5.push_back(latDataOut1a);

    Endpoint latDataOut1b =
        sourceNode->AddTestingSlot(CommonSlots::FloatData("latOut1b", ConnectionType::Output, true))->GetEndpoint();
    latOut1.outputs.emplace_back(latDataOut1b.GetSlotId());
    dataOutEndpointSet5.push_back(latDataOut1b);

    outs.push_back(latOut1);

    // Configure the slot execution map on the source node
    auto map = aznew SlotExecution::Map(AZStd::move(ins), AZStd::move(outs));

    sourceNode->SetSlotExecutionMap(map);


    // Node that creates implicit connections
    ConfigurableUnitTestNode* targetNode = CreateConfigurableNode();

    // Implicit Execution input with two data inputs
    Endpoint compImpExecIn = targetNode->AddTestingSlot(CommonSlots::Execution("impExec", ConnectionType::Input, false, true))->GetEndpoint();

    Endpoint compDataIn1 = targetNode->AddTestingSlot(CommonSlots::FloatData("compDataIn1", ConnectionType::Input))->GetEndpoint();
    dataInEndpointSet.push_back(compDataIn1);

    Endpoint compDataIn2 = targetNode->AddTestingSlot(CommonSlots::FloatData("compDataIn2", ConnectionType::Input))->GetEndpoint();
    dataInEndpointSet.push_back(compDataIn2);


    // Test to make sure implicit connections are being made correctly in each set of data slots
    TestAllImplicitConnections(editorGraph, dataOutEndpointSet1, dataInEndpointSet, execOut1, compImpExecIn, executions);

    TestAllImplicitConnections(editorGraph, dataOutEndpointSet2, dataInEndpointSet, execOut2a, compImpExecIn, executions);

    TestAllImplicitConnections(editorGraph, dataOutEndpointSet3, dataInEndpointSet, execOut2b, compImpExecIn, executions);

    TestAllImplicitConnections(editorGraph, dataOutEndpointSet4, dataInEndpointSet, execOut3, compImpExecIn, executions);

    TestAllImplicitConnections(editorGraph, dataOutEndpointSet5, dataInEndpointSet, latExecOut1, compImpExecIn, executions);

    delete map;
}

// Exhaustive test of connecting Execution to a variety of invalid targets.
TEST_F(ScriptCanvasTestFixture, SlotConnecting_ExecutionFailure)
{
    using namespace ScriptCanvas;

    const bool invalidConnection = false;

    CreateGraph();
    ConfigurableUnitTestNode* inputNode = CreateConfigurableNode();
    ConfigurableUnitTestNode* outputNode = CreateConfigurableNode();

    Slot* outputSlot = outputNode->AddTestingSlot(CommonSlots::GeneralOutSlot());
    Endpoint outputEndpoint = Endpoint(outputNode->GetEntityId(), outputSlot->GetId());

    Slot* inputSlot = inputNode->AddTestingSlot(CommonSlots::GeneralInSlot());
    Endpoint inputEndpoint = Endpoint(inputNode->GetEntityId(), inputSlot->GetId());

    AZStd::unordered_map< Data::Type, ScriptCanvas::Endpoint > inputTypeMapping;

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.SetConnectionType(ConnectionType::Input);
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;

        Slot* dynamicAnyInSlot = inputNode->AddTestingSlot(slotConfiguration);
        Endpoint dynamicAnyInEndpoint = Endpoint(inputNode->GetEntityId(), dynamicAnyInSlot->GetId());

        TestConnectionBetween(outputEndpoint, dynamicAnyInEndpoint, invalidConnection);
    }

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.SetConnectionType(ConnectionType::Output);
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;

        Slot* dynamicAnyOutSlot = outputNode->AddTestingSlot(slotConfiguration);
        Endpoint dynamicAnyOutEndpoint = Endpoint(outputNode->GetEntityId(), dynamicAnyOutSlot->GetId());

        TestConnectionBetween(dynamicAnyOutEndpoint, inputEndpoint, invalidConnection);
    }

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.SetConnectionType(ConnectionType::Input);
        slotConfiguration.m_dynamicDataType = DynamicDataType::Container;

        Slot* dynamicContainerInSlot = inputNode->AddTestingSlot(slotConfiguration);
        Endpoint dynamicContainerInEndpoint = Endpoint(inputNode->GetEntityId(), dynamicContainerInSlot->GetId());

        TestConnectionBetween(outputEndpoint, dynamicContainerInEndpoint, invalidConnection);
    }

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.SetConnectionType(ConnectionType::Output);
        slotConfiguration.m_dynamicDataType = DynamicDataType::Container;

        Slot* dynamicContainerOutSlot = outputNode->AddTestingSlot(slotConfiguration);
        Endpoint dynamicContainerOutEndpoint = Endpoint(outputNode->GetEntityId(), dynamicContainerOutSlot->GetId());

        TestConnectionBetween(dynamicContainerOutEndpoint, inputEndpoint, invalidConnection);
    }

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.SetConnectionType(ConnectionType::Input);
        slotConfiguration.m_dynamicDataType = DynamicDataType::Value;

        Slot* dynamicValueInSlot = inputNode->AddTestingSlot(slotConfiguration);
        Endpoint dynamicValueInEndpoint = Endpoint(inputNode->GetEntityId(), dynamicValueInSlot->GetId());
        TestConnectionBetween(outputEndpoint, dynamicValueInEndpoint, invalidConnection);
    }

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.SetConnectionType(ConnectionType::Output);
        slotConfiguration.m_dynamicDataType = DynamicDataType::Value;

        Slot* dynamicValueOutSlot = outputNode->AddTestingSlot(slotConfiguration);
        Endpoint dynamicValueOutEndpoint = Endpoint(outputNode->GetEntityId(), dynamicValueOutSlot->GetId());

        TestConnectionBetween(dynamicValueOutEndpoint, inputEndpoint, invalidConnection);
    }
    
    for (auto type : GetTypes())
    {
        Endpoint dataInputEndpoint;
        Endpoint dataOutputEndpoint;

        {
            DataSlotConfiguration slotConfiguration;

            slotConfiguration.m_name = GenerateSlotName();
            slotConfiguration.SetConnectionType(ConnectionType::Input);
            slotConfiguration.SetType(type);

            Slot* inputSlot2 = inputNode->AddTestingSlot(slotConfiguration);
            dataInputEndpoint = Endpoint(inputNode->GetEntityId(), inputSlot2->GetId());
        }

        {
            DataSlotConfiguration slotConfiguration;

            slotConfiguration.m_name = GenerateSlotName();
            slotConfiguration.SetConnectionType(ConnectionType::Output);
            slotConfiguration.SetType(type);

            Slot* outputSlot2 = outputNode->AddTestingSlot(slotConfiguration);
            dataOutputEndpoint = Endpoint(outputNode->GetEntityId(), outputSlot2->GetId());
        }

        TestConnectionBetween(outputEndpoint, dataInputEndpoint, invalidConnection);
        TestConnectionBetween(dataOutputEndpoint, inputEndpoint, invalidConnection);
    }
}

// Basic acid test of Data Connections.
TEST_F(ScriptCanvasTestFixture, SlotConnecting_DataBasic)
{
    using namespace ScriptCanvas;

    CreateGraph();
    ConfigurableUnitTestNode* inputNode = CreateConfigurableNode();
    ConfigurableUnitTestNode* outputNode = CreateConfigurableNode();

    Endpoint inputEndpoint;

    {
        DataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();        
        slotConfiguration.SetType(Data::Type::Number());
        slotConfiguration.SetConnectionType(ConnectionType::Input);

        Slot* inputSlot = inputNode->AddTestingSlot(slotConfiguration);

        inputEndpoint = Endpoint(inputNode->GetEntityId(), inputSlot->GetId());
    }

    Endpoint outputEndpoint;
    {
        DataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.SetType(Data::Type::Number());
        slotConfiguration.SetConnectionType(ConnectionType::Output);

        Slot* outputSlot = outputNode->AddTestingSlot(slotConfiguration);

        outputEndpoint = Endpoint(outputNode->GetEntityId(), outputSlot->GetId());
    }

    const bool validConnection = true;
    TestConnectionBetween(outputEndpoint, inputEndpoint, validConnection);
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_SubClassShouldMatchBaseClassSlot)
{
    // When a slot is configured to use a base class, the slot should accept subclasses of that base class as well.

    using namespace ScriptCanvas;

    CreateGraph();

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();

    for (const ConnectionType& connectionType : { ConnectionType::Input, ConnectionType::Output })
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(connectionType);

        // Set the slot to the base class type
        dataSlotConfiguration.SetType(m_baseClassType);

        Slot* slot = emptyNode->AddTestingSlot(dataSlotConfiguration);

        // When a slot is set to a base class type, it should be able to be hooked up to
        // either a base class type or a subclass type.
        EXPECT_TRUE(slot->IsTypeMatchFor(m_baseClassType));
        EXPECT_TRUE(slot->IsTypeMatchFor(m_subClassType));
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_BaseClassShouldNotMatchSubClassSlot)
{
    // When a slot is configured to use a subclass, the slot should accept the subclass but not the base class.

    using namespace ScriptCanvas;

    CreateGraph();

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();

    for (const ConnectionType& connectionType : { ConnectionType::Input, ConnectionType::Output })
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(connectionType);

        // Set the slot to the subclass type
        dataSlotConfiguration.SetType(m_subClassType);

        Slot* slot = emptyNode->AddTestingSlot(dataSlotConfiguration);

        // When a slot is set to a subclass type, it will only connect to the subclass, not to the base class.
        EXPECT_FALSE(slot->IsTypeMatchFor(m_baseClassType));
        EXPECT_TRUE(slot->IsTypeMatchFor(m_subClassType));
    }
}

TEST_F(ScriptCanvasTestFixture, DynamicSlotCreation_SubClassShouldMatchBaseClassDisplayType)
{
    // When a dynamic slot is created with a base class type, it should match both base classes and subclasses.

    using namespace ScriptCanvas;

    CreateGraph();

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();

    for (const ConnectionType& connectionType : { ConnectionType::Input, ConnectionType::Output })
    {
        for (auto dynamicDataType : { DynamicDataType::Any, DynamicDataType::Value })
        {
            DynamicDataSlotConfiguration dataSlotConfiguration;

            dataSlotConfiguration.m_name = GenerateSlotName();
            dataSlotConfiguration.SetConnectionType(connectionType);
            dataSlotConfiguration.m_dynamicDataType = dynamicDataType;

            // Set the dynamic display type to the base class
            dataSlotConfiguration.m_displayType = m_baseClassType;

            Slot* slot = emptyNode->AddTestingSlot(dataSlotConfiguration);

            // Both the base class and the subclass should match.
            EXPECT_TRUE(slot->IsTypeMatchFor(m_baseClassType));
            EXPECT_TRUE(slot->IsTypeMatchFor(m_subClassType));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, DynamicSlotCreation_BaseClassShouldNotMatchSubClassDisplayType)
{
    // When a dynamic slot is created with a subclass type, it should only match the subclass not the base class.

    using namespace ScriptCanvas;

    CreateGraph();

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();

    for (const ConnectionType& connectionType : { ConnectionType::Input, ConnectionType::Output })
    {
        for (auto dynamicDataType : { DynamicDataType::Any, DynamicDataType::Value })
        {
            DynamicDataSlotConfiguration dataSlotConfiguration;

            dataSlotConfiguration.m_name = GenerateSlotName();
            dataSlotConfiguration.SetConnectionType(connectionType);
            dataSlotConfiguration.m_dynamicDataType = dynamicDataType;

            // Set the dynamic display type to the subclass
            dataSlotConfiguration.m_displayType = m_subClassType;

            Slot* slot = emptyNode->AddTestingSlot(dataSlotConfiguration);

            // Only the subclass should match, not the base class.
            EXPECT_FALSE(slot->IsTypeMatchFor(m_baseClassType));
            EXPECT_TRUE(slot->IsTypeMatchFor(m_subClassType));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_BaseClassSlotWithSubClassVariableShouldMatchBaseClass)
{
    // When a slot is configured to use a base class, and it has a variable of a subclass type assigned to it,
    // the slot should still match base classes. This is important for being able to change what is hooked to the slot.

    using namespace ScriptCanvas;

    CreateGraph();

    // Create a slot of type TestBaseClass

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();
    DataSlotConfiguration dataSlotConfiguration;

    dataSlotConfiguration.m_name = GenerateSlotName();
    dataSlotConfiguration.SetConnectionType(ConnectionType::Input);
    dataSlotConfiguration.SetType(m_baseClassType);
    Slot* slot = emptyNode->AddTestingSlot(dataSlotConfiguration);

    // Create a variable of type TestSubClass.

    ScriptCanvasId scriptCanvasId = m_graph->GetScriptCanvasId();

    TestSubClass testObject;
    auto testSubClassDatum = Datum(testObject);

    constexpr bool FunctionScope = false;
    AZ::Outcome<VariableId, AZStd::string> variableOutcome(AZ::Failure(""));
    GraphVariableManagerRequestBus::EventResult(
        variableOutcome, scriptCanvasId, &GraphVariableManagerRequests::AddVariable, "TestSubClass", testSubClassDatum, FunctionScope);
    EXPECT_TRUE(variableOutcome);
    EXPECT_TRUE(variableOutcome.GetValue().IsValid());

    // Set the slot to a variable of type TestSubClass

    slot->SetVariableReference(variableOutcome.GetValue());

    // The slot's data type should appear to be TestSubClass, matching the currently-assigned variable
    EXPECT_EQ(slot->GetDataType(), m_subClassType);

    // However, the slot should still allow type matches for both TestBaseClass and TestSubClass
    EXPECT_TRUE(slot->IsTypeMatchFor(m_baseClassType));
    EXPECT_TRUE(slot->IsTypeMatchFor(m_subClassType));
}

// Exhaustive Data Connection Test(attempts to connect every data type to every other data type, in both input and output)

// 
// TEST_F(ScriptCanvasTestFixture, SlotConnecting_DataExhaustive)
// {
//     using namespace ScriptCanvas;
// 
//     ScriptCanvas::Graph* graph = CreateGraph();
//     ConfigurableUnitTestNode* inputNode = CreateConfigurableNode();
//     ConfigurableUnitTestNode* outputNode = CreateConfigurableNode();
// 
//     Endpoint dynamicAnyInEndpoint;
//     Endpoint dynamicAnyOutEndpoint;
// 
//     Endpoint dynamicContainerInEndpoint;
//     Endpoint dynamicContainerOutEndpoint;
// 
//     Endpoint dynamicValueInEndpoint;
//     Endpoint dynamicValueOutEndpoint;
// 
//     AZStd::unordered_map< Data::Type, ScriptCanvas::Endpoint > inputTypeMapping;
// 
//     {
//         DynamicDataSlotConfiguration slotConfiguration;
// 
//         slotConfiguration.m_name = GenerateSlotName();
//         slotConfiguration.SetConnectionType(ConnectionType::Input);
//         slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
// 
//         Slot* dynamicAnyInSlot = inputNode->AddTestingSlot(slotConfiguration);
// 
//         dynamicAnyInEndpoint = Endpoint(inputNode->GetEntityId(), dynamicAnyInSlot->GetId());
//     }
// 
//     {
//         DynamicDataSlotConfiguration slotConfiguration;
// 
//         slotConfiguration.m_name = GenerateSlotName();
//         slotConfiguration.SetConnectionType(ConnectionType::Output);
//         slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
// 
//         Slot* dynamicAnyOutSlot = outputNode->AddTestingSlot(slotConfiguration);
// 
//         dynamicAnyOutEndpoint = Endpoint(outputNode->GetEntityId(), dynamicAnyOutSlot->GetId());
//     }
// 
//     {
//         DynamicDataSlotConfiguration slotConfiguration;
// 
//         slotConfiguration.m_name = GenerateSlotName();
//         slotConfiguration.SetConnectionType(ConnectionType::Input);
//         slotConfiguration.m_dynamicDataType = DynamicDataType::Container;
// 
//         Slot* dynamicContainerInSlot = inputNode->AddTestingSlot(slotConfiguration);
// 
//         dynamicContainerInEndpoint = Endpoint(inputNode->GetEntityId(), dynamicContainerInSlot->GetId());
//     }
// 
//     {
//         DynamicDataSlotConfiguration slotConfiguration;
// 
//         slotConfiguration.m_name = GenerateSlotName();
//         slotConfiguration.SetConnectionType(ConnectionType::Output);
//         slotConfiguration.m_dynamicDataType = DynamicDataType::Container;
// 
//         Slot* dynamicContainerOutSlot = outputNode->AddTestingSlot(slotConfiguration);
// 
//         dynamicContainerOutEndpoint = Endpoint(outputNode->GetEntityId(), dynamicContainerOutSlot->GetId());
//     }
// 
//     {
//         DynamicDataSlotConfiguration slotConfiguration;
// 
//         slotConfiguration.m_name = GenerateSlotName();
//         slotConfiguration.SetConnectionType(ConnectionType::Input);
//         slotConfiguration.m_dynamicDataType = DynamicDataType::Value;
// 
//         Slot* dynamicValueInSlot = inputNode->AddTestingSlot(slotConfiguration);
// 
//         dynamicValueInEndpoint = Endpoint(inputNode->GetEntityId(), dynamicValueInSlot->GetId());
//     }
// 
//     {
//         DynamicDataSlotConfiguration slotConfiguration;
// 
//         slotConfiguration.m_name = GenerateSlotName();
//         slotConfiguration.SetConnectionType(ConnectionType::Output);
//         slotConfiguration.m_dynamicDataType = DynamicDataType::Value;
// 
//         Slot* dynamicValueOutSlot = outputNode->AddTestingSlot(slotConfiguration);
// 
//         dynamicValueOutEndpoint = Endpoint(outputNode->GetEntityId(), dynamicValueOutSlot->GetId());
//     }
// 
//     for (auto type : GetTypes())
//     {
//         DataSlotConfiguration slotConfiguration;
// 
//         slotConfiguration.m_name = GenerateSlotName();
//         slotConfiguration.SetType(type);
//         slotConfiguration.SetConnectionType(ConnectionType::Input);
// 
//         Slot* newSlot = inputNode->AddTestingSlot(slotConfiguration);
// 
//         Endpoint inputEndpoint = Endpoint(inputNode->GetEntityId(), newSlot->GetId());
//         inputTypeMapping[type] = inputEndpoint;
// 
//         const bool validConnection = true;
//         TestIsConnectionPossible(dynamicAnyOutEndpoint, inputEndpoint, validConnection);
// 
//         bool isContainerType = Data::IsContainerType(type);
// 
//         TestIsConnectionPossible(dynamicContainerOutEndpoint, inputEndpoint, isContainerType);
//         TestIsConnectionPossible(dynamicValueOutEndpoint, inputEndpoint, !isContainerType);
//     }
// 
//     for (auto type : GetTypes())
//     {
//         DataSlotConfiguration slotConfiguration;
// 
//         slotConfiguration.m_name = GenerateSlotName();
//         slotConfiguration.SetType(type);
//         slotConfiguration.SetConnectionType(ConnectionType::Output);
// 
//         Slot* outputSlot = outputNode->AddTestingSlot(slotConfiguration);
// 
//         ScriptCanvas::Endpoint outputEndpoint(outputNode->GetEntityId(), outputSlot->GetId());
// 
//         const bool validConnection = true;
//         TestIsConnectionPossible(outputEndpoint, dynamicAnyInEndpoint, validConnection);
// 
//         bool isContainerType = Data::IsContainerType(type);
// 
//         TestIsConnectionPossible(outputEndpoint, dynamicContainerInEndpoint, isContainerType);
//         TestIsConnectionPossible(outputEndpoint, dynamicValueInEndpoint, !isContainerType);
// 
//         for (auto slotPair : inputTypeMapping)
//         {
//             bool isSameType = slotPair.first == type;
// 
//             TestIsConnectionPossible(outputEndpoint, slotPair.second, isSameType);
//         }        
//     }
// }


/*
TEST_F(ScriptCanvasTestFixture, TypeMatching_NumericType)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();

    for (const ConnectionType& connectionType : { ConnectionType::Input, ConnectionType::Output })
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();

        dataSlotConfiguration.SetConnectionType(connectionType);
        dataSlotConfiguration.SetType(ScriptCanvas::Data::Type::Number());

        Slot* slot = emptyNode->AddTestingSlot(dataSlotConfiguration);

        EXPECT_TRUE(slot->IsTypeMatchFor(ScriptCanvas::Data::Type::Number()));

        for (auto type : GetTypes())
        {
            if (type == ScriptCanvas::Data::Type::Number())
            {
                continue;
            }

            EXPECT_FALSE(slot->IsTypeMatchFor(type));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_RandomizedFixedType)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();
    ScriptCanvas::Data::Type randomType = GetRandomPrimitiveType();

    for (const ConnectionType& connectionType : { ConnectionType::Input, ConnectionType::Output })
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(connectionType);
        dataSlotConfiguration.SetType(randomType);

        Slot* slot = emptyNode->AddTestingSlot(dataSlotConfiguration);

        EXPECT_TRUE(slot->IsTypeMatchFor(randomType));

        for (auto type : GetTypes())
        {
            if (type == randomType)
            {
                continue;
            }

            EXPECT_FALSE(slot->IsTypeMatchFor(type));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_FixedBehaviorObject)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();

    for (const ConnectionType& connectionType : { ConnectionType::Input, ConnectionType::Output })
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(connectionType);
        dataSlotConfiguration.SetType(m_dataSlotConfigurationType);

        Slot* slot = emptyNode->AddTestingSlot(dataSlotConfiguration);

        EXPECT_TRUE(slot->IsTypeMatchFor(m_dataSlotConfigurationType));

        for (auto type : GetTypes())
        {
            if (type == m_dataSlotConfigurationType)
            {
                continue;
            }

            EXPECT_FALSE(slot->IsTypeMatchFor(type));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_RandomizedFixedBehaviorObject)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();
    ScriptCanvas::Data::Type randomType = GetRandomObjectType();

    for (const ConnectionType& connectionType : { ConnectionType::Input, ConnectionType::Output })
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(connectionType);
        dataSlotConfiguration.SetType(randomType);

        Slot* slot = emptyNode->AddTestingSlot(dataSlotConfiguration);

        EXPECT_TRUE(slot->IsTypeMatchFor(randomType));

        for (auto type : GetTypes())
        {
            if (type == randomType)
            {
                continue;
            }

            EXPECT_FALSE(slot->IsTypeMatchFor(type));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_FixedContainer)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();

    for (const ConnectionType& connectionType : { ConnectionType::Input, ConnectionType::Output })
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(connectionType);
        dataSlotConfiguration.SetType(m_stringToNumberMapType);

        Slot* slot = emptyNode->AddTestingSlot(dataSlotConfiguration);

        EXPECT_TRUE(slot->IsTypeMatchFor(m_stringToNumberMapType));

        for (auto type : GetTypes())
        {
            if (type == m_stringToNumberMapType)
            {
                continue;
            }

            EXPECT_FALSE(slot->IsTypeMatchFor(type));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_RandomizedFixedContainer)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();
    ScriptCanvas::Data::Type randomType = GetRandomContainerType();

    for (const ConnectionType& connectionType : { ConnectionType::Input, ConnectionType::Output })
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(connectionType);
        dataSlotConfiguration.SetType(randomType);

        Slot* slot = emptyNode->AddTestingSlot(dataSlotConfiguration);

        EXPECT_TRUE(slot->IsTypeMatchFor(randomType));

        for (auto type : GetTypes())
        {
            if (type == randomType)
            {
                continue;
            }

            EXPECT_FALSE(slot->IsTypeMatchFor(type));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, DynamicSlotCreation_NoDisplayType)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();

    for (const ConnectionType& connectionType : { ConnectionType::Input, ConnectionType::Output })
    {
        for (auto dynamicDataType : { DynamicDataType::Any, DynamicDataType::Value, DynamicDataType::Container })
        {
            DynamicDataSlotConfiguration dataSlotConfiguration;

            dataSlotConfiguration.m_name = GenerateSlotName();            
            dataSlotConfiguration.SetConnectionType(connectionType);            
            dataSlotConfiguration.m_dynamicDataType = dynamicDataType;

            Slot* slot = emptyNode->AddTestingSlot(dataSlotConfiguration);
            EXPECT_FALSE(slot->HasDisplayType());
            EXPECT_TRUE(slot->IsDynamicSlot());
            EXPECT_TRUE(slot->GetDynamicDataType() == dynamicDataType);
        }
    }
}

TEST_F(ScriptCanvasTestFixture, DynamicSlotCreation_WithDisplayType)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();

    for (const ConnectionType& connectionType : { ConnectionType::Input, ConnectionType::Output })
    {
        for (auto dynamicDataType : { DynamicDataType::Any, DynamicDataType::Value, DynamicDataType::Container })
        {
            DynamicDataSlotConfiguration dataSlotConfiguration;

            dataSlotConfiguration.m_name = GenerateSlotName();
            dataSlotConfiguration.SetConnectionType(connectionType);
            dataSlotConfiguration.m_dynamicDataType = dynamicDataType;

            ScriptCanvas::Data::Type dataType = ScriptCanvas::Data::Type::Invalid();

            if (dynamicDataType == DynamicDataType::Any)
            {
                dataType = GetRandomType();
            }
            else if (dynamicDataType == DynamicDataType::Value)
            {
                if (rand() % 2 == 0)
                {
                    dataType = GetRandomPrimitiveType();
                }
                else
                {
                    dataType = GetRandomObjectType();
                }
            }
            else if (dynamicDataType == DynamicDataType::Container)
            {
                dataType = GetRandomContainerType();
            }

            dataSlotConfiguration.m_displayType = dataType;

            Slot* slot = emptyNode->AddTestingSlot(dataSlotConfiguration);
            
            EXPECT_TRUE(slot->HasDisplayType());
            EXPECT_TRUE(slot->IsDynamicSlot());
            EXPECT_TRUE(slot->GetDynamicDataType() == dynamicDataType);
            EXPECT_TRUE(slot->GetDataType() == dataType);
        }
    }
}

TEST_F(ScriptCanvasTestFixture, DynamicTypingDisplayType_Any)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();

    for (const ConnectionType& connectionType : { ConnectionType::Input, ConnectionType::Output })
    {
        DynamicDataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(connectionType);
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Any;

        Slot* slot = emptyNode->AddTestingSlot(dataSlotConfiguration);
        EXPECT_FALSE(slot->HasDisplayType());

        for (auto type : GetTypes())
        {
            slot->SetDisplayType(type);
            EXPECT_TRUE(slot->HasDisplayType());
            EXPECT_EQ(slot->GetDisplayType(), type);

            slot->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
            EXPECT_FALSE(slot->HasDisplayType());
        }
    }
}

TEST_F(ScriptCanvasTestFixture, DynamicTypingDisplayType_Value)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();

    for (const ConnectionType& connectionType : { ConnectionType::Input, ConnectionType::Output })
    {
        DynamicDataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(connectionType);
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Value;

        Slot* slot = emptyNode->AddTestingSlot(dataSlotConfiguration);

        EXPECT_FALSE(slot->HasDisplayType());

        for (auto primitiveType : GetPrimitiveTypes())
        {
            slot->SetDisplayType(primitiveType);
            EXPECT_TRUE(slot->HasDisplayType());
            EXPECT_EQ(slot->GetDisplayType(), primitiveType);

            slot->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
            EXPECT_FALSE(slot->HasDisplayType());
        }

        for (auto containerType : GetContainerDataTypes())
        {
            slot->SetDisplayType(containerType);
            EXPECT_FALSE(slot->HasDisplayType());
        }

        for (auto objectType : GetBehaviorObjectTypes())
        {
            slot->SetDisplayType(objectType);
            EXPECT_TRUE(slot->HasDisplayType());
            EXPECT_EQ(slot->GetDisplayType(), objectType);

            slot->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
            EXPECT_FALSE(slot->HasDisplayType());
        }
    }
}

TEST_F(ScriptCanvasTestFixture, DynamicTypingDisplayType_Container)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();

    for (const ConnectionType& connectionType : { ConnectionType::Input, ConnectionType::Output })
    {
        DynamicDataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(connectionType);
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Container;

        Slot* slot = emptyNode->AddTestingSlot(dataSlotConfiguration);

        EXPECT_FALSE(slot->HasDisplayType());

        for (auto primitiveType : GetPrimitiveTypes())
        {
            slot->SetDisplayType(primitiveType);
            EXPECT_FALSE(slot->HasDisplayType());
        }

        for (auto containerType : GetContainerDataTypes())
        {
            slot->SetDisplayType(containerType);
            EXPECT_TRUE(slot->HasDisplayType());
            EXPECT_EQ(slot->GetDisplayType(), containerType);

            slot->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
            EXPECT_FALSE(slot->HasDisplayType());
        }

        for (auto objectType : GetBehaviorObjectTypes())
        {
            slot->SetDisplayType(objectType);
            EXPECT_FALSE(slot->HasDisplayType());
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_Any)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();

    for (const ConnectionType& connectionType : { ConnectionType::Input, ConnectionType::Output })
    {
        DynamicDataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(connectionType);
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Any;

        Slot* slot = emptyNode->AddTestingSlot(dataSlotConfiguration);

        for (auto type : GetTypes())
        {
            EXPECT_TRUE(slot->IsTypeMatchFor(type));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_AnyWithDisplayType)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();

    for (const ConnectionType& connectionType : { ConnectionType::Input, ConnectionType::Output })
    {
        DynamicDataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(connectionType);
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Any;

        Slot* slot = emptyNode->AddTestingSlot(dataSlotConfiguration);

        slot->SetDisplayType(ScriptCanvas::Data::Type::Number());
        
        EXPECT_TRUE(slot->IsTypeMatchFor(ScriptCanvas::Data::Type::Number()));

        for (auto type : GetTypes())
        {
            if (type == ScriptCanvas::Data::Type::Number())
            {
                continue;
            }

            EXPECT_FALSE(slot->IsTypeMatchFor(type));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_AnyWithRandomizedDisplayType)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();
    ScriptCanvas::Data::Type randomType = GetRandomType();

    for (const ConnectionType& connectionType : { ConnectionType::Input, ConnectionType::Output })
    {
        DynamicDataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(connectionType);
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Any;

        Slot* slot = emptyNode->AddTestingSlot(dataSlotConfiguration);

        slot->SetDisplayType(randomType);

        EXPECT_TRUE(slot->IsTypeMatchFor(randomType));

        for (auto type : GetTypes())
        {
            if (type == randomType)
            {
                continue;
            }

            EXPECT_FALSE(slot->IsTypeMatchFor(type));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_DynamicValue)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();

    for (const ConnectionType& connectionType : { ConnectionType::Input, ConnectionType::Output })
    {
        DynamicDataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(connectionType);
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Value;

        Slot* slot = emptyNode->AddTestingSlot(dataSlotConfiguration);

        for (auto primitiveType : GetPrimitiveTypes())
        {
            EXPECT_TRUE(slot->IsTypeMatchFor(primitiveType));
        }

        for (auto containerType : GetContainerDataTypes())
        {
            EXPECT_FALSE(slot->IsTypeMatchFor(containerType));
        }

        for (auto objectType : GetBehaviorObjectTypes())
        {
            EXPECT_TRUE(slot->IsTypeMatchFor(objectType));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_DynamicValueWithDisplayType)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();

    for (const ConnectionType& connectionType : { ConnectionType::Input, ConnectionType::Output })
    {
        DynamicDataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(connectionType);;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Value;

        Slot* slot = emptyNode->AddTestingSlot(dataSlotConfiguration);

        slot->SetDisplayType(ScriptCanvas::Data::Type::EntityID());

        EXPECT_TRUE(slot->IsTypeMatchFor(ScriptCanvas::Data::Type::EntityID()));
        
        for (auto primitiveType : GetPrimitiveTypes())
        {
            if (primitiveType == ScriptCanvas::Data::Type::EntityID())
            {
                continue;
            }

            EXPECT_FALSE(slot->IsTypeMatchFor(primitiveType));
        }

        for (auto containerType : GetContainerDataTypes())
        {
            EXPECT_FALSE(slot->IsTypeMatchFor(containerType));
        }

        for (auto objectType : GetBehaviorObjectTypes())
        {
            EXPECT_FALSE(slot->IsTypeMatchFor(objectType));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_DynamicContainer)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();

    for (const ConnectionType& connectionType : { ConnectionType::Input, ConnectionType::Output })
    {
        DynamicDataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(connectionType);
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Container;

        Slot* slot = emptyNode->AddTestingSlot(dataSlotConfiguration);

        for (auto primitiveType : GetPrimitiveTypes())
        {
            EXPECT_FALSE(slot->IsTypeMatchFor(primitiveType));
        }

        for (auto containerType : GetContainerDataTypes())
        {
            EXPECT_TRUE(slot->IsTypeMatchFor(containerType));
        }

        for (auto objectType : GetBehaviorObjectTypes())
        {
            EXPECT_FALSE(slot->IsTypeMatchFor(objectType));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, TypeMatching_DynamicContainerWithDisplayType)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* emptyNode = CreateConfigurableNode();

    for (const ConnectionType& connectionType : { ConnectionType::Input, ConnectionType::Output })
    {
        DynamicDataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(connectionType);
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Container;

        Slot* slot = emptyNode->AddTestingSlot(dataSlotConfiguration);

        slot->SetDisplayType(m_stringToNumberMapType);

        for (auto primitiveType : GetPrimitiveTypes())
        {
            EXPECT_FALSE(slot->IsTypeMatchFor(primitiveType));
        }

        EXPECT_TRUE(slot->IsTypeMatchFor(m_stringToNumberMapType));

        for (auto containerType : GetContainerDataTypes())
        {
            if (containerType == m_stringToNumberMapType)
            {
                continue;
            }

            EXPECT_FALSE(slot->IsTypeMatchFor(containerType));
        }

        for (auto objectType : GetBehaviorObjectTypes())
        {
            EXPECT_FALSE(slot->IsTypeMatchFor(objectType));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, SlotMatching_FixedPrimitiveSlotToFixedPrimitiveSlot)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* sourceNode = CreateConfigurableNode();
    Slot* sourceSlot = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Input);
        dataSlotConfiguration.SetType(ScriptCanvas::Data::Type::Number());

        sourceSlot = sourceNode->AddTestingSlot(dataSlotConfiguration);
    }

    ConfigurableUnitTestNode* targetNode = CreateConfigurableNode();
    Slot* validTargetSlot = nullptr;
    Slot* invalidTargetSlot = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Output);
        dataSlotConfiguration.SetType(ScriptCanvas::Data::Type::Number());

        validTargetSlot = targetNode->AddTestingSlot(dataSlotConfiguration);
    }

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Output);
        dataSlotConfiguration.SetType(ScriptCanvas::Data::Type::Boolean());

        invalidTargetSlot = targetNode->AddTestingSlot(dataSlotConfiguration);
    }

    EXPECT_TRUE(sourceSlot->IsTypeMatchFor((*validTargetSlot)));
    EXPECT_TRUE(validTargetSlot->IsTypeMatchFor((*sourceSlot)));

    EXPECT_FALSE(sourceSlot->IsTypeMatchFor((*invalidTargetSlot)));
    EXPECT_FALSE(invalidTargetSlot->IsTypeMatchFor((*sourceSlot)));
}

TEST_F(ScriptCanvasTestFixture, SlotMatching_FixedObjectSlotToFixedObjectSlot)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* sourceNode = CreateConfigurableNode();
    Slot* sourceSlot = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Input);
        dataSlotConfiguration.SetType(m_dataSlotConfigurationType);

        sourceSlot = sourceNode->AddTestingSlot(dataSlotConfiguration);
    }

    ConfigurableUnitTestNode* targetNode = CreateConfigurableNode();
    Slot* validTargetSlot = nullptr;
    Slot* invalidTargetSlot = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Output);
        dataSlotConfiguration.SetType(m_dataSlotConfigurationType);

        validTargetSlot = targetNode->AddTestingSlot(dataSlotConfiguration);
    }

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Output);
        dataSlotConfiguration.SetType(ScriptCanvas::Data::Type::Boolean());

        invalidTargetSlot = targetNode->AddTestingSlot(dataSlotConfiguration);
    }

    EXPECT_TRUE(sourceSlot->IsTypeMatchFor((*validTargetSlot)));
    EXPECT_TRUE(validTargetSlot->IsTypeMatchFor((*sourceSlot)));

    EXPECT_FALSE(sourceSlot->IsTypeMatchFor((*invalidTargetSlot)));
    EXPECT_FALSE(invalidTargetSlot->IsTypeMatchFor((*sourceSlot)));
}

TEST_F(ScriptCanvasTestFixture, SlotMatching_FixedContainerSlotToFixedContainerSlot)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* sourceNode = CreateConfigurableNode();
    Slot* sourceSlot = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Input);
        dataSlotConfiguration.SetType(m_stringToNumberMapType);

        sourceSlot = sourceNode->AddTestingSlot(dataSlotConfiguration);
    }

    ConfigurableUnitTestNode* targetNode = CreateConfigurableNode();
    Slot* validTargetSlot = nullptr;
    Slot* invalidTargetSlot = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Output);
        dataSlotConfiguration.SetType(m_stringToNumberMapType);

        validTargetSlot = targetNode->AddTestingSlot(dataSlotConfiguration);
    }

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Output);
        dataSlotConfiguration.SetType(m_numericVectorType);

        invalidTargetSlot = targetNode->AddTestingSlot(dataSlotConfiguration);
    }

    EXPECT_TRUE(sourceSlot->IsTypeMatchFor((*validTargetSlot)));
    EXPECT_TRUE(validTargetSlot->IsTypeMatchFor((*sourceSlot)));

    EXPECT_FALSE(sourceSlot->IsTypeMatchFor((*invalidTargetSlot)));
    EXPECT_FALSE(invalidTargetSlot->IsTypeMatchFor((*sourceSlot)));
}

TEST_F(ScriptCanvasTestFixture, SlotMatching_FixedPrimitiveSlotToDynamicAnySlot)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* sourceNode = CreateConfigurableNode();
    Slot* sourceSlot = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Input);
        dataSlotConfiguration.SetType(ScriptCanvas::Data::Type::Number());

        sourceSlot = sourceNode->AddTestingSlot(dataSlotConfiguration);
    }

    ConfigurableUnitTestNode* targetNode = CreateConfigurableNode();
    Slot* dynamicTarget = nullptr;    

    {
        DynamicDataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Output);
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Any;

        dynamicTarget = targetNode->AddTestingSlot(dataSlotConfiguration);
    }

    EXPECT_TRUE(sourceSlot->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*sourceSlot)));

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Number());
    EXPECT_TRUE(sourceSlot->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*sourceSlot)));

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    EXPECT_TRUE(sourceSlot->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*sourceSlot)));

    for (auto type : GetTypes())
    {
        if (type == ScriptCanvas::Data::Type::Number())
        {
            continue;
        }

        dynamicTarget->SetDisplayType(type);
        EXPECT_FALSE(sourceSlot->IsTypeMatchFor((*dynamicTarget)));
        EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*sourceSlot)));

        dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    }
}

TEST_F(ScriptCanvasTestFixture, SlotMatching_FixedPrimitiveSlotToDynamicValueSlot)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* sourceNode = CreateConfigurableNode();
    Slot* sourceSlot = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Input);
        dataSlotConfiguration.SetType(ScriptCanvas::Data::Type::Number());

        sourceSlot = sourceNode->AddTestingSlot(dataSlotConfiguration);
    }

    ConfigurableUnitTestNode* targetNode = CreateConfigurableNode();
    Slot* dynamicTarget = nullptr;

    {
        DynamicDataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Output);
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Value;

        dynamicTarget = targetNode->AddTestingSlot(dataSlotConfiguration);
    }

    EXPECT_TRUE(sourceSlot->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*sourceSlot)));

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Number());
    EXPECT_TRUE(sourceSlot->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*sourceSlot)));

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    EXPECT_TRUE(sourceSlot->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*sourceSlot)));

    for (auto primitiveType : GetPrimitiveTypes())
    {
        if (primitiveType == ScriptCanvas::Data::Type::Number())
        {
            continue;
        }

        dynamicTarget->SetDisplayType(primitiveType);
        EXPECT_FALSE(sourceSlot->IsTypeMatchFor((*dynamicTarget)));
        EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*sourceSlot)));

        dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    }

    for (auto objectType : GetBehaviorObjectTypes())
    {
        dynamicTarget->SetDisplayType(objectType);
        EXPECT_FALSE(sourceSlot->IsTypeMatchFor((*dynamicTarget)));
        EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*sourceSlot)));

        dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    }
}

TEST_F(ScriptCanvasTestFixture, SlotMatching_FixedPrimitiveSlotToDynamicContainerSlot)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* sourceNode = CreateConfigurableNode();
    Slot* sourceSlot = nullptr;

    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Input);
        dataSlotConfiguration.SetType(ScriptCanvas::Data::Type::Number());

        sourceSlot = sourceNode->AddTestingSlot(dataSlotConfiguration);
    }

    ConfigurableUnitTestNode* targetNode = CreateConfigurableNode();
    Slot* dynamicTarget = nullptr;

    {
        DynamicDataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Output);
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Container;

        dynamicTarget = targetNode->AddTestingSlot(dataSlotConfiguration);
    }

    EXPECT_FALSE(sourceSlot->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*sourceSlot)));

    for (auto containerType : GetContainerDataTypes())
    {
        dynamicTarget->SetDisplayType(containerType);
        EXPECT_FALSE(sourceSlot->IsTypeMatchFor((*dynamicTarget)));
        EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*sourceSlot)));

        dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    }
}

TEST_F(ScriptCanvasTestFixture, SlotMatching_DynamicAnySlotToDynamicValueSlot)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* sourceNode = CreateConfigurableNode();
    Slot* dynamicSource = nullptr;

    {
        DynamicDataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Input);
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Any;

        dynamicSource = sourceNode->AddTestingSlot(dataSlotConfiguration);
    }

    ConfigurableUnitTestNode* targetNode = CreateConfigurableNode();
    Slot* dynamicTarget = nullptr;

    {
        DynamicDataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Output);
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Value;

        dynamicTarget = targetNode->AddTestingSlot(dataSlotConfiguration);
    }

    // Any : Value
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Any : Value[Number]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Number());
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));
    
    // Any[Number] : Value
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Number());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Any[Number] : Value[Number]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Number());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Number());
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Any[Boolean] : Value[Number]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Boolean());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Number());
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Any[Display Container] : Dynamic Value
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(m_numericVectorType);

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));
    
    // Any[Display Container] : Value[Display Object]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(m_numericVectorType);

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(m_dataSlotConfigurationType);
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));
}

TEST_F(ScriptCanvasTestFixture, SlotMatching_DynamicAnySlotToDynamicContainerSlot)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* sourceNode = CreateConfigurableNode();
    Slot* dynamicSource = nullptr;

    {
        DynamicDataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Input);
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Any;

        dynamicSource = sourceNode->AddTestingSlot(dataSlotConfiguration);
    }

    ConfigurableUnitTestNode* targetNode = CreateConfigurableNode();
    Slot* dynamicTarget = nullptr;

    {
        DynamicDataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Output);
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Container;

        dynamicTarget = targetNode->AddTestingSlot(dataSlotConfiguration);
    }

    // Any : Container
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Any : Container[Vector<Number>]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(m_numericVectorType);
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Any[Number] : Container
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Number());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Any[Vector<Number>] : Container
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(m_numericVectorType);

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Any[Vector<Number>] : Container[Vector<Number>]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(m_numericVectorType);

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(m_numericVectorType);
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Any[Map<String, Number>] : Container[Vector<Number>]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(m_stringToNumberMapType);

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(m_numericVectorType);
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Any[Object] : Container[Map<String,Number>]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(m_dataSlotConfigurationType);

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(m_stringToNumberMapType);
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));
}

TEST_F(ScriptCanvasTestFixture, SlotMatching_DynamicValueSlotToDynamicValueSlot)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* sourceNode = CreateConfigurableNode();
    Slot* dynamicSource = nullptr;

    {
        DynamicDataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Input);
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Value;

        dynamicSource = sourceNode->AddTestingSlot(dataSlotConfiguration);
    }

    ConfigurableUnitTestNode* targetNode = CreateConfigurableNode();
    Slot* dynamicTarget = nullptr;

    {
        DynamicDataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Output);
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Value;

        dynamicTarget = targetNode->AddTestingSlot(dataSlotConfiguration);
    }

    // Value : Value
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Value[Number] : Value
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Number());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Value : Value[Number]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());    

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Number());
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Value[Number] : Value[Number]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Number());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Number());
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Value[Object] : Value[Object]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(m_dataSlotConfigurationType);

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(m_dataSlotConfigurationType);
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Value[Number] : Value[Boolean]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Number());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Boolean());
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Value[Object] : Value[Boolean]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(m_dataSlotConfigurationType);

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Boolean());
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));
}

TEST_F(ScriptCanvasTestFixture, SlotMatching_DynamicValueSlotToDynamicContainerSlot)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* sourceNode = CreateConfigurableNode();
    Slot* dynamicSource = nullptr;

    {
        DynamicDataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Input);
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Value;

        dynamicSource = sourceNode->AddTestingSlot(dataSlotConfiguration);
    }

    ConfigurableUnitTestNode* targetNode = CreateConfigurableNode();
    Slot* dynamicTarget = nullptr;

    {
        DynamicDataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Output);
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Container;

        dynamicTarget = targetNode->AddTestingSlot(dataSlotConfiguration);
    }

    // Value : Container
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Value : Container[Vector<Number>]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(m_numericVectorType);
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Value[Number] : Container
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Number());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Value[Number] : Container[Vector<Number>]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Number());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(m_numericVectorType);
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Value[Object] : Container[Map<String,Number>]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(m_dataSlotConfigurationType);

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(m_stringToNumberMapType);
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));
}

TEST_F(ScriptCanvasTestFixture, SlotMatching_DynamicContainerSlotToDynamicContainerSlot)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* sourceNode = CreateConfigurableNode();
    Slot* dynamicSource = nullptr;

    {
        DynamicDataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Input);
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Container;

        dynamicSource = sourceNode->AddTestingSlot(dataSlotConfiguration);
    }

    ConfigurableUnitTestNode* targetNode = CreateConfigurableNode();
    Slot* dynamicTarget = nullptr;

    {
        DynamicDataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Output);
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Container;

        dynamicTarget = targetNode->AddTestingSlot(dataSlotConfiguration);
    }

    // Container : Container
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Container[Vector<Number>] : Container
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(m_numericVectorType);

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Container : Container[Vector<Number>]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(m_numericVectorType);
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Container[Vector<Number>] : Container[Vector<Number>]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(m_numericVectorType);

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(m_numericVectorType);
    EXPECT_TRUE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_TRUE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));

    // Container[Vector<Number>] : Container[Map<String,Number>]
    dynamicSource->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicSource->SetDisplayType(m_numericVectorType);

    dynamicTarget->SetDisplayType(ScriptCanvas::Data::Type::Invalid());
    dynamicTarget->SetDisplayType(m_stringToNumberMapType);
    EXPECT_FALSE(dynamicSource->IsTypeMatchFor((*dynamicTarget)));
    EXPECT_FALSE(dynamicTarget->IsTypeMatchFor((*dynamicSource)));
}

TEST_F(ScriptCanvasTestFixture, SlotGrouping_BasicFunctionalitySanityTest)
{
    using namespace ScriptCanvas;

    [[maybe_unused]] ScriptCanvas::Graph* graph = CreateGraph();
    ConfigurableUnitTestNode* inputNode = CreateConfigurableNode();

    Slot* dynamicInputSlot = nullptr;
    Slot* dynamicOutputSlot = nullptr;

    AZ::Crc32 dynamicGroupName = AZ::Crc32("Group");

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
        slotConfiguration.m_dynamicGroup = dynamicGroupName;
        slotConfiguration.SetConnectionType(ConnectionType::Input);

        dynamicInputSlot = inputNode->AddTestingSlot(slotConfiguration);
    }

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
        slotConfiguration.m_dynamicGroup = dynamicGroupName;
        slotConfiguration.SetConnectionType(ConnectionType::Output);

        dynamicOutputSlot = inputNode->AddTestingSlot(slotConfiguration);
    }

    EXPECT_EQ(dynamicInputSlot->GetDynamicGroup(), dynamicGroupName);
    EXPECT_EQ(dynamicOutputSlot->GetDynamicGroup(), dynamicGroupName);

    EXPECT_FALSE(dynamicInputSlot->HasDisplayType());    
    EXPECT_EQ(dynamicInputSlot->GetDataType(), Data::Type::Invalid());

    EXPECT_FALSE(dynamicOutputSlot->HasDisplayType());
    EXPECT_EQ(dynamicOutputSlot->GetDataType(), Data::Type::Invalid());

    inputNode->TestSetDisplayType(dynamicGroupName, Data::Type::Number());

    EXPECT_TRUE(dynamicInputSlot->HasDisplayType());
    EXPECT_EQ(dynamicInputSlot->GetDisplayType(), Data::Type::Number());
    EXPECT_EQ(dynamicInputSlot->GetDataType(), Data::Type::Number());

    EXPECT_TRUE(dynamicOutputSlot->HasDisplayType());
    EXPECT_EQ(dynamicOutputSlot->GetDisplayType(), Data::Type::Number());
    EXPECT_EQ(dynamicOutputSlot->GetDataType(), Data::Type::Number());
}

TEST_F(ScriptCanvasTestFixture, SlotGrouping_SingleGroupDisplayTypeConnection)
{
    using namespace ScriptCanvas;

    ScriptCanvas::Graph* graph = CreateGraph();
    ConfigurableUnitTestNode* groupedNode = CreateConfigurableNode();

    Slot* dynamicInputSlot = nullptr;
    Slot* dynamicOutputSlot = nullptr;
    Slot* separateGroupSlot = nullptr;

    AZ::Crc32 dynamicGroupName = AZ::Crc32("Group");

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
        slotConfiguration.m_dynamicGroup = dynamicGroupName;
        slotConfiguration.SetConnectionType(ConnectionType::Input);

        dynamicInputSlot = groupedNode->AddTestingSlot(slotConfiguration);
    }

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
        slotConfiguration.m_dynamicGroup = dynamicGroupName;
        slotConfiguration.SetConnectionType(ConnectionType::Output);

        dynamicOutputSlot = groupedNode->AddTestingSlot(slotConfiguration);
    }

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
        slotConfiguration.m_dynamicGroup = AZ::Crc32("SecondGroup");
        slotConfiguration.SetConnectionType(ConnectionType::Output);

        separateGroupSlot = groupedNode->AddTestingSlot(slotConfiguration);
    }

    Slot* fixedOutputSlot = nullptr;
    Slot* fixedInputSlot = nullptr;

    ConfigurableUnitTestNode* concreteNode = CreateConfigurableNode();

    {
        DataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.SetType(Data::Type::Boolean());
        slotConfiguration.SetConnectionType(ConnectionType::Output);

        fixedOutputSlot = concreteNode->AddTestingSlot(slotConfiguration);
    }

    {
        DataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.SetType(Data::Type::Boolean());
        slotConfiguration.SetConnectionType(ConnectionType::Input);

        fixedInputSlot = concreteNode->AddTestingSlot(slotConfiguration);
    }

    const bool isValidConnection = true;

    TestConnectionBetween(fixedOutputSlot->GetEndpoint(), dynamicInputSlot->GetEndpoint(), isValidConnection);

    EXPECT_FALSE(separateGroupSlot->HasDisplayType());
    EXPECT_EQ(separateGroupSlot->GetDisplayType(), Data::Type::Invalid());

    EXPECT_TRUE(dynamicInputSlot->HasDisplayType());
    EXPECT_EQ(dynamicInputSlot->GetDisplayType(), Data::Type::Boolean());
    EXPECT_EQ(dynamicInputSlot->GetDataType(), Data::Type::Boolean());    

    EXPECT_TRUE(dynamicOutputSlot->HasDisplayType());
    EXPECT_EQ(dynamicOutputSlot->GetDisplayType(), Data::Type::Boolean());
    EXPECT_EQ(dynamicOutputSlot->GetDataType(), Data::Type::Boolean());

    EXPECT_TRUE(groupedNode->TestHasConcreteDisplayType(dynamicGroupName));

    TestIsConnectionPossible(dynamicOutputSlot->GetEndpoint(), fixedInputSlot->GetEndpoint(), isValidConnection);

    AZ::Entity* connection = nullptr;
    if (graph->FindConnection(connection, fixedOutputSlot->GetEndpoint(), dynamicInputSlot->GetEndpoint()))
    {
        graph->RemoveConnection(connection->GetId());
    }

    EXPECT_FALSE(dynamicInputSlot->HasDisplayType());    
    EXPECT_EQ(dynamicInputSlot->GetDataType(), Data::Type::Invalid());

    EXPECT_FALSE(dynamicOutputSlot->HasDisplayType());
    EXPECT_EQ(dynamicOutputSlot->GetDataType(), Data::Type::Invalid());

    EXPECT_FALSE(groupedNode->TestHasConcreteDisplayType(dynamicGroupName));

    TestConnectionBetween(dynamicOutputSlot->GetEndpoint(), fixedInputSlot->GetEndpoint(), isValidConnection);

    EXPECT_FALSE(separateGroupSlot->HasDisplayType());
    EXPECT_EQ(separateGroupSlot->GetDisplayType(), Data::Type::Invalid());

    EXPECT_TRUE(dynamicInputSlot->HasDisplayType());
    EXPECT_EQ(dynamicInputSlot->GetDisplayType(), Data::Type::Boolean());
    EXPECT_EQ(dynamicInputSlot->GetDataType(), Data::Type::Boolean());

    EXPECT_TRUE(dynamicOutputSlot->HasDisplayType());
    EXPECT_EQ(dynamicOutputSlot->GetDisplayType(), Data::Type::Boolean());
    EXPECT_EQ(dynamicOutputSlot->GetDataType(), Data::Type::Boolean());

    EXPECT_TRUE(groupedNode->TestHasConcreteDisplayType(dynamicGroupName));
}

TEST_F(ScriptCanvasTestFixture, SlotGrouping_MultiGroupDisplayTypeConnection)
{
    using namespace ScriptCanvas;
    ScriptCanvas::Graph* graph = CreateGraph();
    ConfigurableUnitTestNode* groupedNode = CreateConfigurableNode();

    Slot* dynamicInputSlot = nullptr;
    Slot* dynamicOutputSlot = nullptr;

    Slot* separateGroupSlot = nullptr;

    AZ::Crc32 dynamicGroupName = AZ::Crc32("Group");
    AZ::Crc32 secondaryGroupName = AZ::Crc32("SecondGroup");

    groupedNode->AddTestingSlot(CommonSlots::GeneralInSlot());
    groupedNode->AddTestingSlot(CommonSlots::GeneralOutSlot());

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
        slotConfiguration.m_dynamicGroup = dynamicGroupName;
        slotConfiguration.SetConnectionType(ConnectionType::Input);

        dynamicInputSlot = groupedNode->AddTestingSlot(slotConfiguration);
    }

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
        slotConfiguration.m_dynamicGroup = dynamicGroupName;
        slotConfiguration.SetConnectionType(ConnectionType::Output);

        dynamicOutputSlot = groupedNode->AddTestingSlot(slotConfiguration);
    }

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
        slotConfiguration.m_dynamicGroup = secondaryGroupName;
        slotConfiguration.SetConnectionType(ConnectionType::Output);

        separateGroupSlot = groupedNode->AddTestingSlot(slotConfiguration);
    }

    Slot* secondaryDynamicInputSlot = nullptr;
    Slot* secondaryDynamicOutputSlot = nullptr;

    ConfigurableUnitTestNode* secondaryGroupedNode = CreateConfigurableNode();

    secondaryGroupedNode->AddTestingSlot(CommonSlots::GeneralInSlot());
    secondaryGroupedNode->AddTestingSlot(CommonSlots::GeneralOutSlot());

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
        slotConfiguration.m_dynamicGroup = secondaryGroupName;
        slotConfiguration.SetConnectionType(ConnectionType::Output);

        secondaryDynamicOutputSlot = secondaryGroupedNode->AddTestingSlot(slotConfiguration);
    }

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
        slotConfiguration.m_dynamicGroup = secondaryGroupName;
        slotConfiguration.SetConnectionType(ConnectionType::Input);

        secondaryDynamicInputSlot = secondaryGroupedNode->AddTestingSlot(slotConfiguration);
    }

    Slot* fixedOutputSlot = nullptr;
    Slot* fixedInputSlot = nullptr;

    ConfigurableUnitTestNode* concreteNode = CreateConfigurableNode();

    {
        DataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.SetType(Data::Type::Boolean());
        slotConfiguration.SetConnectionType(ConnectionType::Output);

        fixedOutputSlot = concreteNode->AddTestingSlot(slotConfiguration);
    }

    {
        DataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.SetType(Data::Type::Boolean());
        slotConfiguration.SetConnectionType(ConnectionType::Input);

        fixedInputSlot = concreteNode->AddTestingSlot(slotConfiguration);
    }

    const bool isValidConnection = true;

    AZStd::vector< Slot* > groupedSlots = { dynamicInputSlot, dynamicOutputSlot, secondaryDynamicInputSlot, secondaryDynamicOutputSlot };

    for (Slot* dynamicTestSlot : { dynamicInputSlot, dynamicOutputSlot })
    {
        Slot* dynamicTargetSlot = nullptr;

        if (dynamicTestSlot->IsInput())
        {
            dynamicTargetSlot = secondaryDynamicOutputSlot;
        }
        else
        {
            dynamicTargetSlot = secondaryDynamicInputSlot;
        }

        TestConnectionBetween(dynamicTestSlot->GetEndpoint(), dynamicTargetSlot->GetEndpoint(), isValidConnection);

        for (Slot* testSlot : groupedSlots)
        {
            Slot* targetSlot = nullptr;

            if (testSlot->IsInput())
            {
                targetSlot = fixedOutputSlot;
            }
            else
            {
                targetSlot = fixedInputSlot;
            }

            TestConnectionBetween(testSlot->GetEndpoint(), targetSlot->GetEndpoint(), isValidConnection);

            EXPECT_FALSE(separateGroupSlot->HasDisplayType());
            EXPECT_EQ(separateGroupSlot->GetDisplayType(), Data::Type::Invalid());

            for (Slot* groupedSlot : groupedSlots)
            {
                EXPECT_TRUE(groupedSlot->HasDisplayType());
                EXPECT_EQ(groupedSlot->GetDisplayType(), Data::Type::Boolean());
                EXPECT_EQ(groupedSlot->GetDataType(), Data::Type::Boolean());
            }

            EXPECT_TRUE(groupedNode->TestHasConcreteDisplayType(dynamicGroupName));
            EXPECT_TRUE(secondaryGroupedNode->TestHasConcreteDisplayType(secondaryGroupName));

            AZ::Entity* connection = nullptr;
            if (graph->FindConnection(connection, targetSlot->GetEndpoint(), testSlot->GetEndpoint()))
            {
                graph->RemoveConnection(connection->GetId());
            }

            for (Slot* groupedSlot : groupedSlots)
            {
                EXPECT_FALSE(groupedSlot->HasDisplayType());                
                EXPECT_EQ(groupedSlot->GetDataType(), Data::Type::Invalid());
            }

            EXPECT_FALSE(groupedNode->TestHasConcreteDisplayType(dynamicGroupName));
            EXPECT_FALSE(secondaryGroupedNode->TestHasConcreteDisplayType(secondaryGroupName));
        }
    }
}

TEST_F(ScriptCanvasTestFixture, SlotGrouping_SingleGroupDisplayTypeRestriction)
{
    using namespace ScriptCanvas;

    [[maybe_unused]] ScriptCanvas::Graph* graph = CreateGraph();
    ConfigurableUnitTestNode* groupedNode = CreateConfigurableNode();

    Slot* restrictedInputSlot = nullptr;
    Slot* unrestictedOutputSlot = nullptr;
    Slot* separateGroupSlot = nullptr;

    AZ::Crc32 dynamicGroupName = AZ::Crc32("Group");

    Data::Type randomType = GetRandomType();

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
        slotConfiguration.m_dynamicGroup = dynamicGroupName;
        slotConfiguration.SetConnectionType(ConnectionType::Input);
        slotConfiguration.m_contractDescs = { { [randomType]() { return aznew RestrictedTypeContract({ randomType }); } } };

        restrictedInputSlot = groupedNode->AddTestingSlot(slotConfiguration);
    }

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
        slotConfiguration.m_dynamicGroup = dynamicGroupName;
        slotConfiguration.SetConnectionType(ConnectionType::Output);

        unrestictedOutputSlot = groupedNode->AddTestingSlot(slotConfiguration);
    }

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
        slotConfiguration.m_dynamicGroup = AZ::Crc32("SecondGroup");
        slotConfiguration.SetConnectionType(ConnectionType::Output);

        separateGroupSlot = groupedNode->AddTestingSlot(slotConfiguration);
    }

    const bool isValidConnection = true;

    auto dataTypes = GetTypes();

    for (const auto& dataType : dataTypes)
    {
        bool isValidRestrictedConnection = (randomType == dataType);

        EXPECT_TRUE(separateGroupSlot->IsTypeMatchFor(dataType));
        EXPECT_TRUE(unrestictedOutputSlot->IsTypeMatchFor(dataType));
        EXPECT_EQ(restrictedInputSlot->IsTypeMatchFor(dataType).IsSuccess(), isValidRestrictedConnection);
        EXPECT_EQ(groupedNode->IsValidTypeForGroup(dynamicGroupName, dataType).IsSuccess(), isValidRestrictedConnection);
    }
}

TEST_F(ScriptCanvasTestFixture, SlotGrouping_SingleGroupDisplayTypeRestrictionConnection)
{
    using namespace ScriptCanvas;

    [[maybe_unused]] ScriptCanvas::Graph* graph = CreateGraph();
    ConfigurableUnitTestNode* groupedNode = CreateConfigurableNode();

    Slot* restrictedInputSlot = nullptr;
    Slot* unrestictedOutputSlot = nullptr;
    Slot* separateGroupInputSlot = nullptr;
    Slot* separateGroupOutputSlot = nullptr;

    AZ::Crc32 dynamicGroupName = AZ::Crc32("Group");

    Data::Type randomType = GetRandomType();

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
        slotConfiguration.m_dynamicGroup = dynamicGroupName;
        slotConfiguration.SetConnectionType(ConnectionType::Input);
        slotConfiguration.m_contractDescs = { { [randomType]() { return aznew RestrictedTypeContract({ randomType }); } } };

        restrictedInputSlot = groupedNode->AddTestingSlot(slotConfiguration);
    }

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
        slotConfiguration.m_dynamicGroup = dynamicGroupName;
        slotConfiguration.SetConnectionType(ConnectionType::Output);

        unrestictedOutputSlot = groupedNode->AddTestingSlot(slotConfiguration);
    }

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
        slotConfiguration.m_dynamicGroup = AZ::Crc32("SecondGroup");
        slotConfiguration.SetConnectionType(ConnectionType::Output);

        separateGroupOutputSlot = groupedNode->AddTestingSlot(slotConfiguration);
    }

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
        slotConfiguration.m_dynamicGroup = AZ::Crc32("SecondGroup");
        slotConfiguration.SetConnectionType(ConnectionType::Input);

        separateGroupInputSlot = groupedNode->AddTestingSlot(slotConfiguration);
    }

    const bool isValidConnection = true;

    auto dataTypes = GetTypes();

    for (const auto& dataType : dataTypes)
    {
        Slot* fixedOutputSlot = nullptr;
        Slot* fixedInputSlot = nullptr;

        ConfigurableUnitTestNode* concreteNode = CreateConfigurableNode();

        {
            DataSlotConfiguration slotConfiguration;

            slotConfiguration.m_name = GenerateSlotName();
            slotConfiguration.SetType(dataType);
            slotConfiguration.SetConnectionType(ConnectionType::Output);

            fixedOutputSlot = concreteNode->AddTestingSlot(slotConfiguration);
        }

        {
            DataSlotConfiguration slotConfiguration;

            slotConfiguration.m_name = GenerateSlotName();
            slotConfiguration.SetType(dataType);
            slotConfiguration.SetConnectionType(ConnectionType::Input);

            fixedInputSlot = concreteNode->AddTestingSlot(slotConfiguration);
        }

        bool isValidRestrictedConnection = (randomType == dataType);

        TestIsConnectionPossible(fixedOutputSlot->GetEndpoint(), separateGroupInputSlot->GetEndpoint(), isValidConnection);
        TestIsConnectionPossible(fixedInputSlot->GetEndpoint(), separateGroupOutputSlot->GetEndpoint(), isValidConnection);
        TestIsConnectionPossible(fixedOutputSlot->GetEndpoint(), restrictedInputSlot->GetEndpoint(), isValidRestrictedConnection);
        TestIsConnectionPossible(fixedInputSlot->GetEndpoint(), unrestictedOutputSlot->GetEndpoint(), isValidRestrictedConnection);
    }
}

TEST_F(ScriptCanvasTestFixture, SlotGrouping_MultiGroupDisplayTypeRestrictionConnection)
{
    using namespace ScriptCanvas;

    AZ::Crc32 firstGroupName = AZ::Crc32("FirstGroupName");
    AZ::Crc32 secondGroupName = AZ::Crc32("SecondGroupName");

    ScriptCanvas::Graph* graph = CreateGraph();

    ConfigurableUnitTestNode* groupedUnrestrictedNode = CreateConfigurableNode();

    Slot* groupedUnrestrictedInputSlot = nullptr;
    Slot* groupedUnrestrictedOutputSlot = nullptr;

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.SetConnectionType(ConnectionType::Input);
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
        slotConfiguration.m_dynamicGroup = firstGroupName;

        groupedUnrestrictedInputSlot = groupedUnrestrictedNode->AddTestingSlot(slotConfiguration);
    }

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.SetConnectionType(ConnectionType::Output);
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
        slotConfiguration.m_dynamicGroup = firstGroupName;

        groupedUnrestrictedOutputSlot = groupedUnrestrictedNode->AddTestingSlot(slotConfiguration);
    }

    ConfigurableUnitTestNode* groupedRestrictedNode = CreateConfigurableNode();

    Slot* groupedRestrictedInputSlot = nullptr;
    Slot* groupedRestrictedOutputSlot = nullptr;

    Data::Type randomType = GetRandomType();

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.SetConnectionType(ConnectionType::Input);
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
        slotConfiguration.m_dynamicGroup = firstGroupName;
        slotConfiguration.m_contractDescs = { { [randomType]() { return aznew RestrictedTypeContract({ randomType }); } } };

        groupedRestrictedInputSlot = groupedRestrictedNode->AddTestingSlot(slotConfiguration);
    }

    {
        DynamicDataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.SetConnectionType(ConnectionType::Output);
        slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
        slotConfiguration.m_dynamicGroup = firstGroupName;
        slotConfiguration.m_contractDescs = { { [randomType]() { return aznew RestrictedTypeContract({ randomType }); } } };

        groupedRestrictedOutputSlot = groupedRestrictedNode->AddTestingSlot(slotConfiguration);
    }

    AZStd::unordered_map<Data::Type, Slot* > inputTypeMapping;
    AZStd::unordered_map<Data::Type, Slot* > outputTypeMapping;

    ConfigurableUnitTestNode* concreteTypeNode = CreateConfigurableNode();

    for (auto type : GetTypes())
    {
        {
            DataSlotConfiguration slotConfiguration;

            slotConfiguration.m_name = GenerateSlotName();
            slotConfiguration.SetConnectionType(ConnectionType::Output);
            slotConfiguration.SetType(type);

            outputTypeMapping[type] = concreteTypeNode->AddTestingSlot(slotConfiguration);
        }

        {
            DataSlotConfiguration slotConfiguration;

            slotConfiguration.m_name = GenerateSlotName();
            slotConfiguration.SetConnectionType(ConnectionType::Input);
            slotConfiguration.SetType(type);

            inputTypeMapping[type] = concreteTypeNode->AddTestingSlot(slotConfiguration);
        }
    }

    CreateExecutionFlowBetween({concreteTypeNode, groupedUnrestrictedNode, groupedRestrictedNode });    

    for (Slot* unrestrictedSlot : {groupedUnrestrictedInputSlot, groupedUnrestrictedOutputSlot})
    {
        Slot* restrictedSlot = nullptr;

        if (unrestrictedSlot == groupedUnrestrictedInputSlot)
        {
            restrictedSlot = groupedRestrictedOutputSlot;
        }
        else if (unrestrictedSlot == groupedRestrictedOutputSlot)
        {
            restrictedSlot = groupedRestrictedInputSlot;
        }
        else
        {
            continue;
        }

        TestConnectionBetween(unrestrictedSlot->GetEndpoint(), restrictedSlot->GetEndpoint());

        for (auto inputPair : inputTypeMapping)
        {
            bool isValidConnection = inputPair.first == randomType;

            TestIsConnectionPossible(inputPair.second->GetEndpoint(), groupedUnrestrictedOutputSlot->GetEndpoint(), isValidConnection);
        }

        for (auto outputPair : outputTypeMapping)
        {
            bool isValidConnection = outputPair.first == randomType;

            TestIsConnectionPossible(groupedUnrestrictedInputSlot->GetEndpoint(), outputPair.second->GetEndpoint(), isValidConnection);
        }

        AZ::Entity* connection = nullptr;
        if (graph->FindConnection(connection, unrestrictedSlot->GetEndpoint(), restrictedSlot->GetEndpoint()))
        {
            graph->RemoveConnection(connection->GetId());
        }
    }
}

TEST_F(ScriptCanvasTestFixture, DynamicSlot_DisplayTypeDatum)
{
    using namespace ScriptCanvas;

    ConfigurableUnitTestNode* sourceNode = CreateConfigurableNode();
    Slot* dynamicSlot = nullptr;

    Data::Type randomType = GetRandomType();

    {
        DynamicDataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = GenerateSlotName();
        dataSlotConfiguration.SetConnectionType(ConnectionType::Input);
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Any;
        dataSlotConfiguration.m_displayType = randomType;

        dynamicSlot = sourceNode->AddTestingSlot(dataSlotConfiguration);
    }

    const Datum* sourceDatum = sourceNode->FindDatum(dynamicSlot->GetId());

    EXPECT_TRUE(sourceDatum != nullptr);

    for (auto dataType : GetTypes())
    {
        EXPECT_EQ(dataType == randomType, sourceDatum->IS_A(dataType));
    }
}
*/
