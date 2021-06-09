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

namespace AZ
{
    namespace Render
    {
        void EditorMeshStatsForLod::Reflect(ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorMeshStatsForLod>()
                    ->Field("meshCount", &EditorMeshStatsForLod::m_meshCount)
                    ->Field("vertCount", &EditorMeshStatsForLod::m_vertCount)
                    ->Field("triCount", &EditorMeshStatsForLod::m_triCount)
                ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorMeshStatsForLod>("EditorMeshStatsForLod", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorMeshStatsForLod::m_meshCount, "Mesh Count", "")
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorMeshStatsForLod::m_vertCount, "Vert Count", "")
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorMeshStatsForLod::m_triCount, "Tri Count", "")
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                    ;
                }
            }
        }

        void EditorMeshStats::Reflect(ReflectContext* context)
        {
            EditorMeshStatsForLod::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorMeshStats>()
                    ->Field("meshStatsForLod", &EditorMeshStats::m_meshStatsForLod)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorMeshStats>(
                        "EditorMeshStats", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorMeshStats::m_meshStatsForLod, "Mesh Stats", "")
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }
        }
    } // namespace Render
} // namespace AZ
