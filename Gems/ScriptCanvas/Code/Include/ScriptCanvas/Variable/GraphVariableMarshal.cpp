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

#include <ScriptCanvas/Variable/GraphVariableMarshal.h>
#include <ScriptCanvas/Variable/GraphVariable.h>
#include <ScriptCanvas/Variable/GraphVariableNetBindings.h>
#include <ScriptCanvas/Core/ModifiableDatumView.h>
#include <AzFramework/Network/EntityIdMarshaler.h>
#include <GridMate/Serialize/MathMarshal.h>
#include <GridMate/Serialize/UtilityMarshal.h>
#include <GridMate/Serialize/UuidMarshal.h>

namespace ScriptCanvas
{
    void DatumMarshaler::SetNetBindingTable(GraphVariableNetBindingTable* netBindingTable)
    {
        m_graphVariableNetBindingTable = netBindingTable;
    }

    void DatumMarshaler::Marshal(GridMate::WriteBuffer& wb, const Datum* const & property) const
    {
        if (!property)
        {
            return;
        }

        GridMate::Marshaler<Data::eType> typeMarshaler;
        const Data::eType& datumType = property->GetType().GetType();
        typeMarshaler.Marshal(wb, datumType);

        VariableId assetVariableId;
        AZStd::unordered_map<VariableId, AZStd::pair<GraphVariable*, int>>& variableIdMap = m_graphVariableNetBindingTable->GetVariableIdMap();

        for (AZStd::pair<VariableId, AZStd::pair<GraphVariable*, int>>& pair : variableIdMap)
        {
            AZStd::pair<GraphVariable*, int>& variableIndexPair = pair.second;

            if (variableIndexPair.first->GetDatum() == property)
            {
                assetVariableId = m_graphVariableNetBindingTable->FindAssetVariableIdByRuntimeVariableId(pair.first);
                break;
            }
        }

        if (!assetVariableId.IsValid())
        {
            return;
        }

        GridMate::Marshaler<AZ::Uuid> uuidMarshaler;
        uuidMarshaler.Marshal(wb, assetVariableId.GetDatumId());

        AZStd::string uuidString = assetVariableId.m_id.ToString<AZStd::string>();

        switch (datumType)
        {
            case Data::eType::AABB:
                MarshalType<Data::AABBType>(wb, property);
                break;

            case Data::eType::Boolean:
                MarshalType<Data::BooleanType>(wb, property);
                break;

            case Data::eType::Color:
                MarshalType<Data::ColorType>(wb, property);
                break;

            case Data::eType::CRC:
                MarshalType<Data::CRCType>(wb, property);
                break;

            case Data::eType::EntityID:
                MarshalType<Data::EntityIDType>(wb, property);
                break;

            case Data::eType::Matrix3x3:
                MarshalType<Data::Matrix3x3Type>(wb, property);
                break;

            case Data::eType::Matrix4x4:
                MarshalType<Data::Matrix4x4Type>(wb, property);
                break;

            case Data::eType::NamedEntityID:
                MarshalType<Data::NamedEntityIDType>(wb, property);
                break;

            case Data::eType::Number:
                MarshalType<Data::NumberType>(wb, property);
                break;

            case Data::eType::OBB:
                MarshalType<Data::OBBType>(wb, property);
                break;

            case Data::eType::Plane:
                MarshalType<Data::PlaneType>(wb, property);
                break;

            case Data::eType::Quaternion:
                MarshalType<Data::QuaternionType>(wb, property);
                break;

            case Data::eType::String:
                MarshalType<Data::StringType>(wb, property);
                break;

            case Data::eType::Transform:
                MarshalType<Data::TransformType>(wb, property);
                break;

            case Data::eType::Vector2:
                MarshalType<Data::Vector2Type>(wb, property);
                break;

            case Data::eType::Vector3:
                MarshalType<Data::Vector3Type>(wb, property);
                break;

            case Data::eType::Vector4:
                MarshalType<Data::Vector4Type>(wb, property);
                break;

            default:
                AZ_Warning("ScriptCanvasNetworking", false, "Marshal unsupported data type");
                break;
        }
    }

    bool DatumMarshaler::UnmarshalToPointer(const Datum*& target, GridMate::ReadBuffer& rb)
    {
        // :SCTODO: for some reason, this UnmarshalToPointer can get called before SetNetworkBinding is called
        // (which is where we set m_graphVariableNetBindingTable). So we check for nullptr here just in case.
        if (!m_graphVariableNetBindingTable)
        {
            return false;
        }

        ScriptCanvas::Data::eType datumType = Data::eType::Invalid;
        GridMate::Marshaler<Data::eType> typeMarshaler;
        typeMarshaler.Unmarshal(datumType, rb);

        AZ::Uuid uuid;
        GridMate::Marshaler<AZ::Uuid> uuidMarshaler;
        uuidMarshaler.Unmarshal(uuid, rb);

        VariableId runtimeVariableId = m_graphVariableNetBindingTable->FindRuntimeVariableIdByAssetVariableId(VariableId(uuid));

        if (!runtimeVariableId.IsValid())
        {
            return false;
        }

        AZStd::string uuidString = runtimeVariableId.m_id.ToString<AZStd::string>();

        AZStd::unordered_map<VariableId, AZStd::pair<GraphVariable*, int>>& m_variableIdMap = m_graphVariableNetBindingTable->GetVariableIdMap();
        AZStd::pair<GraphVariable*, int>& variableIndexPair = m_variableIdMap[runtimeVariableId];
        GraphVariable* graphVariable = variableIndexPair.first;

        switch (datumType)
        {
            case Data::eType::AABB:
                return UnmarshalType<Data::AABBType>(target, rb, graphVariable);

            case Data::eType::Boolean:
                return UnmarshalType<Data::BooleanType>(target, rb, graphVariable);

            case Data::eType::Color:
                return UnmarshalType<Data::ColorType>(target, rb, graphVariable);

            case Data::eType::CRC:
                return UnmarshalType<Data::CRCType>(target, rb, graphVariable);

            case Data::eType::EntityID:
                return UnmarshalType<Data::EntityIDType>(target, rb, graphVariable);

            case Data::eType::Matrix3x3:
                return UnmarshalType<Data::Matrix3x3Type>(target, rb, graphVariable);

            case Data::eType::Matrix4x4:
                return UnmarshalType<Data::Matrix4x4Type>(target, rb, graphVariable);

            case Data::eType::NamedEntityID:
                return UnmarshalType<Data::NamedEntityIDType>(target, rb, graphVariable);

            case Data::eType::Number:
                return UnmarshalType<Data::NumberType>(target, rb, graphVariable);

            case Data::eType::OBB:
                return UnmarshalType<Data::OBBType>(target, rb, graphVariable);

            case Data::eType::Plane:
                return UnmarshalType<Data::PlaneType>(target, rb, graphVariable);

            case Data::eType::Quaternion:
                return UnmarshalType<Data::QuaternionType>(target, rb, graphVariable);

            case Data::eType::String:
                return UnmarshalType<Data::StringType>(target, rb, graphVariable);

            case Data::eType::Transform:
                return UnmarshalType<Data::TransformType>(target, rb, graphVariable);

            case Data::eType::Vector2:
                return UnmarshalType<Data::Vector2Type>(target, rb, graphVariable);

            case Data::eType::Vector3:
                return UnmarshalType<Data::Vector3Type>(target, rb, graphVariable);

            case Data::eType::Vector4:
                return UnmarshalType<Data::Vector4Type>(target, rb, graphVariable);

            default:
                AZ_Warning("ScriptCanvasNetworking", false, "Unmarshal unsupported data type");
                break;
        }

        return false;
    }

    void DatumThrottler::SignalDirty()
    {
        m_isDirty = true;
    }

    bool DatumThrottler::WithinThreshold(const Datum* newValue) const
    {
        return (newValue == nullptr || !m_isDirty);
    }

    void DatumThrottler::UpdateBaseline([[maybe_unused]] const Datum* baseline)
    {
        m_isDirty = false;
    }
}
