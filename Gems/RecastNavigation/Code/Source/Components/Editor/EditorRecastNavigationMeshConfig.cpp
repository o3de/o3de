/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/Editor/EditorRecastNavigationMeshConfig.h>

namespace RecastNavigation
{
    void EditorRecastNavigationMeshConfig::Reflect(AZ::ReflectContext* context)
    {
        if (const auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            using Self = EditorRecastNavigationMeshConfig;

            serialize->Class<EditorRecastNavigationMeshConfig>()
                ->Field("Draw Mesh", &Self::m_showNavigationMesh)
                ->Field("Update Every (N) Seconds", &Self::m_updateEveryNSeconds)
                ->Field("(N) Threads to Use", &Self::m_backgroundThreadsToUse)
                ->Version(1)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<EditorRecastNavigationMeshConfig>("Editor Recast Navigation Mesh Config",
                    "[Navigation mesh configuration, Editor specific parameters]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(nullptr, &Self::m_showNavigationMesh, "Draw Mesh",
                        "Draw the debug view of mesh in Editor viewport")
                    ->DataElement(nullptr, &Self::m_updateEveryNSeconds, "Update Every (N) Seconds",
                        "Re-calculate the navigation mesh at least every (N) seconds. A negative value disables updates.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorRecastNavigationMeshConfig::OnUpdateEveryNSecondsChanged)
                    ->DataElement(nullptr, &Self::m_backgroundThreadsToUse, "(N) Threads to Use",
                        "Number of background threads to use when re-building navigation mesh in Editor viewport")
                    ;
            }
        }
    }

    void EditorRecastNavigationMeshConfig::BindUpdateEveryNSecondsFieldEventHandler(AZ::Event<int>::Handler& handler)
    {
        handler.Connect(m_updateEveryNSecondsFieldEvent);
    }

    AZ::Crc32 EditorRecastNavigationMeshConfig::OnUpdateEveryNSecondsChanged()
    {
        m_updateEveryNSecondsFieldEvent.Signal(m_updateEveryNSeconds);
        return AZ::Edit::PropertyRefreshLevels::None;
    }
}
