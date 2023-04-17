/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneData/GraphData/MeshVertexUVData.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            void MeshVertexUVData::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<MeshVertexUVData>()->Version(1);
                }

                BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
                if (behaviorContext)
                {
                    behaviorContext->Class<MeshVertexUVData>()
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                        ->Attribute(AZ::Script::Attributes::Module, "scene")
                        ->Method("GetCustomName",[](const MeshVertexUVData& self) { return self.GetCustomName().GetCStr(); })
                        ->Method("GetCount", &MeshVertexUVData::GetCount)
                        ->Method("GetUV", &MeshVertexUVData::GetUV);
                }
            }

            void MeshVertexUVData::CloneAttributesFrom(const IGraphObject* sourceObject)
            {
                IMeshVertexUVData::CloneAttributesFrom(sourceObject);
                if (const auto* typedSource = azrtti_cast<const MeshVertexUVData*>(sourceObject))
                {
                    SetCustomName(typedSource->GetCustomName());
                }
            }

            const AZ::Name& MeshVertexUVData::GetCustomName() const
            {
                return m_customName;
            }

            void MeshVertexUVData::SetCustomName(const char* name)
            {
                m_customName = name;
            }
            void MeshVertexUVData::SetCustomName(const AZ::Name& name)
            {
                m_customName = name;
            }

            size_t MeshVertexUVData::GetCount() const
            {
                return m_uvs.size();
            }

            const AZ::Vector2& MeshVertexUVData::GetUV(size_t index) const
            {
                AZ_Assert(index < m_uvs.size(), "Invalid index %i for mesh vertex UVs.", index);
                return m_uvs[index];
            }

            void MeshVertexUVData::ReserveContainerSpace(size_t size)
            {
                m_uvs.reserve(size);
            }

            void MeshVertexUVData::Clear()
            {
                m_uvs.clear();
            }

            void MeshVertexUVData::AppendUV(const AZ::Vector2& uv)
            {
                m_uvs.push_back(uv);
            }

            void MeshVertexUVData::GetDebugOutput(AZ::SceneAPI::Utilities::DebugOutput& output) const
            {
                output.Write("UVs", m_uvs);
                output.Write("UVCustomName", m_customName.GetCStr());
            }
        } // GraphData
    } // SceneData
} // AZ
