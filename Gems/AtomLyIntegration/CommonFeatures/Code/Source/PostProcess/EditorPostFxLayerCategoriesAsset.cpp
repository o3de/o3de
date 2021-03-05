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

#include <PostProcess/EditorPostFxLayerCategoriesAsset.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Render
    {
        void EditorPostFxLayerCategoriesAsset::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<EditorPostFxLayerCategoriesAsset>()
                    ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
                    ->Version(0)
                    ->Field("Layer Categories", &EditorPostFxLayerCategoriesAsset::m_layerCategories)
                    ;

                if (AZ::EditContext* edit = serialize->GetEditContext())
                {
                    edit->Class<EditorPostFxLayerCategoriesAsset>(
                        "Layer Categories", "Contains priority-indexed layer categories used by PostFX Layer Component")
                        ->DataElement(0, &EditorPostFxLayerCategoriesAsset::m_layerCategories, "Layer Categories", "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                        ->ElementAttribute(AZ::Edit::Attributes::MaxLength, 64)
                        ;
                }
            }
        }
    }
}
