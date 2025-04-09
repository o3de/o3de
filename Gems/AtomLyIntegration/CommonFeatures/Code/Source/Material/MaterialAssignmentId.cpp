/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomLyIntegration/CommonFeatures/Material/MaterialAssignmentId.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Render
    {
        bool MaterialAssignmentId::ConvertVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < 2)
            {
                constexpr AZ::u32 materialAssetIdCrc = AZ_CRC_CE("materialAssetId");

                AZ::Data::AssetId materialAssetId;
                if (!classElement.GetChildData(materialAssetIdCrc, materialAssetId))
                {
                    AZ_Error("AZ::Render::MaterialAssignmentId::ConvertVersion", false, "Failed to get AssetId element");
                    return false;
                }

                if (!classElement.RemoveElementByName(materialAssetIdCrc))
                {
                    AZ_Error("AZ::Render::MaterialAssignmentId::ConvertVersion", false, "Failed to remove deprecated element materialAssetId");
                    // No need to early-return, the object will still load successfully, it will just report more errors about the unrecognized element.
                }

                if (materialAssetId.IsValid())
                {
                    classElement.AddElementWithData(context, "materialSlotStableId", materialAssetId.m_subId);
                }
                else
                {
                    classElement.AddElementWithData(context, "materialSlotStableId", RPI::ModelMaterialSlot::InvalidStableId);
                }
            }

            return true;
        }

        void MaterialAssignmentId::Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<MaterialAssignmentId>()
                    ->Version(2, &MaterialAssignmentId::ConvertVersion)
                    ->Field("lodIndex", &MaterialAssignmentId::m_lodIndex)
                    ->Field("materialSlotStableId", &MaterialAssignmentId::m_materialSlotStableId)
                    ;

                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<MaterialAssignmentId>(
                        "Material Assignment Id", "Material Assignment Id")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialAssignmentId::m_lodIndex, "LOD Index", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialAssignmentId::m_materialSlotStableId, "Material Slot Stable Id", "")
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<MaterialAssignmentId>("MaterialAssignmentId")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "render")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Constructor()
                    ->Constructor<const MaterialAssignmentId&>()
                    ->Constructor<MaterialAssignmentLodIndex, RPI::ModelMaterialSlot::StableId>()
                    ->Method("IsDefault", &MaterialAssignmentId::IsDefault)
                    ->Method("IsAssetOnly", &MaterialAssignmentId::IsSlotIdOnly)     // Included for compatibility. Use "IsSlotIdOnly" instead.
                    ->Method("IsLodAndAsset", &MaterialAssignmentId::IsLodAndSlotId) // Included for compatibility. Use "IsLodAndSlotId" instead.
                    ->Method("IsSlotIdOnly", &MaterialAssignmentId::IsSlotIdOnly)
                    ->Method("IsLodAndSlotId", &MaterialAssignmentId::IsLodAndSlotId)
                    ->Method("ToString", &MaterialAssignmentId::ToString)
                    ->Property("lodIndex", BehaviorValueProperty(&MaterialAssignmentId::m_lodIndex))
                    ->Property("materialSlotStableId", BehaviorValueProperty(&MaterialAssignmentId::m_materialSlotStableId))
                    ;
            }
        }

        MaterialAssignmentId::MaterialAssignmentId(MaterialAssignmentLodIndex lodIndex, RPI::ModelMaterialSlot::StableId materialSlotStableId)
            : m_lodIndex(lodIndex)
            , m_materialSlotStableId(materialSlotStableId)
        {
        }

        MaterialAssignmentId MaterialAssignmentId::CreateDefault()
        {
            return MaterialAssignmentId(NonLodIndex, RPI::ModelMaterialSlot::InvalidStableId);
        }

        MaterialAssignmentId MaterialAssignmentId::CreateFromStableIdOnly(RPI::ModelMaterialSlot::StableId materialSlotStableId)
        {
            return MaterialAssignmentId(NonLodIndex, materialSlotStableId);
        }

        MaterialAssignmentId MaterialAssignmentId::CreateFromLodAndStableId(
            MaterialAssignmentLodIndex lodIndex, RPI::ModelMaterialSlot::StableId materialSlotStableId)
        {
            return MaterialAssignmentId(lodIndex, materialSlotStableId);
        }

        bool MaterialAssignmentId::IsDefault() const
        {
            return m_lodIndex == NonLodIndex && m_materialSlotStableId == RPI::ModelMaterialSlot::InvalidStableId;
        }

        bool MaterialAssignmentId::IsSlotIdOnly() const
        {
            return m_lodIndex == NonLodIndex && m_materialSlotStableId != RPI::ModelMaterialSlot::InvalidStableId;
        }

        bool MaterialAssignmentId::IsLodAndSlotId() const
        {
            return m_lodIndex != NonLodIndex && m_materialSlotStableId != RPI::ModelMaterialSlot::InvalidStableId;
        }

        AZStd::string MaterialAssignmentId::ToString() const
        {
            return AZStd::string::format("%u:%llu", m_materialSlotStableId, m_lodIndex);
        }

        size_t MaterialAssignmentId::GetHash() const
        {
            size_t seed = 0;
            AZStd::hash_combine(seed, m_lodIndex);
            AZStd::hash_combine(seed, m_materialSlotStableId);
            return seed;
        }

        bool MaterialAssignmentId::operator==(const MaterialAssignmentId& rhs) const
        {
            return m_lodIndex == rhs.m_lodIndex && m_materialSlotStableId == rhs.m_materialSlotStableId;
        }

        bool MaterialAssignmentId::operator!=(const MaterialAssignmentId& rhs) const
        {
            return !(*this == rhs);
        }
    } // namespace Render
} // namespace AZ
