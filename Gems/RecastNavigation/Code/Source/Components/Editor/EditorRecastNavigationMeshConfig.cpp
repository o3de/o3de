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
                ->Field("Auto Update in Editor", &Self::m_autoUpdateNavigationMesh)
                ->Field("Threads", &Self::m_backgroundThreadsToUse)
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
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorRecastNavigationMeshConfig::OnShowNavMeshChanged)
                    ->DataElement(nullptr, &Self::m_autoUpdateNavigationMesh, "Auto Update in Editor",
                        "Re-calculate the navigation mesh at least every (N) seconds. A negative value disables updates.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorRecastNavigationMeshConfig::OnAutoUpdateChanged)
                    ->DataElement(nullptr, &Self::m_backgroundThreadsToUse, "Threads",
                        "Number of background threads to use when re-building navigation mesh in Editor viewport")
                    ;
            }
        }
    }

    void EditorRecastNavigationMeshConfig::BindAutoUpdateChangedEventHandler(AZ::Event<bool>::Handler& handler)
    {
        handler.Connect(m_autoUpdateNavigationMeshEvent);
    }

    void EditorRecastNavigationMeshConfig::BindShowNavMeshChangedEventHandler(AZ::Event<bool>::Handler& handler)
    {
        handler.Connect(m_showNavigationMeshEvent);
    }

    AZ::Crc32 EditorRecastNavigationMeshConfig::OnShowNavMeshChanged()
    {
        m_showNavigationMeshEvent.Signal(m_showNavigationMesh);
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::Crc32 EditorRecastNavigationMeshConfig::OnAutoUpdateChanged()
    {
        m_autoUpdateNavigationMeshEvent.Signal(m_autoUpdateNavigationMesh);
        return AZ::Edit::PropertyRefreshLevels::None;
    }
}
