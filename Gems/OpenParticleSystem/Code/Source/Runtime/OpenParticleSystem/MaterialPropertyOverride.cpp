/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <OpenParticleSystem/MaterialPropertyOverride.h>

#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace OpenParticle
{
    namespace
    {
        //! Coerces whatever numeric type happens to be sitting in the any into the type the property wants.
        //! The editor stores doubles for float properties, and script can hand back int for uint, so a plain
        //! any_cast is not enough.
        template<typename TargetType>
        AZ::RPI::MaterialPropertyValue ConvertNumericType(const AZStd::any& value)
        {
            if (value.is<bool>())
            {
                return aznumeric_cast<TargetType>(AZStd::any_cast<bool>(value));
            }
            if (value.is<int8_t>())
            {
                return aznumeric_cast<TargetType>(AZStd::any_cast<int8_t>(value));
            }
            if (value.is<int16_t>())
            {
                return aznumeric_cast<TargetType>(AZStd::any_cast<int16_t>(value));
            }
            if (value.is<int32_t>())
            {
                return aznumeric_cast<TargetType>(AZStd::any_cast<int32_t>(value));
            }
            if (value.is<int64_t>())
            {
                return aznumeric_cast<TargetType>(AZStd::any_cast<int64_t>(value));
            }
            if (value.is<uint8_t>())
            {
                return aznumeric_cast<TargetType>(AZStd::any_cast<uint8_t>(value));
            }
            if (value.is<uint16_t>())
            {
                return aznumeric_cast<TargetType>(AZStd::any_cast<uint16_t>(value));
            }
            if (value.is<uint32_t>())
            {
                return aznumeric_cast<TargetType>(AZStd::any_cast<uint32_t>(value));
            }
            if (value.is<uint64_t>())
            {
                return aznumeric_cast<TargetType>(AZStd::any_cast<uint64_t>(value));
            }
            if (value.is<float>())
            {
                return aznumeric_cast<TargetType>(AZStd::any_cast<float>(value));
            }
            if (value.is<double>())
            {
                return aznumeric_cast<TargetType>(AZStd::any_cast<double>(value));
            }

            return AZ::RPI::MaterialPropertyValue::FromAny(value);
        }
    } // namespace

    void ReflectMaterialPropertyOverrides(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->RegisterGenericType<MaterialPropertyOverrideMap>();
        }
    }

    AZ::RPI::MaterialPropertyValue ConvertMaterialPropertyValue(
        const AZ::RPI::MaterialPropertyDescriptor* propertyDescriptor, const AZStd::any& value)
    {
        if (propertyDescriptor == nullptr)
        {
            return AZ::RPI::MaterialPropertyValue::FromAny(value);
        }

        switch (propertyDescriptor->GetDataType())
        {
        case AZ::RPI::MaterialPropertyDataType::Enum:
            // Enums round trip through the editor as either the symbolic name or the raw index.
            if (value.is<AZ::Name>())
            {
                return propertyDescriptor->GetEnumValue(AZStd::any_cast<AZ::Name>(value));
            }
            if (value.is<AZStd::string>())
            {
                return propertyDescriptor->GetEnumValue(AZ::Name(AZStd::any_cast<AZStd::string>(value)));
            }
            return ConvertNumericType<uint32_t>(value);
        case AZ::RPI::MaterialPropertyDataType::Int:
            return ConvertNumericType<int32_t>(value);
        case AZ::RPI::MaterialPropertyDataType::UInt:
            return ConvertNumericType<uint32_t>(value);
        case AZ::RPI::MaterialPropertyDataType::Float:
            return ConvertNumericType<float>(value);
        case AZ::RPI::MaterialPropertyDataType::Bool:
            return ConvertNumericType<bool>(value);
        default:
            break;
        }

        // Vector/Color/Image/SamplerState are all handled by FromAny, including resolving an asset id or an
        // image asset reference into something Material::SetPropertyValue can consume.
        return AZ::RPI::MaterialPropertyValue::FromAny(value);
    }

    bool ApplyMaterialPropertyOverrides(
        const AZ::Data::Instance<AZ::RPI::Material>& material, const MaterialPropertyOverrideMap& overrides)
    {
        if (!material || overrides.empty())
        {
            return true;
        }

        for (const auto& [propertyId, propertyValue] : overrides)
        {
            if (propertyValue.empty())
            {
                continue;
            }

            bool wasRenamed = false;
            AZ::Name newName;
            AZ::RPI::MaterialPropertyIndex propertyIndex = material->FindPropertyIndex(propertyId, &wasRenamed, &newName);

            // If the material type renamed this property and we also carry an override under the new name,
            // the new name wins and the stale entry is ignored rather than silently clobbering it.
            if (wasRenamed && overrides.find(newName) != overrides.end())
            {
                AZ_Warning(
                    "OpenParticle", false,
                    "Emitter material property '%s' was renamed to '%s' and an override exists for both. "
                    "The override using the old name will be ignored.",
                    propertyId.GetCStr(), newName.GetCStr());
                continue;
            }

            if (propertyIndex.IsNull())
            {
                AZ_Warning(
                    "OpenParticle", false,
                    "Emitter material property override '%s' does not exist on material '%s' and will be ignored.",
                    propertyId.GetCStr(), material->GetAsset().GetHint().c_str());
                continue;
            }

            const auto* propertyDescriptor = material->GetMaterialPropertiesLayout()->GetPropertyDescriptor(propertyIndex);
            material->SetPropertyValue(propertyIndex, ConvertMaterialPropertyValue(propertyDescriptor, propertyValue));
        }

        // Compile can fail when the SRG is still queued for this frame. Reporting that lets the caller retry.
        return !material->NeedsCompile() || material->Compile();
    }

    void PruneMaterialPropertyOverrides(
        const AZ::Data::Asset<AZ::RPI::MaterialAsset>& materialAsset, MaterialPropertyOverrideMap& overrides)
    {
        if (overrides.empty())
        {
            return;
        }

        // Without a loaded asset there is no layout to validate against, so leave the overrides alone rather
        // than throwing away values that are probably still valid.
        if (!materialAsset.IsReady() || materialAsset->GetMaterialPropertiesLayout() == nullptr)
        {
            return;
        }

        const auto* layout = materialAsset->GetMaterialPropertiesLayout();
        for (auto it = overrides.begin(); it != overrides.end();)
        {
            AZ::Name propertyId = it->first;
            if (materialAsset->GetMaterialTypeAsset())
            {
                materialAsset->GetMaterialTypeAsset()->ApplyPropertyRenames(propertyId);
            }

            if (layout->FindPropertyIndex(propertyId).IsNull())
            {
                it = overrides.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
} // namespace OpenParticle
