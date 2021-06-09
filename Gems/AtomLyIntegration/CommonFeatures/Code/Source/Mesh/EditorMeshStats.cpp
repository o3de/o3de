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

#include <Mesh/EditorMeshStats.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace AZ
{
    namespace Render
    {
        void EditorMeshStats::Reflect(ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorMeshStats>()
                    ->Field("stringRepresentation", &EditorMeshStats::m_stringRepresentation)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorMeshStats>(
                        "EditorMeshStats", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->DataElement(AZ::Edit::UIHandlers::MultiLineEdit, &EditorMeshStats::m_stringRepresentation, "Mesh Stats", "")
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                        ;
                }
            }
        }

        void EditorMeshStats::UpdateStringRepresentation()
        {
            m_stringRepresentation = "";
            int lodIndex = 0;
            for (auto& meshStatsForLod : m_meshStatsForLod)
            {
                m_stringRepresentation += AZStd::string::format("LOD: %d:\n", lodIndex++);
                m_stringRepresentation += AZStd::string::format("\tMesh Count: %d\n", meshStatsForLod.m_meshCount);
                m_stringRepresentation += AZStd::string::format("\tVert Count: %d\n", meshStatsForLod.m_vertCount);
                m_stringRepresentation += AZStd::string::format("\tTriangle Count: %d\n", meshStatsForLod.m_triCount);
            }
            AZ::StringFunc::TrimWhiteSpace(m_stringRepresentation, true, true);
        };        
    } // namespace Render
} // namespace AZ
