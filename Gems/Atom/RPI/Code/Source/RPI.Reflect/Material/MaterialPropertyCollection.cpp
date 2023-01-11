/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/ColorManagement/TransformColor.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Reflect/Image/AttachmentImageAsset.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/ShaderReloadDebugTracker.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderSystemInterface.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>

#include <AtomCore/Instance/InstanceDatabase.h>
#include <AtomCore/Utils/ScopedValue.h>

namespace AZ
{
    namespace RPI
    {
        bool MaterialPropertyCollection::Init(
            RHI::ConstPtr<MaterialPropertiesLayout> layout,
            const AZStd::vector<MaterialPropertyValue>& defaultValues)
        {
            m_layout = layout;
            if (!m_layout)
            {
                AZ_Error("MaterialPropertyCollection", false, "MaterialPropertiesLayout is invalid");
                return false;
            }

            // If this Init() is actually a re-initialize, we need to re-apply any overridden property values
            // after loading the default property values, so we save that data here.
            MaterialPropertyFlags prevOverrideFlags = m_propertyOverrideFlags;
            AZStd::vector<MaterialPropertyValue> prevPropertyValues = m_propertyValues;

            // The property values are cleared to their default state to ensure that SetPropertyValue() does not early-return
            // when called below. This is important when Init() is actually a re-initialize.
            m_propertyValues.clear();

            // Initialize the shader runtime data like shader constant buffers and shader variants by applying the 
            // material's property values. This will feed through the normal runtime material value-change data flow, which may
            // include custom property change handlers provided by the material type.
            //
            // This baking process could be more efficient by doing it at build-time rather than run-time. However, the 
            // architectural complexity of supporting separate asset/runtime paths for assigning buffers/images is prohibitive.

            m_propertyValues.resize(defaultValues.size());
            AZ_Assert(defaultValues.size() == m_layout->GetPropertyCount(), "The number of properties in this material doesn't match the property layout");
            AZ_Assert(defaultValues.size() <= Limits::Material::PropertyCountMax, "Too many material properties. Max is %d.", Limits::Material::PropertyCountMax);

            for (size_t i = 0; i < defaultValues.size(); ++i)
            {
                const MaterialPropertyValue& value = defaultValues[i];
                MaterialPropertyIndex propertyIndex{i};
                if (!SetPropertyValue(propertyIndex, value))
                {
                    return false;
                }
            }

            // Clear all override flags because we just loaded properties from the asset
            m_propertyOverrideFlags.reset();

            // Now apply any properties that were overridden before
            for (size_t i = 0; i < prevPropertyValues.size(); ++i)
            {
                if (prevOverrideFlags[i])
                {
                    SetPropertyValue(MaterialPropertyIndex{i}, prevPropertyValues[i]);
                }
            }

            return true;
        }

        const MaterialPropertyValue& MaterialPropertyCollection::GetPropertyValue(MaterialPropertyIndex index) const
        {
            static const MaterialPropertyValue emptyValue;
            if (m_propertyValues.size() <= index.GetIndex())
            {
                AZ_Error("MaterialPropertyCollection", false, "Property index out of range.");
                return emptyValue;
            }
            return m_propertyValues[index.GetIndex()];
        }

        const AZStd::vector<MaterialPropertyValue>& MaterialPropertyCollection::GetPropertyValues() const
        {
            return m_propertyValues;
        }

        void MaterialPropertyCollection::SetAllPropertyDirtyFlags()
        {
            m_propertyDirtyFlags.set();
        }

        void MaterialPropertyCollection::ClearAllPropertyDirtyFlags()
        {
            m_propertyDirtyFlags.reset();
        }

        template<typename Type>
        bool MaterialPropertyCollection::SetPropertyValue(MaterialPropertyIndex index, const Type& value)
        {
            if (!index.IsValid())
            {
                AZ_Assert(false, "SetPropertyValue: Invalid MaterialPropertyIndex");
                return false;
            }

            const MaterialPropertyDescriptor* propertyDescriptor = m_layout->GetPropertyDescriptor(index);

            if (!ValidatePropertyAccess<Type>(propertyDescriptor))
            {
                return false;
            }

            MaterialPropertyValue& savedPropertyValue = m_propertyValues[index.GetIndex()];

            // If the property value didn't actually change, don't waste time running functors and compiling the changes.
            if (savedPropertyValue == value)
            {
                return false;
            }

            savedPropertyValue = value;
            m_propertyDirtyFlags.set(index.GetIndex());
            m_propertyOverrideFlags.set(index.GetIndex());

            return true;
        }

        template<>
        bool MaterialPropertyCollection::SetPropertyValue<Data::Asset<ImageAsset>>(MaterialPropertyIndex index, const Data::Asset<ImageAsset>& value)
        {
            Data::Asset<ImageAsset> imageAsset = value;

            if (!imageAsset.GetId().IsValid())
            {
                // The image asset reference is null so set the property to an empty Image instance so the AZStd::any will not be empty.
                return SetPropertyValue(index, Data::Instance<Image>());
            }
            else
            {
                AZ::Data::AssetType assetType = imageAsset.GetType();
                if (assetType != azrtti_typeid<StreamingImageAsset>() && assetType != azrtti_typeid<AttachmentImageAsset>())
                {
                    AZ::Data::AssetInfo assetInfo;
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                        assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, imageAsset.GetId());
                    assetType = assetInfo.m_assetType;
                }

                // There is an issue in the Asset<T>(Asset<U>) copy constructor which is used with the FindOrCreate() calls below.
                // If the AssetData is valid, then it will get the actual asset type ID from the AssetData. However, if it is null
                // then it will continue using the original type ID. The InstanceDatabase will end up asking the AssetManager for
                // the asset using the wrong type (ImageAsset) and will lead to various error messages and in the end the asset
                // will never be loaded. So we work around this issue by forcing the asset type ID to the correct value first.
                // See https://github.com/o3de/o3de/issues/12224
                if (!imageAsset.Get())
                {
                    imageAsset = Data::Asset<ImageAsset>{imageAsset.GetId(), assetType, imageAsset.GetHint()};
                }

                Data::Instance<Image> image = nullptr;
                if (assetType == azrtti_typeid<StreamingImageAsset>())
                {
                    Data::Asset<StreamingImageAsset> streamingImageAsset = imageAsset;
                    image = StreamingImage::FindOrCreate(streamingImageAsset);
                }
                else if (assetType == azrtti_typeid<AttachmentImageAsset>())
                {
                    Data::Asset<AttachmentImageAsset> attachmentImageAsset = imageAsset;
                    image = AttachmentImage::FindOrCreate(attachmentImageAsset);
                }
                else
                {
                    AZ_Error("MaterialPropertyCollection", false, "Unsupported image asset type: %s", assetType.ToString<AZStd::string>().c_str());
                    return false;
                }

                if (!image)
                {
                    AZ_Error("MaterialPropertyCollection", false, "Image asset could not be loaded");
                    return false;
                }

                return SetPropertyValue(index, image);
            }
        }

        template bool MaterialPropertyCollection::SetPropertyValue<bool>(MaterialPropertyIndex index, const bool& value);
        template bool MaterialPropertyCollection::SetPropertyValue<int32_t>(MaterialPropertyIndex index, const int32_t& value);
        template bool MaterialPropertyCollection::SetPropertyValue<uint32_t>(MaterialPropertyIndex index, const uint32_t& value);
        template bool MaterialPropertyCollection::SetPropertyValue<float>(MaterialPropertyIndex index, const float& value);
        template bool MaterialPropertyCollection::SetPropertyValue<Vector2>(MaterialPropertyIndex index, const Vector2& value);
        template bool MaterialPropertyCollection::SetPropertyValue<Vector3>(MaterialPropertyIndex index, const Vector3& value);
        template bool MaterialPropertyCollection::SetPropertyValue<Vector4>(MaterialPropertyIndex index, const Vector4& value);
        template bool MaterialPropertyCollection::SetPropertyValue<Color>(MaterialPropertyIndex index, const Color& value);
        template bool MaterialPropertyCollection::SetPropertyValue<Data::Instance<Image>>(MaterialPropertyIndex index, const Data::Instance<Image>& value);

        bool MaterialPropertyCollection::SetPropertyValue(MaterialPropertyIndex propertyIndex, const MaterialPropertyValue& value)
        {
            if (!value.IsValid())
            {
                auto descriptor = m_layout->GetPropertyDescriptor(propertyIndex);
                if (descriptor)
                {
                    AZ_Assert(false, "Empty value found for material property '%s'", descriptor->GetName().GetCStr());
                }
                else
                {
                    AZ_Assert(false, "Empty value found for material property [%d], and this property does not have a descriptor.");
                }
                return false;
            }
            if (value.Is<bool>())
            {
                return SetPropertyValue(propertyIndex, value.GetValue<bool>());
            }
            else if (value.Is<int32_t>())
            {
                return SetPropertyValue(propertyIndex, value.GetValue<int32_t>());
            }
            else if (value.Is<uint32_t>())
            {
                return SetPropertyValue(propertyIndex, value.GetValue<uint32_t>());
            }
            else if (value.Is<float>())
            {
                return SetPropertyValue(propertyIndex, value.GetValue<float>());
            }
            else if (value.Is<Vector2>())
            {
                return SetPropertyValue(propertyIndex, value.GetValue<Vector2>());
            }
            else if (value.Is<Vector3>())
            {
                return SetPropertyValue(propertyIndex, value.GetValue<Vector3>());
            }
            else if (value.Is<Vector4>())
            {
                return SetPropertyValue(propertyIndex, value.GetValue<Vector4>());
            }
            else if (value.Is<Color>())
            {
                return SetPropertyValue(propertyIndex, value.GetValue<Color>());
            }
            else if (value.Is<Data::Instance<Image>>())
            {
                return SetPropertyValue(propertyIndex, value.GetValue<Data::Instance<Image>>());
            }
            else if (value.Is<Data::Asset<ImageAsset>>())
            {
                return SetPropertyValue(propertyIndex, value.GetValue<Data::Asset<ImageAsset>>());
            }
            else
            {
                AZ_Assert(false, "Unhandled material property value type");
                return false;
            }
        }

        template<typename Type>
        const Type& MaterialPropertyCollection::GetPropertyValue(MaterialPropertyIndex index) const
        {
            static const Type defaultValue{};

            const MaterialPropertyDescriptor* propertyDescriptor = nullptr;
            if (Validation::IsEnabled())
            {
                if (!index.IsValid())
                {
                    AZ_Assert(false, "GetPropertyValue: Invalid MaterialPropertyIndex");
                    return defaultValue;
                }

                propertyDescriptor = m_layout->GetPropertyDescriptor(index);

                if (!ValidatePropertyAccess<Type>(propertyDescriptor))
                {
                    return defaultValue;
                }
            }

            const MaterialPropertyValue& value = m_propertyValues[index.GetIndex()];
            if (value.Is<Type>())
            {
                return value.GetValue<Type>();
            }
            else
            {
                if (Validation::IsEnabled())
                {
                    AZ_Assert(false, "Material property '%s': Stored property value has the wrong data type. Expected %s but is %s.",
                    propertyDescriptor->GetName().GetCStr(),
                        azrtti_typeid<Type>().template ToString<AZStd::string>().data(), // 'template' because clang says "error: use 'template' keyword to treat 'ToString' as a dependent template name"
                        value.GetTypeId().ToString<AZStd::string>().data());
                }
                return defaultValue;
            }
        }

        // Using explicit instantiation to restrict GetPropertyValue to the set of types that we support

        template const bool&     MaterialPropertyCollection::GetPropertyValue<bool>     (MaterialPropertyIndex index) const;
        template const int32_t&  MaterialPropertyCollection::GetPropertyValue<int32_t>  (MaterialPropertyIndex index) const;
        template const uint32_t& MaterialPropertyCollection::GetPropertyValue<uint32_t> (MaterialPropertyIndex index) const;
        template const float&    MaterialPropertyCollection::GetPropertyValue<float>    (MaterialPropertyIndex index) const;
        template const Vector2&  MaterialPropertyCollection::GetPropertyValue<Vector2>  (MaterialPropertyIndex index) const;
        template const Vector3&  MaterialPropertyCollection::GetPropertyValue<Vector3>  (MaterialPropertyIndex index) const;
        template const Vector4&  MaterialPropertyCollection::GetPropertyValue<Vector4>  (MaterialPropertyIndex index) const;
        template const Color&    MaterialPropertyCollection::GetPropertyValue<Color>    (MaterialPropertyIndex index) const;
        template const Data::Instance<Image>& MaterialPropertyCollection::GetPropertyValue<Data::Instance<Image>>(MaterialPropertyIndex index) const;

        const MaterialPropertyFlags& MaterialPropertyCollection::GetPropertyDirtyFlags() const
        {
            return m_propertyDirtyFlags;
        }

        RHI::ConstPtr<MaterialPropertiesLayout> MaterialPropertyCollection::GetMaterialPropertiesLayout() const
        {
            return m_layout;
        }

        template<typename Type>
        bool MaterialPropertyCollection::ValidatePropertyAccess(const MaterialPropertyDescriptor* propertyDescriptor) const
        {
            // Note that we have warnings here instead of errors because this can happen while materials are hot reloading
            // after a material property layout changes in the MaterialTypeAsset, as there's a brief time when the data
            // might be out of sync between MaterialAssets and MaterialTypeAssets.

            if (!propertyDescriptor)
            {
                AZ_Warning("MaterialPropertyCollection", false, "MaterialPropertyDescriptor is null");
                return false;
            }

            AZ::TypeId accessDataType = azrtti_typeid<Type>();

            // Must align with the order in MaterialPropertyDataType
            static const AZStd::array<AZ::TypeId, MaterialPropertyDataTypeCount> types =
            {{
                AZ::TypeId{}, // Invalid
                azrtti_typeid<bool>(),
                azrtti_typeid<int32_t>(),
                azrtti_typeid<uint32_t>(),
                azrtti_typeid<float>(),
                azrtti_typeid<Vector2>(),
                azrtti_typeid<Vector3>(),
                azrtti_typeid<Vector4>(),
                azrtti_typeid<Color>(),
                azrtti_typeid<Data::Instance<Image>>(),
                azrtti_typeid<uint32_t>()
            }};

            AZ::TypeId actualDataType = types[static_cast<size_t>(propertyDescriptor->GetDataType())];

            if (accessDataType != actualDataType)
            {
                AZ_Warning("MaterialPropertyCollection", false, "Material property '%s': Accessed as type %s but is type %s",
                    propertyDescriptor->GetName().GetCStr(),
                    GetMaterialPropertyDataTypeString(accessDataType).c_str(),
                    ToString(propertyDescriptor->GetDataType()));
                return false;
            }

            return true;
        }

    } // namespace RPI
} // namespace AZ
