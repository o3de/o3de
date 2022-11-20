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
        const char* Material::s_debugTraceName = "Material";

        Data::Instance<Material> Material::FindOrCreate(const Data::Asset<MaterialAsset>& materialAsset)
        {
            return Data::InstanceDatabase<Material>::Instance().FindOrCreate(materialAsset);
        }

        Data::Instance<Material> Material::Create(const Data::Asset<MaterialAsset>& materialAsset)
        {
            return Data::InstanceDatabase<Material>::Instance().Create(materialAsset);
        }

        AZ::Data::Instance<Material> Material::CreateInternal(MaterialAsset& materialAsset)
        {
            Data::Instance<Material> material = aznew Material();
            const RHI::ResultCode resultCode = material->Init(materialAsset);

            if (resultCode == RHI::ResultCode::Success)
            {
                return material;
            }

            return nullptr;
        }

        RHI::ResultCode Material::Init(MaterialAsset& materialAsset)
        {
            AZ_PROFILE_FUNCTION(RPI);

            ScopedValue isInitializing(&m_isInitializing, true, false);

            m_materialAsset = { &materialAsset, AZ::Data::AssetLoadBehavior::PreLoad };

            // Cache off pointers to some key data structures from the material type...
            auto srgLayout = m_materialAsset->GetMaterialSrgLayout();
            if (srgLayout)
            {
                auto shaderAsset = m_materialAsset->GetMaterialTypeAsset()->GetShaderAssetForMaterialSrg();
                m_shaderResourceGroup = ShaderResourceGroup::Create(shaderAsset, srgLayout->GetName());

                if (m_shaderResourceGroup)
                {
                    m_rhiShaderResourceGroup = m_shaderResourceGroup->GetRHIShaderResourceGroup();
                }
                else
                {
                    // No need to report an error message here, ShaderResourceGroup::Create() will have reported.
                    return RHI::ResultCode::Fail;
                }
            }

            m_layout = m_materialAsset->GetMaterialPropertiesLayout();
            if (!m_layout)
            {
                AZ_Error(s_debugTraceName, false, "MaterialAsset did not have a valid MaterialPropertiesLayout");
                return RHI::ResultCode::Fail;
            }

            // Copy the shader collection because the material will make changes, like updating the ShaderVariantId.
            m_shaderCollection = materialAsset.GetShaderCollection();

            // Register for update events related to Shader instances that own the ShaderAssets inside
            // the shader collection.
            ShaderReloadNotificationBus::MultiHandler::BusDisconnect();
            for (auto& shaderItem : m_shaderCollection)
            {
                ShaderReloadDebugTracker::Printf("(Material has ShaderAsset %p)", shaderItem.GetShaderAsset().Get());
                ShaderReloadNotificationBus::MultiHandler::BusConnect(shaderItem.GetShaderAsset().GetId());
            }

            // If this Init() is actually a re-initialize, we need to re-apply any overridden property values
            // after loading the property values from the asset, so we save that data here.
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
            {
                m_propertyValues.resize(materialAsset.GetPropertyValues().size());
                AZ_Assert(m_propertyValues.size() == m_layout->GetPropertyCount(), "The number of properties in this material doesn't match the property layout");

                for (size_t i = 0; i < materialAsset.GetPropertyValues().size(); ++i)
                {
                    const MaterialPropertyValue& value = materialAsset.GetPropertyValues()[i];
                    MaterialPropertyIndex propertyIndex{i};
                    if (!SetPropertyValue(propertyIndex, value))
                    {
                        return RHI::ResultCode::Fail;
                    }
                }

                AZ_Assert(materialAsset.GetPropertyValues().size() <= Limits::Material::PropertyCountMax,
                    "Too many material properties. Max is %d.", Limits::Material::PropertyCountMax);
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

            // Usually SetProperties called above will increment this change ID to invalidate
            // the material, but some materials might not have any properties, and we need
            // the material to be invalidated particularly when hot-reloading.
            ++m_currentChangeId;
            // Set all dirty for the first use.
            m_propertyDirtyFlags.set();

            Compile();

            return RHI::ResultCode::Success;
        }

        Material::~Material()
        {
            ShaderReloadNotificationBus::MultiHandler::BusDisconnect();
        }

        const ShaderCollection& Material::GetShaderCollection() const
        {
            return m_shaderCollection;
        }

        AZ::Outcome<uint32_t> Material::SetSystemShaderOption(const Name& shaderOptionName, RPI::ShaderOptionValue value)
        {
            uint32_t appliedCount = 0;

            // We won't set any shader options if the shader option is owned by any of the other shaders in this material.
            // If the material uses an option in any shader, then it owns that option for all its shaders.
            for (auto& shaderItem : m_shaderCollection)
            {
                const ShaderOptionGroupLayout* layout = shaderItem.GetShaderOptions()->GetShaderOptionLayout();
                ShaderOptionIndex index = layout->FindShaderOptionIndex(shaderOptionName);
                if (index.IsValid())
                {
                    if (shaderItem.MaterialOwnsShaderOption(index))
                    {
                        return AZ::Failure();
                    }
                }
            }

            for (auto& shaderItem : m_shaderCollection)
            {
                const ShaderOptionGroupLayout* layout = shaderItem.GetShaderOptions()->GetShaderOptionLayout();
                ShaderOptionIndex index = layout->FindShaderOptionIndex(shaderOptionName);
                if (index.IsValid())
                {
                    shaderItem.GetShaderOptions()->SetValue(index, value);
                    appliedCount++;
                }
            }

            return AZ::Success(appliedCount);
        }

        void Material::ApplyGlobalShaderOptions()
        {
            // [GFX TODO][ATOM-5625] This really needs to be optimized to put the burden on setting global shader options, not applying global shader options.
            // For example, make the shader system collect a map of all shaders and ShaderVaraintIds, and look up the shader option names at set-time.
            ShaderSystemInterface* shaderSystem = ShaderSystemInterface::Get();
            for (auto iter : shaderSystem->GetGlobalShaderOptions())
            {
                const Name& shaderOptionName = iter.first;
                ShaderOptionValue value = iter.second;
                if (!SetSystemShaderOption(shaderOptionName, value).IsSuccess())
                {
                    AZ_Warning("Material", false, "Shader option '%s' is owned by this material. Global value for this option was ignored.", shaderOptionName.GetCStr());
                }
            }
        }

        void Material::SetPsoHandlingOverride(MaterialPropertyPsoHandling psoHandlingOverride)
        {
            m_psoHandling = psoHandlingOverride;
        }

        const RHI::ShaderResourceGroup* Material::GetRHIShaderResourceGroup() const
        {
            return m_rhiShaderResourceGroup;
        }

        const Data::Asset<MaterialAsset>& Material::GetAsset() const
        {
            return m_materialAsset;
        }

        bool Material::CanCompile() const
        {
            return !m_shaderResourceGroup || !m_shaderResourceGroup->IsQueuedForCompile();
        }

        ///////////////////////////////////////////////////////////////////
        // ShaderReloadNotificationBus overrides...
        void Material::OnShaderReinitialized([[maybe_unused]] const Shader& shader)
        {
            ShaderReloadDebugTracker::ScopedSection reloadSection("{%p}->Material::OnShaderReinitialized %s", this, shader.GetAsset().GetHint().c_str());
            // Note that it might not be strictly necessary to reinitialize the entire material, we might be able to get away with
            // just bumping the m_currentChangeId or some other minor updates. But it's pretty hard to know what exactly needs to be
            // updated to correctly handle the reload, so it's safer to just reinitialize the whole material.
            Init(*m_materialAsset);
        }

        void Material::OnShaderAssetReinitialized(const Data::Asset<ShaderAsset>& shaderAsset)
        {
            ShaderReloadDebugTracker::ScopedSection reloadSection("{%p}->Material::OnShaderAssetReinitialized %s", this, shaderAsset.GetHint().c_str());
            // Note that it might not be strictly necessary to reinitialize the entire material, we might be able to get away with
            // just bumping the m_currentChangeId or some other minor updates. But it's pretty hard to know what exactly needs to be
            // updated to correctly handle the reload, so it's safer to just reinitialize the whole material.
            Init(*m_materialAsset);
        }

        void Material::OnShaderVariantReinitialized(const ShaderVariant& shaderVariant)
        {
            ShaderReloadDebugTracker::ScopedSection reloadSection("{%p}->Material::OnShaderVariantReinitialized %s", this, shaderVariant.GetShaderVariantAsset().GetHint().c_str());

            // Note that it would be better to check the shaderVariantId to see if that variant is relevant to this particular material before reinitializing it.
            // There could be hundreds or even thousands of variants for a shader, but only one of those variants will be used by any given material. So we could
            // get better reload performance by only reinitializing the material when a relevant shader variant is updated.
            //
            // But it isn't always possible to know the exact ShaderVariantId that this material is using. For example, some of the shader options might not be
            // owned by the material and could be set externally *later* in the frame (see SetSystemShaderOption). We could probably check the shader option ownership
            // and mask out the parts of the ShaderVariantId that aren't owned by the material, but that would be premature optimization at this point, adding
            // potentially unnecessary complexity. There may also be more edge cases I haven't thought of. In short, it's much safer to just reinitialize every time
            // this callback happens.
            Init(*m_materialAsset);
        }
        ///////////////////////////////////////////////////////////////////

        const MaterialPropertyValue& Material::GetPropertyValue(MaterialPropertyIndex index) const
        {
            static const MaterialPropertyValue emptyValue;
            if (m_propertyValues.size() <= index.GetIndex())
            {
                AZ_Error("Material", false, "Property index out of range.");
                return emptyValue;
            }
            return m_propertyValues[index.GetIndex()];
        }

        const AZStd::vector<MaterialPropertyValue>& Material::GetPropertyValues() const
        {
            return m_propertyValues;
        }

        bool Material::NeedsCompile() const
        {
            return m_compiledChangeId != m_currentChangeId;
        }

        bool Material::Compile()
        {
            AZ_PROFILE_FUNCTION(RPI);

            if (!NeedsCompile())
            {
                return true;
            }

            if (CanCompile())
            {
                // On some platforms, PipelineStateObjects must be pre-compiled and shipped with the game; they cannot be compiled at runtime. So at some
                // point the material system needs to be smart about when it allows PSO changes and when it doesn't. There is a task scheduled to
                // thoroughly address this in 2022, but for now we just report a warning to alert users who are using the engine in a way that might
                // not be supported for much longer. PSO changes should only be allowed in developer tools (though we could also expose a way for users to
                // enable dynamic PSO changes if their project only targets platforms that support this).
                // PSO modifications are allowed during initialization because that's using the stored asset data, which the asset system can
                // access to pre-compile the necessary PSOs.
                MaterialPropertyPsoHandling psoHandling = m_isInitializing ? MaterialPropertyPsoHandling::Allowed : m_psoHandling;


                AZ_PROFILE_BEGIN(RPI, "Material::Compile() Processing Functors");
                for (const Ptr<MaterialFunctor>& functor : m_materialAsset->GetMaterialFunctors())
                {
                    if (functor)
                    {
                        const MaterialPropertyFlags& materialPropertyDependencies = functor->GetMaterialPropertyDependencies();
                        // None covers case that the client code doesn't register material properties to dependencies,
                        // which will later get caught in Process() when trying to access a property.
                        if (materialPropertyDependencies.none() || functor->NeedsProcess(m_propertyDirtyFlags))
                        {
                            MaterialFunctor::RuntimeContext processContext = MaterialFunctor::RuntimeContext(
                                m_propertyValues,
                                m_layout,
                                &m_shaderCollection,
                                m_shaderResourceGroup.get(),
                                &materialPropertyDependencies,
                                psoHandling
                            );


                            functor->Process(processContext);
                        }
                    }
                    else
                    {
                        // This could happen when the dll containing the functor class is missing. There will likely be more errors
                        // preceding this one, from the serialization system when loading the material asset.
                        AZ_Error(s_debugTraceName, false, "Material functor is null.");
                    }
                }
                AZ_PROFILE_END(RPI);

                m_propertyDirtyFlags.reset();

                if (m_shaderResourceGroup)
                {
                    m_shaderResourceGroup->Compile();
                }

                m_compiledChangeId = m_currentChangeId;

                return true;
            }

            return false;
        }

        Material::ChangeId Material::GetCurrentChangeId() const
        {
            return m_currentChangeId;
        }

        MaterialPropertyIndex Material::FindPropertyIndex(const Name& propertyId, bool* wasRenamed, Name* newName) const
        {
            if (wasRenamed)
            {
                *wasRenamed = false;
            }

            MaterialPropertyIndex index = m_layout->FindPropertyIndex(propertyId);
            if (!index.IsValid())
            {
                Name renamedId = propertyId;
                
                if (m_materialAsset->GetMaterialTypeAsset()->ApplyPropertyRenames(renamedId))
                {                                
                    index = m_layout->FindPropertyIndex(renamedId);

                    if (wasRenamed)
                    {
                        *wasRenamed = true;
                    }

                    if (newName)
                    {
                        *newName = renamedId;
                    }

                    AZ_Warning("Material", false,
                        "Material property '%s' has been renamed to '%s'. Consider updating the corresponding source data.",
                        propertyId.GetCStr(),
                        renamedId.GetCStr());
                }
            }
            return index;
        }

        template<typename Type>
        bool Material::ValidatePropertyAccess(const MaterialPropertyDescriptor* propertyDescriptor) const
        {
            // Note that we have warnings here instead of errors because this can happen while materials are hot reloading
            // after a material property layout changes in the MaterialTypeAsset, as there's a brief time when the data
            // might be out of sync between MaterialAssets and MaterialTypeAssets.

            if (!propertyDescriptor)
            {
                AZ_Warning(s_debugTraceName, false, "MaterialPropertyDescriptor is null");
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
                AZ_Warning(s_debugTraceName, false, "Material property '%s': Accessed as type %s but is type %s",
                    propertyDescriptor->GetName().GetCStr(),
                    GetMaterialPropertyDataTypeString(accessDataType).c_str(),
                    ToString(propertyDescriptor->GetDataType()));
                return false;
            }

            return true;
        }

        template<typename Type>
        void Material::SetShaderConstant(RHI::ShaderInputConstantIndex shaderInputIndex, const Type& value)
        {
            m_shaderResourceGroup->SetConstant(shaderInputIndex, value);
        }

        template<>
        void Material::SetShaderConstant<Vector3>(RHI::ShaderInputConstantIndex shaderInputIndex, const Vector3& value)
        {
            // Vector3 is actually 16 bytes, not 12, so ShaderResourceGroup::SetConstant won't work. We
            // have to pass the raw data instead.
            m_shaderResourceGroup->SetConstantRaw(shaderInputIndex, &value, 3 * sizeof(float));
        }

        template<>
        void Material::SetShaderConstant<Color>(RHI::ShaderInputConstantIndex shaderInputIndex, const Color& value)
        {
            auto transformedColor = AZ::RPI::TransformColor(value, ColorSpaceId::LinearSRGB, ColorSpaceId::ACEScg);

            // Color is special because it could map to either a float3 or a float4
            auto descriptor = m_shaderResourceGroup->GetLayout()->GetShaderInput(shaderInputIndex);
            if (descriptor.m_constantByteCount == 3 * sizeof(float))
            {
                m_shaderResourceGroup->SetConstantRaw(shaderInputIndex, &transformedColor, 3 * sizeof(float));
            }
            else
            {
                m_shaderResourceGroup->SetConstantRaw(shaderInputIndex, &transformedColor, 4 * sizeof(float));
            }
        }

        template<typename Type>
        bool Material::SetShaderOption([[maybe_unused]] ShaderOptionGroup& options, [[maybe_unused]] ShaderOptionIndex shaderOptionIndex, [[maybe_unused]] Type value)
        {
            AZ_Assert(false, "MaterialProperty is incorrectly mapped to a shader option. Data type is incompatible.");
            return false;
        }

        template<>
        bool Material::SetShaderOption<bool>(ShaderOptionGroup& options, ShaderOptionIndex shaderOptionIndex, bool value)
        {
            return options.SetValue(shaderOptionIndex, ShaderOptionValue{ value });
        }

        template<>
        bool Material::SetShaderOption<uint32_t>(ShaderOptionGroup& options, ShaderOptionIndex shaderOptionIndex, uint32_t value)
        {
            return options.SetValue(shaderOptionIndex, ShaderOptionValue{ value });
        }

        template<>
        bool Material::SetShaderOption<int32_t>(ShaderOptionGroup& options, ShaderOptionIndex shaderOptionIndex, int32_t value)
        {
            return options.SetValue(shaderOptionIndex, ShaderOptionValue{ value });
        }

        template<typename Type>
        bool Material::SetPropertyValue(MaterialPropertyIndex index, const Type& value)
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

            for(auto& outputId : propertyDescriptor->GetOutputConnections())
            {
                if (outputId.m_type == MaterialPropertyOutputType::ShaderInput)
                {
                    if (propertyDescriptor->GetDataType() == MaterialPropertyDataType::Image)
                    {
                        const Data::Instance<Image>& image = savedPropertyValue.GetValue<Data::Instance<Image>>();

                        RHI::ShaderInputImageIndex shaderInputIndex(outputId.m_itemIndex.GetIndex());
                        m_shaderResourceGroup->SetImage(shaderInputIndex, image);
                    }
                    else
                    {
                        RHI::ShaderInputConstantIndex shaderInputIndex(outputId.m_itemIndex.GetIndex());
                        SetShaderConstant(shaderInputIndex, value);
                    }
                }
                else if (outputId.m_type == MaterialPropertyOutputType::ShaderOption)
                {
                    ShaderCollection::Item& shaderReference = m_shaderCollection[outputId.m_containerIndex.GetIndex()];
                    if (!SetShaderOption(*shaderReference.GetShaderOptions(), ShaderOptionIndex{outputId.m_itemIndex.GetIndex()}, value))
                    {
                        return false;
                    }
                }
                else
                {
                    AZ_Assert(false, "Unhandled MaterialPropertyOutputType");
                    return false;
                }
            }

            ++m_currentChangeId;

            return true;
        }

        template<>
        bool Material::SetPropertyValue<Data::Asset<ImageAsset>>(MaterialPropertyIndex index, const Data::Asset<ImageAsset>& value)
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
                    AZ_Error(s_debugTraceName, false, "Unsupported image asset type: %s", assetType.ToString<AZStd::string>().c_str());
                    return false;
                }

                if (!image)
                {
                    AZ_Error(s_debugTraceName, false, "Image asset could not be loaded");
                    return false;
                }

                return SetPropertyValue(index, image);
            }
        }

        // Using explicit instantiation to restrict SetPropertyValue to the set of types that we support

        template bool Material::SetPropertyValue<bool>     (MaterialPropertyIndex index, const bool&     value);
        template bool Material::SetPropertyValue<int32_t>  (MaterialPropertyIndex index, const int32_t&  value);
        template bool Material::SetPropertyValue<uint32_t> (MaterialPropertyIndex index, const uint32_t& value);
        template bool Material::SetPropertyValue<float>    (MaterialPropertyIndex index, const float&    value);
        template bool Material::SetPropertyValue<Vector2>  (MaterialPropertyIndex index, const Vector2&  value);
        template bool Material::SetPropertyValue<Vector3>  (MaterialPropertyIndex index, const Vector3&  value);
        template bool Material::SetPropertyValue<Vector4>  (MaterialPropertyIndex index, const Vector4&  value);
        template bool Material::SetPropertyValue<Color>    (MaterialPropertyIndex index, const Color&    value);
        template bool Material::SetPropertyValue<Data::Instance<Image>> (MaterialPropertyIndex index, const Data::Instance<Image>& value);

        bool Material::SetPropertyValue(MaterialPropertyIndex propertyIndex, const MaterialPropertyValue& value)
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
        const Type& Material::GetPropertyValue(MaterialPropertyIndex index) const
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

        template const bool&     Material::GetPropertyValue<bool>     (MaterialPropertyIndex index) const;
        template const int32_t&  Material::GetPropertyValue<int32_t>  (MaterialPropertyIndex index) const;
        template const uint32_t& Material::GetPropertyValue<uint32_t> (MaterialPropertyIndex index) const;
        template const float&    Material::GetPropertyValue<float>    (MaterialPropertyIndex index) const;
        template const Vector2&  Material::GetPropertyValue<Vector2>  (MaterialPropertyIndex index) const;
        template const Vector3&  Material::GetPropertyValue<Vector3>  (MaterialPropertyIndex index) const;
        template const Vector4&  Material::GetPropertyValue<Vector4>  (MaterialPropertyIndex index) const;
        template const Color&    Material::GetPropertyValue<Color>    (MaterialPropertyIndex index) const;
        template const Data::Instance<Image>& Material::GetPropertyValue<Data::Instance<Image>>(MaterialPropertyIndex index) const;

        const MaterialPropertyFlags& Material::GetPropertyDirtyFlags() const
        {
            return m_propertyDirtyFlags;
        }

        RHI::ConstPtr<MaterialPropertiesLayout> Material::GetMaterialPropertiesLayout() const
        {
            return m_layout;
        }
    } // namespace RPI
} // namespace AZ
