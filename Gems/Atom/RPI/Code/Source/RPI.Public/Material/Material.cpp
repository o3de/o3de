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
            
            // Copy the shader collections because the material will make changes, like updating the ShaderVariantId.
            m_shaderCollections = materialAsset.GetShaderCollections();

            if (!m_materialProperties.Init(m_materialAsset->GetMaterialPropertiesLayout(), materialAsset.GetPropertyValues()))
            {
                return RHI::ResultCode::Fail;
            }

            m_materialProperties.SetAllPropertyDirtyFlags();

            // Register for update events related to Shader instances that own the ShaderAssets inside
            // the shader collection.
            ShaderReloadNotificationBus::MultiHandler::BusDisconnect();
            for (const auto& shaderCollectionPair : m_shaderCollections)
            {
                for (const auto& shaderItem : shaderCollectionPair.second)
                {
                    ShaderReloadDebugTracker::Printf("(Material has ShaderAsset %p)", shaderItem.GetShaderAsset().Get());
                    ShaderReloadNotificationBus::MultiHandler::BusConnect(shaderItem.GetShaderAsset().GetId());
                }
            }


            // Usually SetProperties called above will increment this change ID to invalidate
            // the material, but some materials might not have any properties, and we need
            // the material to be invalidated particularly when hot-reloading.
            ++m_currentChangeId;

            Compile();

            return RHI::ResultCode::Success;
        }

        Material::~Material()
        {
            ShaderReloadNotificationBus::MultiHandler::BusDisconnect();
        }

        const MaterialPipelineShaderCollections& Material::GetShaderCollections() const
        {
            return m_shaderCollections;
        }

        const ShaderCollection& Material::GetShaderCollection(const Name& forPipeline) const
        {
            auto iter = m_shaderCollections.find(forPipeline);
            if (iter == m_shaderCollections.end())
            {
                static ShaderCollection EmptyShaderCollection;
                return EmptyShaderCollection;
            }

            return iter->second;
        }

        AZ::Outcome<uint32_t> Material::SetSystemShaderOption(const Name& shaderOptionName, RPI::ShaderOptionValue value)
        {
            uint32_t appliedCount = 0;

            // We won't set any shader options if the shader option is owned by any of the other shaders in this material.
            // If the material uses an option in any shader, then it owns that option for all its shaders.
            for (const auto& shaderCollectionPair : m_shaderCollections)
            {
                for (const auto& shaderItem : shaderCollectionPair.second)
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
            }

            for (auto& shaderCollectionPair : m_shaderCollections)
            {
                for (auto& shaderItem : shaderCollectionPair.second)
                {
                    const ShaderOptionGroupLayout* layout = shaderItem.GetShaderOptions()->GetShaderOptionLayout();
                    ShaderOptionIndex index = layout->FindShaderOptionIndex(shaderOptionName);
                    if (index.IsValid())
                    {
                        shaderItem.GetShaderOptions()->SetValue(index, value);
                        appliedCount++;
                    }
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

        const MaterialPropertyCollection& Material::GetPropertyCollection() const
        {
            return m_materialProperties;
        }

        const MaterialPropertyValue& Material::GetPropertyValue(MaterialPropertyIndex index) const
        {
            return m_materialProperties.GetPropertyValue(index);
        }

        const AZStd::vector<MaterialPropertyValue>& Material::GetPropertyValues() const
        {
            return m_materialProperties.GetPropertyValues();
        }

        bool Material::NeedsCompile() const
        {
            return m_compiledChangeId != m_currentChangeId;
        }

        void Material::ProcessDirectConnections()
        {
            AZ_PROFILE_SCOPE(RPI, "Material::ProcessDirectConnections()");

            for (size_t i = 0; i < m_materialProperties.GetMaterialPropertiesLayout()->GetPropertyCount(); ++i)
            {
                if (!m_materialProperties.GetPropertyDirtyFlags()[i])
                {
                    continue;
                }

                MaterialPropertyIndex propertyIndex{i};

                const MaterialPropertyValue value = m_materialProperties.GetPropertyValue(propertyIndex);

                const MaterialPropertyDescriptor* propertyDescriptor =
                    m_materialProperties.GetMaterialPropertiesLayout()->GetPropertyDescriptor(propertyIndex);

                for(auto& outputId : propertyDescriptor->GetOutputConnections())
                {
                    if (outputId.m_type == MaterialPropertyOutputType::ShaderInput)
                    {
                        if (propertyDescriptor->GetDataType() == MaterialPropertyDataType::Image)
                        {
                            const Data::Instance<Image>& image = value.GetValue<Data::Instance<Image>>();

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
                        ShaderCollection::Item& shaderReference = m_shaderCollections[outputId.m_materialPipelineName][outputId.m_containerIndex.GetIndex()];
                        SetShaderOption(*shaderReference.GetShaderOptions(), ShaderOptionIndex{outputId.m_itemIndex.GetIndex()}, value);
                    }
                    else if (outputId.m_type == MaterialPropertyOutputType::ShaderEnabled)
                    {
                        ShaderCollection::Item& shaderReference = m_shaderCollections[outputId.m_materialPipelineName][outputId.m_containerIndex.GetIndex()];
                        if (value.Is<bool>())
                        {
                            shaderReference.SetEnabled(value.GetValue<bool>());
                        }
                        else
                        {
                            // We should never get here because MaterialTypeAssetCreator and ValidatePropertyAccess ensure savedPropertyValue is a bool.
                            AZ_Assert(false, "Unsupported data type for MaterialPropertyOutputType::ShaderEnabled");
                        }
                    }
                    else
                    {
                        AZ_Assert(false, "Unhandled MaterialPropertyOutputType");
                    }
                }
            }
        }

        void Material::ProcessMaterialFunctors()
        {
            AZ_PROFILE_SCOPE(RPI, "Material::ProcessMaterialFunctors()");

            // On some platforms, PipelineStateObjects must be pre-compiled and shipped with the game; they cannot be compiled at runtime. So at some
            // point the material system needs to be smart about when it allows PSO changes and when it doesn't. There is a task scheduled to
            // thoroughly address this in 2022, but for now we just report a warning to alert users who are using the engine in a way that might
            // not be supported for much longer. PSO changes should only be allowed in developer tools (though we could also expose a way for users to
            // enable dynamic PSO changes if their project only targets platforms that support this).
            // PSO modifications are allowed during initialization because that's using the stored asset data, which the asset system can
            // access to pre-compile the necessary PSOs.
            MaterialPropertyPsoHandling psoHandling = m_isInitializing ? MaterialPropertyPsoHandling::Allowed : m_psoHandling;

            for (const Ptr<MaterialFunctor>& functor : m_materialAsset->GetMaterialFunctors())
            {
                if (functor)
                {
                    const MaterialPropertyFlags& materialPropertyDependencies = functor->GetMaterialPropertyDependencies();
                    // None covers case that the client code doesn't register material properties to dependencies,
                    // which will later get caught in Process() when trying to access a property.
                    if (materialPropertyDependencies.none() || functor->NeedsProcess(m_materialProperties.GetPropertyDirtyFlags()))
                    {
                        MaterialFunctorAPI::RuntimeContext processContext = MaterialFunctorAPI::RuntimeContext(
                            m_materialProperties,
                            &materialPropertyDependencies,
                            psoHandling,
                            m_shaderResourceGroup.get(),
                            &m_shaderCollections
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
                ProcessDirectConnections();
                ProcessMaterialFunctors();

                m_materialProperties.ClearAllPropertyDirtyFlags();

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

            MaterialPropertyIndex index = m_materialProperties.GetMaterialPropertiesLayout()->FindPropertyIndex(propertyId);
            if (!index.IsValid())
            {
                Name renamedId = propertyId;
                
                if (m_materialAsset->GetMaterialTypeAsset()->ApplyPropertyRenames(renamedId))
                {                                
                    index = m_materialProperties.GetMaterialPropertiesLayout()->FindPropertyIndex(renamedId);

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

        bool Material::SetShaderConstant(RHI::ShaderInputConstantIndex shaderInputIndex, const MaterialPropertyValue& value)
        {
            if (!value.IsValid())
            {
                AZ_Assert(false, "Empty value found for shader input index %u", shaderInputIndex.GetIndex());
                return false;
            }
            else if (value.Is<bool>())
            {
                return m_shaderResourceGroup->SetConstant(shaderInputIndex, value.GetValue<bool>());
            }
            else if (value.Is<int32_t>())
            {
                return m_shaderResourceGroup->SetConstant(shaderInputIndex, value.GetValue<int32_t>());
            }
            else if (value.Is<uint32_t>())
            {
                return m_shaderResourceGroup->SetConstant(shaderInputIndex, value.GetValue<uint32_t>());
            }
            else if (value.Is<float>())
            {
                return m_shaderResourceGroup->SetConstant(shaderInputIndex, value.GetValue<float>());
            }
            else if (value.Is<Vector2>())
            {
                return m_shaderResourceGroup->SetConstant(shaderInputIndex, value.GetValue<Vector2>());
            }
            else if (value.Is<Vector3>())
            {
                // Vector3 is actually 16 bytes, not 12, so ShaderResourceGroup::SetConstant won't work. We
                // have to pass the raw data instead.
                return m_shaderResourceGroup->SetConstantRaw(shaderInputIndex, &value.GetValue<Vector3>(), 3 * sizeof(float));
            }
            else if (value.Is<Vector4>())
            {
                return m_shaderResourceGroup->SetConstant(shaderInputIndex, value.GetValue<Vector4>());
            }
            else if (value.Is<Color>())
            {
                auto transformedColor = AZ::RPI::TransformColor(value.GetValue<Color>(), ColorSpaceId::LinearSRGB, ColorSpaceId::ACEScg);

                // Color is special because it could map to either a float3 or a float4
                auto descriptor = m_shaderResourceGroup->GetLayout()->GetShaderInput(shaderInputIndex);
                if (descriptor.m_constantByteCount == 3 * sizeof(float))
                {
                    return m_shaderResourceGroup->SetConstantRaw(shaderInputIndex, &transformedColor, 3 * sizeof(float));
                }
                else
                {
                    return m_shaderResourceGroup->SetConstantRaw(shaderInputIndex, &transformedColor, 4 * sizeof(float));
                }
            }
            else if (value.Is<Data::Instance<Image>>())
            {
                return m_shaderResourceGroup->SetConstant(shaderInputIndex, value.GetValue<Data::Instance<Image>>());
            }
            else if (value.Is<Data::Asset<ImageAsset>>())
            {
                return m_shaderResourceGroup->SetConstant(shaderInputIndex, value.GetValue<Data::Asset<ImageAsset>>());
            }
            else
            {
                AZ_Assert(false, "Unhandled material property value type");
                return false;
            }
        }

        bool Material::SetShaderOption(ShaderOptionGroup& options, ShaderOptionIndex shaderOptionIndex, const MaterialPropertyValue& value)
        {
            if (!value.IsValid())
            {
                AZ_Assert(false, "Empty value found for shader option %u", shaderOptionIndex.GetIndex());
                return false;
            }
            else if (value.Is<bool>())
            {
                return options.SetValue(shaderOptionIndex, ShaderOptionValue{value.GetValue<bool>()});
            }
            else if (value.Is<int32_t>())
            {
                return options.SetValue(shaderOptionIndex, ShaderOptionValue{value.GetValue<int32_t>()});
            }
            else if (value.Is<uint32_t>())
            {
                return options.SetValue(shaderOptionIndex, ShaderOptionValue{value.GetValue<uint32_t>()});
            }
            else
            {
                AZ_Assert(false, "MaterialProperty is incorrectly mapped to a shader option. Data type is incompatible.");
                return false;
            }
        }

        template<typename Type>
        bool Material::SetPropertyValue(MaterialPropertyIndex index, const Type& value)
        {
            bool success = m_materialProperties.SetPropertyValue<Type>(index, value);

            if (success)
            {
                ++m_currentChangeId;
            }

            return success;
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
            bool success = m_materialProperties.SetPropertyValue(propertyIndex, value);

            if (success)
            {
                ++m_currentChangeId;
            }

            return success;
        }

        template<typename Type>
        const Type& Material::GetPropertyValue(MaterialPropertyIndex index) const
        {
            return m_materialProperties.GetPropertyValue<Type>(index);
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
            return m_materialProperties.GetPropertyDirtyFlags();
        }

        RHI::ConstPtr<MaterialPropertiesLayout> Material::GetMaterialPropertiesLayout() const
        {
            return m_materialProperties.GetMaterialPropertiesLayout();
        }

    } // namespace RPI
} // namespace AZ
