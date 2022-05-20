/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <AzFramework/Physics/Material/PhysicsMaterialAsset.h>

namespace Physics
{
    void MaterialAsset::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Physics::MaterialAsset, AZ::Data::AssetData>()
                ->Version(2)
                ->Field("MaterialProperties", &MaterialAsset::m_materialProperties)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<Physics::MaterialAsset>("Physics MaterialAsset", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "");
            }
        }

        if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Method("ReflectAssetPhysicsMaterialAsset", [](AZ::Data::Asset<MaterialAsset>) {});
        }
    }

    void MaterialAsset::SetData(const AZStd::unordered_map<AZStd::string, float>& materialProperties)
    {
        m_materialProperties = materialProperties;
        m_status = AZ::Data::AssetData::AssetStatus::Ready;
    }

    const AZStd::unordered_map<AZStd::string, float>& MaterialAsset::GetMaterialProperties() const
    {
        return m_materialProperties;
    }
} // namespace Physics
