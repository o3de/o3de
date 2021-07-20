/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "EditorDebugDrawComponentCommon.h"

namespace DebugDraw
{
    EditorDebugDrawComponentSettings::EditorDebugDrawComponentSettings()
        : m_visibleInGame(true)
        , m_visibleInEditor(true)
    {
    }

    void EditorDebugDrawComponentSettings::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorDebugDrawComponentSettings>()
                ->Version(0)
                ->Field("VisibleInGame", &EditorDebugDrawComponentSettings::m_visibleInGame)
                ->Field("VisibleInEditor", &EditorDebugDrawComponentSettings::m_visibleInEditor)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorDebugDrawComponentSettings>("DebugDraw Component Settings", "Common settings for DebugDraw components.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Debugging")
                    ->DataElement(0, &EditorDebugDrawComponentSettings::m_visibleInGame,
                        "Visible In Game", "Whether this DebugDraw component is visible in game.")
                    ->DataElement(0, &EditorDebugDrawComponentSettings::m_visibleInEditor,
                        "Visible In Editor", "Whether this DebugDraw component is visible in editor.")
                    ;
            }
        }
    }

} // namespace DebugDraw
