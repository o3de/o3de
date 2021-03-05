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

#include <SceneAPI/SceneData/GraphData/MeshVertexColorData.h>

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            const AZ::Name& MeshVertexColorData::GetCustomName() const
            {
                return m_customName;
            }

            void MeshVertexColorData::SetCustomName(const char* name)
            {
                m_customName = name;
            }


            size_t MeshVertexColorData::GetCount() const
            {
                return m_colors.size();
            }

            const AZ::SceneAPI::DataTypes::Color& MeshVertexColorData::GetColor(size_t index) const
            {
                AZ_Assert(index < m_colors.size(), "Invalid index %i for mesh vertex color.", index);
                return m_colors[index];
            }

            void MeshVertexColorData::ReserveContainerSpace(size_t size)
            {
                m_colors.reserve(size);
            }

            void MeshVertexColorData::AppendColor(const AZ::SceneAPI::DataTypes::Color& color)
            {
                m_colors.push_back(color);
            }

            void MeshVertexColorData::GetDebugOutput(AZ::SceneAPI::Utilities::DebugOutput& output) const
            {
                output.Write("Colors", m_colors);
                output.Write("ColorsCustomName", m_customName.GetCStr());
            }
        } // GraphData
    } // SceneData
} // AZ
