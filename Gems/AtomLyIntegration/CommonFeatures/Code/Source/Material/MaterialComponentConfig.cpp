/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentConfig.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace Render
    {
        using DeprecatedMaterialAssignmentId = AZStd::pair<MaterialAssignmentLodIndex, AZ::Data::AssetId>;
        using DeprecatedMaterialAssignmentMap = AZStd::unordered_map<DeprecatedMaterialAssignmentId, MaterialAssignment>;

        // Update serialized data to the new format and data types
        bool MaterialComponentConfigVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < 3)
            {
                constexpr AZ::u32 materialDataCrc = AZ_CRC_CE("Materials");

                // MaterialAssignmentId was changed from an AZStd::pair to an explicit structure
                // Any previously stored data needs to be converted to preserve existing levels and slices
                DeprecatedMaterialAssignmentMap oldMaterials;
                if (!classElement.GetChildData(materialDataCrc, oldMaterials))
                {
                    AZ_Error("AZ::Render::MaterialComponentConfigVersionConverter", false, "Failed to get Materials element");
                    return false;
                }

                if (!classElement.RemoveElementByName(materialDataCrc))
                {
                    AZ_Error("AZ::Render::MaterialComponentConfigVersionConverter", false, "Failed to remove Materials element");
                    return false;
                }

                // Transform the old map to the new format
                MaterialAssignmentMap newMaterials;
                for (const auto& oldPair : oldMaterials)
                {
                    const DeprecatedMaterialAssignmentId& oldId = oldPair.first;
                    const MaterialAssignmentId newId(oldId.first, oldId.second.m_subId);
                    newMaterials[newId] = oldPair.second;

                }
                classElement.AddElementWithData(context, "materials", newMaterials);
            }

            return true;
        }

        void MaterialComponentConfig::Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                // The types being replaced must be reflected to deserialize old data
                serializeContext->RegisterGenericType<DeprecatedMaterialAssignmentId>();
                serializeContext->RegisterGenericType<DeprecatedMaterialAssignmentMap>();

                serializeContext->Class<MaterialComponentConfig, ComponentConfig>()
                    ->Version(3, MaterialComponentConfigVersionConverter)
                    ->Field("materials", &MaterialComponentConfig::m_materials)
                    ;
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<MaterialComponentConfig>("MaterialComponentConfig")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "render")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Constructor()
                    ->Constructor<const MaterialComponentConfig&>()
                    ->Property("materials", BehaviorValueProperty(&MaterialComponentConfig::m_materials))
                    ;
            }
        }
    } // namespace Render
} // namespace AZ
