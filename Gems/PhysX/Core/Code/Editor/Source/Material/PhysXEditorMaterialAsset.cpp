/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <Editor/Source/Material/PhysXEditorMaterialAsset.h>

namespace PhysX
{
    void EditorMaterialAsset::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PhysX::EditorMaterialAsset, AZ::Data::AssetData>()
                ->Version(3)
                ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
                ->Field("MaterialConfiguration", &EditorMaterialAsset::m_materialConfiguration)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<PhysX::EditorMaterialAsset>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorMaterialAsset::m_materialConfiguration, "PhysX Material",
                        "PhysX material properties")
                        ->Attribute(AZ::Edit::Attributes::ForceAutoExpand, true);
            }
        }
    }

    const MaterialConfiguration& EditorMaterialAsset::GetMaterialConfiguration() const
    {
        return m_materialConfiguration;
    }
} // namespace PhysX
