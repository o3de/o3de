/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzFramework/Physics/Material/PhysicsMaterialAsset.h>

namespace Physics
{
    void MaterialAsset::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Physics::MaterialAsset, AZ::Data::AssetData>()
                ->Version(1)
                ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
                ->Field("MaterialConfiguration", &MaterialAsset::m_materialConfiguration)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<Physics::MaterialAsset>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &MaterialAsset::m_materialConfiguration, "Physics Material",
                        "PhysX material properties")
                        ->Attribute(AZ::Edit::Attributes::ForceAutoExpand, true);
            }
        }
    }

    const MaterialConfiguration2& MaterialAsset::GetMaterialConfiguration() const
    {
        return m_materialConfiguration;
    }
} // namespace Physics
