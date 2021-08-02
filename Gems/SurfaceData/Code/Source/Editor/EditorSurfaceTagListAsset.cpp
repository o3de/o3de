/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorSurfaceTagListAsset.h"
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace SurfaceData
{
    void EditorSurfaceTagListAsset::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<EditorSurfaceTagListAsset>()
                ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
                ->Version(0)
                ->Field("SurfaceTagNames", &EditorSurfaceTagListAsset::m_surfaceTagNames)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<EditorSurfaceTagListAsset>(
                    "Surface Tag Name List Asset", "Contains a list of tag names")
                    ->DataElement(0, &EditorSurfaceTagListAsset::m_surfaceTagNames, "Surface Tag Name List", "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                        ->ElementAttribute(AZ::Edit::Attributes::MaxLength, 64)
                    ;
            }
        }
    }
}
