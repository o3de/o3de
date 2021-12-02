/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>

#include <Mesh/EditorMeshStats.h>
#include <Mesh/EditorMeshStatsSerializer.h>

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

            if (auto jsonContext = azrtti_cast<JsonRegistrationContext*>(context))
            {
                jsonContext->Serializer<JsonEditorMeshStatsSerializer>()->HandlesType<EditorMeshStats>();
            }

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
                            ->Attribute(AZ::Edit::Attributes::IndexedChildNameLabelOverride, &EditorMeshStats::GetLodLabel)
                        ;
                }
            }
        }

        AZStd::string EditorMeshStats::GetLodLabel(int index) const
        {
            return AZStd::string::format("LOD %d", index);
        }
    } // namespace Render
} // namespace AZ
