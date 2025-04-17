/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/PlatformDef.h>

#include <AzCore/RTTI/RTTI.h>

#include <Source/Framework/ScriptCanvasTestFixture.h>
#include <Source/Framework/ScriptCanvasTestUtilities.h>
#include <Source/Framework/ScriptCanvasTestNodes.h>

#include <ScriptCanvas/Core/NodeableNodeOverloaded.h>
#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Core/SlotConfigurationDefaults.h>

using namespace ScriptCanvasTests;

template<typename Type>
class SingleTypeNodeable
    : public ScriptCanvas::Nodeable
{
private:
    using ClassName = SingleTypeNodeable<Type>;

public:
    AZ_RTTI((SingleTypeNodeable, "{62F173B5-596B-4872-B88B-E03DFCD5D059}", Type), ScriptCanvas::Nodeable);
    AZ_CLASS_ALLOCATOR(SingleTypeNodeable<Type>, AZ::SystemAllocator);

    static void Reflect(AZ::ReflectContext* reflectContext)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<SingleTypeNodeable, Nodeable>()
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SingleTypeNodeable>("Single Type Nodeable", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
        {
            behaviorContext->Class<ClassName>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List)
                ->Method("MethodOne", &ClassName::MethodOne)
                ->Method("MethodTwo", &ClassName::MethodTwo)
                ;
        }
    }

    void MethodOne(Type paramOne)
    {
        AZ_UNUSED(paramOne);
    }

    void MethodTwo(Type paramOne, Type paramTwo)
    {
        AZ_UNUSED(paramOne);
        AZ_UNUSED(paramTwo);
    }
};

class SingleTypeNodeableNode
    : public ScriptCanvas::Nodes::NodeableNodeOverloaded
{
public:
    AZ_COMPONENT(SingleTypeNodeableNode, "{CDB445D4-A129-4E40-90E7-332DF825CC5E}", ScriptCanvas::Nodes::NodeableNodeOverloaded);

    static void Reflect(AZ::ReflectContext* reflectContext)
    {
        SingleTypeNodeable<ScriptCanvas::Data::NumberType>::Reflect(reflectContext);
        SingleTypeNodeable<ScriptCanvas::Data::Vector2Type>::Reflect(reflectContext);
        SingleTypeNodeable<ScriptCanvas::Data::Vector3Type>::Reflect(reflectContext);
        SingleTypeNodeable<ScriptCanvas::Data::Vector4Type>::Reflect(reflectContext);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<SingleTypeNodeableNode, ScriptCanvas::Nodes::NodeableNodeOverloaded>()
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SingleTypeNodeableNode>("Lerp Between", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Math")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void ConfigureSlots() override
    {
        ScriptCanvas::SlotExecution::Ins ins;
        {
            {
                ScriptCanvas::SlotExecution::In methodOne;
                ScriptCanvas::ExecutionSlotConfiguration inSlotConfiguration;
                inSlotConfiguration.m_name = "MethodOne";
                inSlotConfiguration.m_displayGroup = "In";
                inSlotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Input);
                inSlotConfiguration.m_isLatent = false;

                ScriptCanvas::SlotId inSlotId = AddSlot(inSlotConfiguration);

                AZ_Assert(inSlotId.IsValid(), "In slot is not created successfully.");

                methodOne.slotId = inSlotId;

                {
                    ScriptCanvas::DynamicDataSlotConfiguration dataSlotConfiguration;
                    dataSlotConfiguration.m_name = "Param";
                    dataSlotConfiguration.m_displayGroup = "In";

                    dataSlotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Input);
                    dataSlotConfiguration.m_dynamicDataType = ScriptCanvas::DynamicDataType::Value;

                    // Since this contract will check in with the underlying overload to enforce
                    // the typing, we don't need to have one of these contracts on each slot, as each
                    // group assignment will trigger this contract to confirm the typing.
                    dataSlotConfiguration.m_contractDescs = AZStd::vector<ScriptCanvas::ContractDescriptor>
                    {
                        { []() { return aznew ScriptCanvas::OverloadContract(); } }
                    };

                    ScriptCanvas::SlotId slotId = AddSlot(dataSlotConfiguration);

                    AZ_Assert(slotId.IsValid(), "Data slot is not created successfully.");

                    methodOne.inputs.push_back(slotId);
                }

                ins.push_back(methodOne);
            }

            {
                ScriptCanvas::SlotExecution::In methodTwo;

                ScriptCanvas::ExecutionSlotConfiguration inSlotConfiguration;
                inSlotConfiguration.m_name = "Cancel";
                inSlotConfiguration.m_displayGroup = "Cancel";
                inSlotConfiguration.m_toolTip = "Stops the lerp action immediately.";
                inSlotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Input);
                inSlotConfiguration.m_isLatent = false;

                ScriptCanvas::SlotId inSlotId = AddSlot(inSlotConfiguration);

                AZ_Assert(inSlotId.IsValid(), "Cancel slot is not created successfully.");
                methodTwo.slotId = inSlotId;

                {
                    ScriptCanvas::DynamicDataSlotConfiguration dataSlotConfiguration;
                    dataSlotConfiguration.m_name = "ParamOne";
                    dataSlotConfiguration.m_displayGroup = "In";

                    dataSlotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Input);
                    dataSlotConfiguration.m_dynamicDataType = ScriptCanvas::DynamicDataType::Value;

                    // Since this contract will check in with the underlying overload to enforce
                    // the typing, we don't need to have one of these contracts on each slot, as each
                    // group assignment will trigger this contract to confirm the typing.
                    dataSlotConfiguration.m_contractDescs = AZStd::vector<ScriptCanvas::ContractDescriptor>
                    {
                        { []() { return aznew ScriptCanvas::OverloadContract(); } }
                    };

                    ScriptCanvas::SlotId slotId = AddSlot(dataSlotConfiguration);

                    AZ_Assert(slotId.IsValid(), "Data slot is not created successfully.");

                    methodTwo.inputs.push_back(slotId);
                }

                {
                    ScriptCanvas::DynamicDataSlotConfiguration dataSlotConfiguration;
                    dataSlotConfiguration.m_name = "ParamTwo";
                    dataSlotConfiguration.m_displayGroup = "In";

                    dataSlotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Input);
                    dataSlotConfiguration.m_dynamicDataType = ScriptCanvas::DynamicDataType::Value;

                    // Since this contract will check in with the underlying overload to enforce
                    // the typing, we don't need to have one of these contracts on each slot, as each
                    // group assignment will trigger this contract to confirm the typing.
                    dataSlotConfiguration.m_contractDescs = AZStd::vector<ScriptCanvas::ContractDescriptor>
                    {
                        { []() { return aznew ScriptCanvas::OverloadContract(); } }
                    };

                    ScriptCanvas::SlotId slotId = AddSlot(dataSlotConfiguration);

                    AZ_Assert(slotId.IsValid(), "Data slot is not created successfully.");

                    methodTwo.inputs.push_back(slotId);
                }

                ins.push_back(methodTwo);
            }
        }

        SetSlotExecutionMap(AZStd::move(ScriptCanvas::SlotExecution::Map(AZStd::move(ins), {})));
    }

    AZStd::vector<AZStd::unique_ptr<ScriptCanvas::Nodeable>> GetInitializationNodeables() const override
    {
        AZStd::vector<AZStd::unique_ptr<ScriptCanvas::Nodeable>> nodeables;
        nodeables.push_back(AZStd::make_unique<SingleTypeNodeable<ScriptCanvas::Data::NumberType>>());
        nodeables.push_back(AZStd::make_unique<SingleTypeNodeable<ScriptCanvas::Data::Vector2Type>>());
        nodeables.push_back(AZStd::make_unique<SingleTypeNodeable<ScriptCanvas::Data::Vector3Type>>());
        nodeables.push_back(AZStd::make_unique<SingleTypeNodeable<ScriptCanvas::Data::Vector4Type>>());
        return nodeables;
    }
};

TEST_F(ScriptCanvasTestFixture, Overload_NodeableNode_TypeCheck)
{
    RegisterComponentDescriptor<SingleTypeNodeableNode>();

    ScriptCanvas::Graph* graph = CreateGraph();

    const ScriptCanvas::ScriptCanvasId& scriptCanvasId = graph->GetScriptCanvasId();

    AZ::EntityId nodeableNodeId;
    SingleTypeNodeableNode* nodeableNode = CreateTestNode<SingleTypeNodeableNode>(scriptCanvasId, nodeableNodeId);
    nodeableNode->PostActivate();

    ScriptCanvas::SlotId paramId = nodeableNode->GetSlotByName("Param")->GetId();
    ScriptCanvas::SlotId paramOneId = nodeableNode->GetSlotByName("ParamOne")->GetId();
    ScriptCanvas::SlotId paramTwoId = nodeableNode->GetSlotByName("ParamTwo")->GetId();

    AZStd::vector< ScriptCanvas::SlotId > slotIds = {
        paramId,
        paramOneId,
        paramTwoId
    };

    AZStd::vector< ScriptCanvas::Data::Type > acceptedTypes = {
        ScriptCanvas::Data::Type::Number(),
        ScriptCanvas::Data::Type::Vector2(),
        ScriptCanvas::Data::Type::Vector3(),
        ScriptCanvas::Data::Type::Vector4()
    };

    AZStd::vector< ScriptCanvas::Data::Type > invalidTypes = {
        ScriptCanvas::Data::Type::EntityID(),
        ScriptCanvas::Data::Type::Color(),
        ScriptCanvas::Data::Type::Transform()
    };

    for (ScriptCanvas::Data::Type acceptedType : acceptedTypes)
    {
        for (ScriptCanvas::SlotId slotId : slotIds)
        {
            AZ::Outcome<void, AZStd::string> validTypeSlot = nodeableNode->IsValidTypeForSlot(slotId, acceptedType);
            EXPECT_TRUE(validTypeSlot.IsSuccess());
        }
    }

    for (ScriptCanvas::Data::Type invalidType : invalidTypes)
    {
        for (ScriptCanvas::SlotId slotId : slotIds)
        {
            AZ::Outcome<void, AZStd::string> invalidTypeSlot = nodeableNode->IsValidTypeForSlot(slotId, invalidType);
            EXPECT_FALSE(invalidTypeSlot.IsSuccess());
        }
    }
}

TEST_F(ScriptCanvasTestFixture, Overload_NodeableNode_ConnectionCheck)
{
    RegisterComponentDescriptor<SingleTypeNodeableNode>();

    ScriptCanvas::Graph* graph = CreateGraph();

    const ScriptCanvas::ScriptCanvasId& scriptCanvasId = graph->GetScriptCanvasId();

    AZ::EntityId nodeableNodeId;
    SingleTypeNodeableNode* nodeableNode = CreateTestNode<SingleTypeNodeableNode>(scriptCanvasId, nodeableNodeId);
    nodeableNode->PostActivate();

    TestNodes::ConfigurableUnitTestNode* unitTestNode = CreateConfigurableNode("ConfigurableNode");

    ScriptCanvas::SlotId paramId = nodeableNode->GetSlotByName("Param")->GetId();
    ScriptCanvas::SlotId paramOneId = nodeableNode->GetSlotByName("ParamOne")->GetId();
    ScriptCanvas::SlotId paramTwoId = nodeableNode->GetSlotByName("ParamTwo")->GetId();

    AZStd::vector< ScriptCanvas::SlotId > slotIds = {
        paramId,
        paramOneId,
        paramTwoId
    };

    AZStd::vector< ScriptCanvas::Data::Type > acceptedTypes = {
        ScriptCanvas::Data::Type::Number(),
        ScriptCanvas::Data::Type::Vector2(),
        ScriptCanvas::Data::Type::Vector3(),
        ScriptCanvas::Data::Type::Vector4()
    };

    AZStd::vector< ScriptCanvas::Data::Type > invalidTypes = {
        ScriptCanvas::Data::Type::EntityID(),
        ScriptCanvas::Data::Type::Color(),
        ScriptCanvas::Data::Type::Transform()
    };

    for (ScriptCanvas::Data::Type acceptedType : acceptedTypes)
    {
        ScriptCanvas::Slot* validSlot = nullptr;

        {
            ScriptCanvas::DataSlotConfiguration slotConfiguration;

            slotConfiguration.m_name = GenerateSlotName();
            slotConfiguration.SetType(acceptedType);
            slotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Output);

            validSlot = unitTestNode->AddTestingSlot(slotConfiguration);
        }

        ScriptCanvas::Endpoint validStartEndpoint = validSlot->GetEndpoint();

        for (ScriptCanvas::SlotId slotId : slotIds)
        {
            ScriptCanvas::Endpoint paramEndpoint = { nodeableNode->GetEntityId(), slotId };

            const bool isValid = true;
            TestIsConnectionPossible(validStartEndpoint, paramEndpoint, isValid);
        }
    }

    for (ScriptCanvas::Data::Type invalidType : invalidTypes)
    {
        ScriptCanvas::Slot* invalidSlot = nullptr;

        {
            ScriptCanvas::DataSlotConfiguration slotConfiguration;

            slotConfiguration.m_name = GenerateSlotName();
            slotConfiguration.SetType(invalidType);
            slotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Output);

            invalidSlot = unitTestNode->AddTestingSlot(slotConfiguration);
        }

        ScriptCanvas::Endpoint invalidStartEndpoint = invalidSlot->GetEndpoint();

        for (ScriptCanvas::SlotId slotId : slotIds)
        {
            ScriptCanvas::Endpoint paramEndpoint = { nodeableNode->GetEntityId(), slotId };

            const bool isValid = false;
            TestIsConnectionPossible(invalidStartEndpoint, paramEndpoint, isValid);
        }
    }
}

TEST_F(ScriptCanvasTestFixture, Overload_NodeableNode_NumberConnection)
{
    RegisterComponentDescriptor<SingleTypeNodeableNode>();

    ScriptCanvas::Graph* graph = CreateGraph();

    const ScriptCanvas::ScriptCanvasId& scriptCanvasId = graph->GetScriptCanvasId();

    AZ::EntityId nodeableNodeId;
    SingleTypeNodeableNode* nodeableNode = CreateTestNode<SingleTypeNodeableNode>(scriptCanvasId, nodeableNodeId);
    nodeableNode->PostActivate();

    TestNodes::ConfigurableUnitTestNode* unitTestNode = CreateConfigurableNode("ConfigurableNode");

    ScriptCanvas::SlotId paramId = nodeableNode->GetSlotByName("Param")->GetId();
    ScriptCanvas::SlotId paramOneId = nodeableNode->GetSlotByName("ParamOne")->GetId();
    ScriptCanvas::SlotId paramTwoId = nodeableNode->GetSlotByName("ParamTwo")->GetId();

    AZStd::vector< ScriptCanvas::SlotId > slotIds = {
        paramId,
        paramOneId,
        paramTwoId
    };

    AZStd::vector< ScriptCanvas::Data::Type > invalidTypes = {
        ScriptCanvas::Data::Type::Vector2(),
        ScriptCanvas::Data::Type::Vector3(),
        ScriptCanvas::Data::Type::Vector4(),
        ScriptCanvas::Data::Type::EntityID(),
        ScriptCanvas::Data::Type::Color(),
        ScriptCanvas::Data::Type::Transform()
    };

    ScriptCanvas::Slot* validSlot = nullptr;

    {
        ScriptCanvas::DataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.SetType(ScriptCanvas::Data::Type::Number());
        slotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Output);

        validSlot = unitTestNode->AddTestingSlot(slotConfiguration);
    }

    ScriptCanvas::Slot* paramOneSlot = nodeableNode->GetSlot(paramOneId);
    ScriptCanvas::Slot* paramTwoSlot = nodeableNode->GetSlot(paramTwoId);

    EXPECT_FALSE(paramOneSlot->HasDisplayType());
    EXPECT_FALSE(paramTwoSlot->HasDisplayType());


    ScriptCanvas::Endpoint validStartEndpoint = validSlot->GetEndpoint();

    ScriptCanvas::Endpoint paramEndpoint = { nodeableNode->GetEntityId(), paramId };
    TestConnectionBetween(validStartEndpoint, paramEndpoint);    

    EXPECT_TRUE(paramOneSlot->HasDisplayType());
    EXPECT_EQ(paramOneSlot->GetDisplayType(), ScriptCanvas::Data::Type::Number());

    EXPECT_TRUE(paramTwoSlot->HasDisplayType());
    EXPECT_EQ(paramTwoSlot->GetDisplayType(), ScriptCanvas::Data::Type::Number());

    for (ScriptCanvas::Data::Type invalidType : invalidTypes)
    {
        ScriptCanvas::Slot* invalidSlot = nullptr;

        {
            ScriptCanvas::DataSlotConfiguration slotConfiguration;

            slotConfiguration.m_name = GenerateSlotName();
            slotConfiguration.SetType(invalidType);
            slotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Output);

            invalidSlot = unitTestNode->AddTestingSlot(slotConfiguration);
        }

        ScriptCanvas::Endpoint invalidStartEndpoint = invalidSlot->GetEndpoint();

        for (ScriptCanvas::SlotId slotId : slotIds)
        {
            ScriptCanvas::Endpoint paramEndpoint2 = { nodeableNode->GetEntityId(), slotId };

            const bool isValid = false;
            TestIsConnectionPossible(invalidStartEndpoint, paramEndpoint2, isValid);
        }
    }
}

TEST_F(ScriptCanvasTestFixture, Overload_NodeableNode_Vector3Connection)
{
    RegisterComponentDescriptor<SingleTypeNodeableNode>();

    ScriptCanvas::Graph* graph = CreateGraph();

    const ScriptCanvas::ScriptCanvasId& scriptCanvasId = graph->GetScriptCanvasId();

    AZ::EntityId nodeableNodeId;
    SingleTypeNodeableNode* nodeableNode = CreateTestNode<SingleTypeNodeableNode>(scriptCanvasId, nodeableNodeId);
    nodeableNode->PostActivate();

    TestNodes::ConfigurableUnitTestNode* unitTestNode = CreateConfigurableNode("ConfigurableNode");

    ScriptCanvas::SlotId paramId = nodeableNode->GetSlotByName("Param")->GetId();
    ScriptCanvas::SlotId paramOneId = nodeableNode->GetSlotByName("ParamOne")->GetId();
    ScriptCanvas::SlotId paramTwoId = nodeableNode->GetSlotByName("ParamTwo")->GetId();

    AZStd::vector< ScriptCanvas::SlotId > slotIds = {
        paramId,
        paramOneId,
        paramTwoId
    };

    AZStd::vector< ScriptCanvas::Data::Type > invalidTypes = {
        ScriptCanvas::Data::Type::Number(),
        ScriptCanvas::Data::Type::Vector2(),
        ScriptCanvas::Data::Type::Vector4(),
        ScriptCanvas::Data::Type::EntityID(),
        ScriptCanvas::Data::Type::Color(),
        ScriptCanvas::Data::Type::Transform()
    };

    ScriptCanvas::Slot* validSlot = nullptr;

    {
        ScriptCanvas::DataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = GenerateSlotName();
        slotConfiguration.SetType(ScriptCanvas::Data::Type::Vector3());
        slotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Output);

        validSlot = unitTestNode->AddTestingSlot(slotConfiguration);
    }

    ScriptCanvas::Slot* paramOneSlot = nodeableNode->GetSlot(paramOneId);
    ScriptCanvas::Slot* paramTwoSlot = nodeableNode->GetSlot(paramTwoId);

    EXPECT_FALSE(paramOneSlot->HasDisplayType());
    EXPECT_FALSE(paramTwoSlot->HasDisplayType());


    ScriptCanvas::Endpoint validStartEndpoint = validSlot->GetEndpoint();

    ScriptCanvas::Endpoint paramEndpoint = { nodeableNode->GetEntityId(), paramId };
    TestConnectionBetween(validStartEndpoint, paramEndpoint);

    EXPECT_TRUE(paramOneSlot->HasDisplayType());
    EXPECT_EQ(paramOneSlot->GetDisplayType(), ScriptCanvas::Data::Type::Vector3());

    EXPECT_TRUE(paramTwoSlot->HasDisplayType());
    EXPECT_EQ(paramTwoSlot->GetDisplayType(), ScriptCanvas::Data::Type::Vector3());

    for (ScriptCanvas::Data::Type invalidType : invalidTypes)
    {
        ScriptCanvas::Slot* invalidSlot = nullptr;

        {
            ScriptCanvas::DataSlotConfiguration slotConfiguration;

            slotConfiguration.m_name = GenerateSlotName();
            slotConfiguration.SetType(invalidType);
            slotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Output);

            invalidSlot = unitTestNode->AddTestingSlot(slotConfiguration);
        }

        ScriptCanvas::Endpoint invalidStartEndpoint = invalidSlot->GetEndpoint();

        for (ScriptCanvas::SlotId slotId : slotIds)
        {
            ScriptCanvas::Endpoint paramEndpoint2 = { nodeableNode->GetEntityId(), slotId };

            const bool isValid = false;
            TestIsConnectionPossible(invalidStartEndpoint, paramEndpoint2, isValid);
        }
    }
}
