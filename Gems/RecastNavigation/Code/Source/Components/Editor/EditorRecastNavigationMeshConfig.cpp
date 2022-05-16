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
                ->Version(1);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<EditorRecastNavigationMeshConfig>("Editor Recast Navigation Mesh Config",
                    "[Navigation mesh configuration, Editor specific parameters]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(nullptr, &Self::m_showNavigationMesh, "Draw Mesh",
                        "Draw the debug view of mesh in Editor viewport");
            }
        }
    }
}
