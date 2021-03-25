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
#pragma once
#include <AzTest/AzTest.h>
#include <ScriptCanvas/Execution/RuntimeBus.h>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;

    class RuntimeRequestsMock : public RuntimeRequests
    {
    public:
        MOCK_CONST_METHOD1(FindAssetVariableIdByRuntimeVariableId, VariableId(VariableId));
        MOCK_CONST_METHOD1(FindRuntimeVariableIdByAssetVariableId, VariableId(VariableId));
        MOCK_CONST_METHOD1(FindAssetNodeIdByRuntimeNodeId, AZ::EntityId(AZ::EntityId));
        MOCK_CONST_METHOD0(GetAssetId, AZ::Data::AssetId());
        MOCK_CONST_METHOD0(GetGraphIdentifier, GraphIdentifier());
        MOCK_CONST_METHOD0(GetAssetName, AZStd::string());
        MOCK_CONST_METHOD1(FindNode, Node*(AZ::EntityId));
        MOCK_CONST_METHOD1(FindRuntimeNodeIdByAssetNodeId, AZ::EntityId(AZ::EntityId));
        MOCK_CONST_METHOD0(GetRuntimeEntityId, AZ::EntityId());
        MOCK_CONST_METHOD0(GetNodes, AZStd::vector<AZ::EntityId>());
        MOCK_CONST_METHOD0(GetConnections, AZStd::vector<AZ::EntityId>());
        MOCK_CONST_METHOD1(GetConnectedEndpoints, AZStd::vector<Endpoint>(const Endpoint&));
        MOCK_CONST_METHOD1(GetConnectedEndpointIterators, AZStd::pair< EndpointMapConstIterator, EndpointMapConstIterator >(const Endpoint&));
        MOCK_CONST_METHOD1(IsEndpointConnected, bool(const Endpoint&));
        MOCK_METHOD0(GetGraphData, GraphData*());
        MOCK_CONST_METHOD0(GetGraphDataConst, const GraphData*());
        MOCK_METHOD0(GetVariableData, VariableData*());
        MOCK_CONST_METHOD0(GetVariableDataConst, const VariableData*());
        MOCK_CONST_METHOD0(GetVariables, const GraphVariableMapping*());
        MOCK_METHOD1(FindVariable, GraphVariable*(AZStd::string_view));
        MOCK_METHOD1(FindVariableById, GraphVariable*(const VariableId&));
        MOCK_CONST_METHOD1(GetVariableType, Data::Type(const VariableId&));
        MOCK_CONST_METHOD1(GetVariableName, AZStd::string_view(const VariableId&));
        MOCK_CONST_METHOD0(IsGraphObserved, bool());
        MOCK_METHOD1(SetIsGraphObserved, void(bool));
        MOCK_CONST_METHOD0(GetAssetType, AZ::Data::AssetType());
    };
}
