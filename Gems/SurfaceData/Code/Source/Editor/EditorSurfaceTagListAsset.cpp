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

#include "SurfaceData_precompiled.h"
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