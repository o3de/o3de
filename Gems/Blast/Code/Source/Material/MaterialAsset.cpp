/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <Material/MaterialAsset.h>

namespace Blast
{
    void MaterialAsset::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Blast::MaterialAsset, AZ::Data::AssetData>()
                ->Version(1)
                ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
                ->Field("MaterialConfiguration", &MaterialAsset::m_materialConfiguration)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<Blast::MaterialAsset>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &MaterialAsset::m_materialConfiguration, "Blast Material",
                        "Blast material properties")
                        ->Attribute(AZ::Edit::Attributes::ForceAutoExpand, true);
            }
        }
    }

    const MaterialConfiguration& MaterialAsset::GetMaterialConfiguration() const
    {
        return m_materialConfiguration;
    }
} // namespace Blast
