/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneData/GraphData/MeshVertexTangentData.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ::SceneData::GraphData
{
    void MeshVertexTangentData::Reflect(ReflectContext* context)
    {
        SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<MeshVertexTangentData>()->Version(2);
        }

        BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<MeshVertexTangentData>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "scene")
                ->Method("GetCount", &MeshVertexTangentData::GetCount)
                ->Method("GetTangent", &MeshVertexTangentData::GetTangent)
                ->Method("GetTangentSetIndex", &MeshVertexTangentData::GetTangentSetIndex)
                ->Method("GetGenerationMethod", &MeshVertexTangentData::GetGenerationMethod)
                ->Enum<(int)SceneAPI::DataTypes::TangentGenerationMethod::FromSourceScene>("FromSourceScene")
                ->Enum<(int)SceneAPI::DataTypes::TangentGenerationMethod::MikkT>("MikkT");
        }
    }

    void MeshVertexTangentData::CloneAttributesFrom(const IGraphObject* sourceObject)
    {
        IMeshVertexTangentData::CloneAttributesFrom(sourceObject);
        if (const auto* typedSource = azrtti_cast<const MeshVertexTangentData*>(sourceObject))
        {
            SetGenerationMethod(typedSource->GetGenerationMethod());
            SetTangentSetIndex(typedSource->GetTangentSetIndex());
        }
    }

    size_t MeshVertexTangentData::GetCount() const
    {
        return m_tangents.size();
    }

    const AZ::Vector4& MeshVertexTangentData::GetTangent(size_t index) const
    {
        AZ_Assert(index < m_tangents.size(), "Invalid index %i for mesh tangents.", index);
        return m_tangents[index];
    }

    void MeshVertexTangentData::ReserveContainerSpace(size_t numVerts)
    {
        m_tangents.reserve(numVerts);
    }

    void MeshVertexTangentData::Resize(size_t numVerts)
    {
        m_tangents.resize(numVerts);
    }

    void MeshVertexTangentData::AppendTangent(const AZ::Vector4& tangent)
    {
        m_tangents.push_back(tangent);
    }

    void MeshVertexTangentData::GetDebugOutput(AZ::SceneAPI::Utilities::DebugOutput& output) const
    {
        output.Write("Tangents", m_tangents);
        output.Write("GenerationMethod", aznumeric_cast<int64_t>(m_generationMethod));
        output.Write("SetIndex", aznumeric_cast<uint64_t>(m_setIndex));
    }

    void MeshVertexTangentData::SetTangent(size_t vertexIndex, const AZ::Vector4& tangent)
    {
        m_tangents[vertexIndex] = tangent;
    }

    void MeshVertexTangentData::SetTangentSetIndex(size_t setIndex)
    {
        m_setIndex = setIndex;
    }

    size_t MeshVertexTangentData::GetTangentSetIndex() const
    {
        return m_setIndex;
    }

    AZ::SceneAPI::DataTypes::TangentGenerationMethod MeshVertexTangentData::GetGenerationMethod() const
    { 
        return m_generationMethod;
    }

    void MeshVertexTangentData::SetGenerationMethod(AZ::SceneAPI::DataTypes::TangentGenerationMethod method)
    { 
        m_generationMethod = method;
    }
} // AZ::SceneData::GraphData
