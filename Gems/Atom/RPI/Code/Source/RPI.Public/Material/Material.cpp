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

            // All of these members must be reset if the material can be reinitialized because of the shader reload notification bus
            m_shaderResourceGroup = {};
            m_rhiShaderResourceGroup = {};
            m_materialProperties = {};
            m_generalShaderCollection = {};
            m_materialPipelineData = {};
            m_materialAsset = { &materialAsset, AZ::Data::AssetLoadBehavior::PreLoad };

            ShaderReloadNotificationBus::MultiHandler::BusDisconnect();
            if (!m_materialAsset.IsReady())
            {
                AZ_Error(s_debugTraceName, false, "Material::Init failed because shader asset is not ready. materialAsset uuid=%s",
                    materialAsset.GetId().ToFixedString().c_str());
                return RHI::ResultCode::Fail;
            }

            if (!m_materialAsset->InitializeNonSerializedData())
            {
                AZ_Error(s_debugTraceName, false, "Material::InitializeNonSerializedData is not supposed to fail. materialAsset uuid=%s",
                    materialAsset.GetId().ToFixedString().c_str());
                return RHI::ResultCode::Fail;
            }

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

            m_generalShaderCollection = m_materialAsset->GetGeneralShaderCollection();

            if (!m_materialProperties.Init(m_materialAsset->GetMaterialPropertiesLayout(), m_materialAsset->GetPropertyValues()))
            {
                return RHI::ResultCode::Fail;
            }

            m_materialProperties.SetAllPropertyDirtyFlags();

            for (auto& [materialPipelineName, materialPipeline] : m_materialAsset->GetMaterialPipelinePayloads())
            {
                MaterialPipelineState& pipelineData = m_materialPipelineData[materialPipelineName];

                pipelineData.m_shaderCollection = materialPipeline.m_shaderCollection;

                if (!pipelineData.m_materialProperties.Init(materialPipeline.m_materialPropertiesLayout, materialPipeline.m_defaultPropertyValues))
                {
                    return RHI::ResultCode::Fail;
                }

                pipelineData.m_materialProperties.SetAllPropertyDirtyFlags();
            }

            // Register for update events related to Shader instances that own the ShaderAssets inside
            // the shader collection.
            ForAllShaderItems([this](const Name&, const ShaderCollection::Item& shaderItem)
                {
                    ShaderReloadDebugTracker::Printf("(Material has ShaderAsset %p)", shaderItem.GetShaderAsset().Get());
                    ShaderReloadNotificationBus::MultiHandler::BusConnect(shaderItem.GetShaderAsset().GetId());
                    return true;
                });

            // Usually SetProperties called above will increment this change ID to invalidate
            // the material, but some materials might not have any properties, and we need
            // the material to be invalidated particularly when hot-reloading.
            ++m_currentChangeId;

            Compile();

            return RHI::ResultCode::Success;
        }

        Material::Material() = default;

        Material::~Material()
        {
            ShaderReloadNotificationBus::MultiHandler::BusDisconnect();
        }

        const ShaderCollection& Material::GetGeneralShaderCollection() const
        {
            return m_generalShaderCollection;
        }

        const ShaderCollection& Material::GetShaderCollection(const Name& forPipeline) const
        {
            auto iter = m_materialPipelineData.find(forPipeline);
            if (iter == m_materialPipelineData.end())
            {
                static ShaderCollection EmptyShaderCollection;
                return EmptyShaderCollection;
            }

            return iter->second.m_shaderCollection;
        }
        
        void Material::ForAllShaderItemsWriteable(AZStd::function<bool(ShaderCollection::Item& shaderItem)> callback)
        {
            for (auto& shaderItem : m_generalShaderCollection)
            {
                if (!callback(shaderItem))
                {
                    return;
                }
            }
            for (auto& materialPipelinePair : m_materialPipelineData)
            {
                for (auto& shaderItem : materialPipelinePair.second.m_shaderCollection)
                {
                    if (!callback(shaderItem))
                    {
                        return;
                    }
                }
            }
        }

        void Material::ForAllShaderItems(AZStd::function<bool(const Name& materialPipelineName, const ShaderCollection::Item& shaderItem)> callback) const
        {
            for (const auto& shaderItem : m_generalShaderCollection)
            {
                if (!callback(MaterialPipelineNone, shaderItem))
                {
                    return;
                }
            }
            for (const auto& [materialPipelineName, materialPipeline] : m_materialPipelineData)
            {
                for (const auto& shaderItem : materialPipeline.m_shaderCollection)
                {
                    if (!callback(materialPipelineName, shaderItem))
                    {
                        return;
                    }
                }
            }
        }

        bool Material::MaterialOwnsShaderOption(const Name& shaderOptionName) const
        {
            bool isOwned = false;

            // We won't set any shader options if the shader option is owned by any of the other shaders in this material.
            // If the material uses an option in any shader, then it owns that option for all its shaders.
            ForAllShaderItems([&](const Name&, const ShaderCollection::Item& shaderItem)
                {
                    const ShaderOptionGroupLayout* layout = shaderItem.GetShaderOptions()->GetShaderOptionLayout();
                    ShaderOptionIndex index = layout->FindShaderOptionIndex(shaderOptionName);
                    if (index.IsValid())
                    {
                        if (shaderItem.MaterialOwnsShaderOption(index))
                        {
                            isOwned = true;
                            return false; // We can stop searching now
                        }
                    }

                    return true; // Continue
                });

            return isOwned;
        }

        AZ::Outcome<uint32_t> Material::SetSystemShaderOption(const Name& shaderOptionName, RPI::ShaderOptionValue value)
        {
            uint32_t appliedCount = 0;

            if (MaterialOwnsShaderOption(shaderOptionName))
            {
                return AZ::Failure();
            }

            ForAllShaderItemsWriteable([&](ShaderCollection::Item& shaderItem)
                {
                    const ShaderOptionGroupLayout* layout = shaderItem.GetShaderOptions()->GetShaderOptionLayout();
                    ShaderOptionIndex index = layout->FindShaderOptionIndex(shaderOptionName);
                    if (index.IsValid())
                    {
                        shaderItem.GetShaderOptions()->SetValue(index, value);
                        appliedCount++;
                    }
                    return true;
                });

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
            // On some platforms, PipelineStateObjects must be pre-compiled and shipped with the game; they cannot be compiled at runtime. So at some
            // point the material system needs to be smart about when it allows PSO changes and when it doesn't. There is a task scheduled to
            // thoroughly address this in 2022, but for now we just report a warning to alert users who are using the engine in a way that might
            // not be supported for much longer. PSO changes should only be allowed in developer tools (though we could also expose a way for users to
            // enable dynamic PSO changes if their project only targets platforms that support this).
            // PSO modifications are allowed during initialization because that's using the stored asset data, which the asset system can
            // access to pre-compile the necessary PSOs.

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
            return m_materialAsset.IsReady() && (!m_shaderResourceGroup || !m_shaderResourceGroup->IsQueuedForCompile());
        }

        ///////////////////////////////////////////////////////////////////
        // ShaderReloadNotificationBus overrides...
        void Material::OnShaderReinitialized([[maybe_unused]] const Shader& shader)
        {
            ShaderReloadDebugTracker::ScopedSection reloadSection("{%p}->Material::OnShaderReinitialized %s", this, shader.GetAsset().GetHint().c_str());
            // Note that it might not be strictly necessary to reinitialize the entire material, we might be able to get away with
            // just bumping the m_currentChangeId or some other minor updates. But it's pretty hard to know what exactly needs to be
            // updated to correctly handle the reload, so it's safer to just reinitialize the whole material.
            ReInitKeepPropertyValues();
        }

        void Material::OnShaderAssetReinitialized(const Data::Asset<ShaderAsset>& shaderAsset)
        {
            ShaderReloadDebugTracker::ScopedSection reloadSection("{%p}->Material::OnShaderAssetReinitialized %s", this, shaderAsset.GetHint().c_str());
            // Note that it might not be strictly necessary to reinitialize the entire material, we might be able to get away with
            // just bumping the m_currentChangeId or some other minor updates. But it's pretty hard to know what exactly needs to be
            // updated to correctly handle the reload, so it's safer to just reinitialize the whole material.
            ReInitKeepPropertyValues();
        }

        void Material::OnShaderVariantReinitialized(const ShaderVariant& shaderVariant)
        {
            ShaderReloadDebugTracker::ScopedSection reloadSection("{%p}->Material::OnShaderVariantReinitialized %s", this, shaderVariant.GetShaderVariantAsset().GetHint().c_str());

            // Move m_shaderVariantReadyEvent to a local AZ::Event in order to allow
            // the handlers to be signaled outside of the mutex lock.
            // This allows other threads to register their handlers while this thread
            // is invoking Signal() on the current snapshot of handlers.
            decltype(m_shaderVariantReadyEvent) localShaderVariantReadyEvent;
            {
                AZStd::scoped_lock lock(m_shaderVariantReadyEventMutex);
                localShaderVariantReadyEvent = AZStd::move(m_shaderVariantReadyEvent);
            }

            // Note: we don't need to re-compile the material if a shader variant is ready or changed
            // The DrawPacket created for the material need to be updated since the PSO need to be re-creaed.
            // This event can be used to notify the owners to update their DrawPackets.
            localShaderVariantReadyEvent.Signal();

            // Finally restore m_shaderVariantReadyEvent but making sure to claim any new handlers that were added
            // in other threads while Signal() was being called.
            {
                // Swap the local handlers with the current m_notifiers which
                // will contain any handlers added during the signaling of the
                // local event
                AZStd::scoped_lock lock(m_shaderVariantReadyEventMutex);
                AZStd::swap(m_shaderVariantReadyEvent, localShaderVariantReadyEvent);
                // Append any added handlers to the m_notifier structure
                m_shaderVariantReadyEvent.ClaimHandlers(AZStd::move(localShaderVariantReadyEvent));
            }
        }

        void Material::ReInitKeepPropertyValues()
        {
            // Save the material property values to be reapplied after reinitialization. The mapping is stored by name in case the property
            // layout changes after reinitialization.
            AZStd::unordered_map<AZ::Name, MaterialPropertyValue> properties;
            properties.reserve(GetMaterialPropertiesLayout()->GetPropertyCount());
            for (size_t propertyIndex = 0; propertyIndex < GetMaterialPropertiesLayout()->GetPropertyCount(); ++propertyIndex)
            {
                auto descriptor = GetMaterialPropertiesLayout()->GetPropertyDescriptor(AZ::RPI::MaterialPropertyIndex{ propertyIndex });
                properties.emplace(descriptor->GetName(), GetPropertyValue(AZ::RPI::MaterialPropertyIndex{ propertyIndex }));
            }

            if (Init(*m_materialAsset) == RHI::ResultCode::Success)
            {
                for (const auto& [propertyName, propertyValue] : properties)
                {
                    if (const auto& propertyIndex = GetMaterialPropertiesLayout()->FindPropertyIndex(propertyName); propertyIndex.IsValid())
                    {
                        SetPropertyValue(propertyIndex, propertyValue);
                    }
                }
                Compile();
            }
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

        void Material::ConnectEvent(OnMaterialShaderVariantReadyEvent::Handler& handler)
        {
            AZStd::scoped_lock lock(m_shaderVariantReadyEventMutex);
            handler.Connect(m_shaderVariantReadyEvent);
        }

        bool Material::TryApplyPropertyConnectionToShaderInput(
            const MaterialPropertyValue& value,
            const MaterialPropertyOutputId& connection,
            const MaterialPropertyDescriptor* propertyDescriptor)
        {
            if (connection.m_type == MaterialPropertyOutputType::ShaderInput)
            {
                if (propertyDescriptor->GetDataType() == MaterialPropertyDataType::Image)
                {
                    const Data::Instance<Image>& image = value.GetValue<Data::Instance<Image>>();

                    RHI::ShaderInputImageIndex shaderInputIndex(connection.m_itemIndex.GetIndex());
                    m_shaderResourceGroup->SetImage(shaderInputIndex, image);
                }
                else
                {
                    RHI::ShaderInputConstantIndex shaderInputIndex(connection.m_itemIndex.GetIndex());
                    SetShaderConstant(shaderInputIndex, value);
                }

                return true;
            }

            return false;
        }

        bool Material::TryApplyPropertyConnectionToShaderOption(
            const MaterialPropertyValue& value,
            const MaterialPropertyOutputId& connection)
        {
            if (connection.m_type == MaterialPropertyOutputType::ShaderOption)
            {
                ShaderCollection::Item* shaderReference = nullptr;

                if (connection.m_materialPipelineName.IsEmpty())
                {
                    shaderReference = &m_generalShaderCollection[connection.m_containerIndex.GetIndex()];
                }
                else
                {
                    shaderReference = &m_materialPipelineData[connection.m_materialPipelineName].m_shaderCollection[connection.m_containerIndex.GetIndex()];
                }

                SetShaderOption(*shaderReference->GetShaderOptions(), ShaderOptionIndex{connection.m_itemIndex.GetIndex()}, value);

                return true;
            }

            return false;
        }

        bool Material::TryApplyPropertyConnectionToShaderEnable(
            const MaterialPropertyValue& value,
            const MaterialPropertyOutputId& connection)
        {
            if (connection.m_type == MaterialPropertyOutputType::ShaderEnabled)
            {
                ShaderCollection::Item* shaderReference = nullptr;

                if (!value.Is<bool>())
                {
                    // We should never get here because MaterialTypeAssetCreator and MaterialPropertyCollection::ValidatePropertyAccess ensure the value is a bool.
                    AZ_Assert(false, "Unsupported data type for MaterialPropertyOutputType::ShaderEnabled");
                    return false;
                }

                if (connection.m_materialPipelineName.IsEmpty())
                {
                    shaderReference = &m_generalShaderCollection[connection.m_containerIndex.GetIndex()];
                }
                else
                {
                    shaderReference = &m_materialPipelineData[connection.m_materialPipelineName].m_shaderCollection[connection.m_containerIndex.GetIndex()];
                }

                shaderReference->SetEnabled(value.GetValue<bool>());

                return true;
            }

            return false;
        }

        bool Material::TryApplyPropertyConnectionToInternalProperty(
            const MaterialPropertyValue& value,
            const MaterialPropertyOutputId& connection)
        {
            if (connection.m_type == MaterialPropertyOutputType::InternalProperty)
            {
                m_materialPipelineData[connection.m_materialPipelineName].m_materialProperties.SetPropertyValue(MaterialPropertyIndex{connection.m_itemIndex.GetIndex()}, value);
                return true;
            }

            return false;
        }

        void Material::ProcessDirectConnections()
        {
            AZ_PROFILE_SCOPE(RPI, "Process direct connection");

            // Apply any changes to *main* material properties...

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

                for (const MaterialPropertyOutputId& connection : propertyDescriptor->GetOutputConnections())
                {
                    [[maybe_unused]] bool applied =
                        TryApplyPropertyConnectionToShaderInput(value, connection, propertyDescriptor) ||
                        TryApplyPropertyConnectionToShaderOption(value, connection) ||
                        TryApplyPropertyConnectionToShaderEnable(value, connection) ||
                        TryApplyPropertyConnectionToInternalProperty(value, connection);

                    AZ_Error(s_debugTraceName, applied, "Connections of type %s are not supported by material properties.", ToString(connection.m_type));
                }
            }
        }

        void Material::ProcessInternalDirectConnections()
        {
            AZ_PROFILE_SCOPE(RPI, "Process direct connection");

            // Apply any changes to *internal* material properties...

            for (auto& materialPipelinePair : m_materialPipelineData)
            {
                MaterialPropertyCollection& pipelineProperties = materialPipelinePair.second.m_materialProperties;
                const MaterialPropertiesLayout& pipelinePropertiesLayout = *materialPipelinePair.second.m_materialProperties.GetMaterialPropertiesLayout();

                for (size_t i = 0; i < pipelinePropertiesLayout.GetPropertyCount(); ++i)
                {
                    if (!materialPipelinePair.second.m_materialProperties.GetPropertyDirtyFlags()[i])
                    {
                        continue;
                    }

                    MaterialPropertyIndex propertyIndex{i};

                    const MaterialPropertyValue value = pipelineProperties.GetPropertyValue(propertyIndex);
                    const MaterialPropertyDescriptor* propertyDescriptor = pipelinePropertiesLayout.GetPropertyDescriptor(propertyIndex);

                    for (const MaterialPropertyOutputId& connection : propertyDescriptor->GetOutputConnections())
                    {
                        // Note that ShaderInput is not supported for internal properties. Internal properties are used exclusively for the
                        // .materialpipeline which is not allowed to access to the MaterialSrg, only the .materialtype should know about the MaterialSrg.
                        [[maybe_unused]] bool applied =
                            TryApplyPropertyConnectionToShaderOption(value, connection) ||
                            TryApplyPropertyConnectionToShaderEnable(value, connection);

                        AZ_Error(s_debugTraceName, applied, "Connections of type %s are not supported by material pipeline properties.", ToString(connection.m_type));
                    }
                }
            }
        }

        void Material::ProcessMaterialFunctors()
        {
            AZ_PROFILE_SCOPE(RPI, "Process material functors");

            MaterialPropertyPsoHandling psoHandling = m_isInitializing ? MaterialPropertyPsoHandling::Allowed : m_psoHandling;

            // First run the "main" MaterialPipelineNone functors, which use the MaterialFunctorAPI::RuntimeContext
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
                            &m_generalShaderCollection,
                            &m_materialPipelineData
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

        void Material::ProcessInternalMaterialFunctors()
        {
            AZ_PROFILE_SCOPE(RPI, "Process material functors");

            MaterialPropertyPsoHandling psoHandling = m_isInitializing ? MaterialPropertyPsoHandling::Allowed : m_psoHandling;

            // Then run the "pipeline" functors, which use the MaterialFunctorAPI::PipelineRuntimeContext
            for (auto& [materialPipelineName, materialPipeline] : m_materialAsset->GetMaterialPipelinePayloads())
            {
                MaterialPipelineState& materialPipelineData = m_materialPipelineData[materialPipelineName];

                for (const Ptr<MaterialFunctor>& functor : materialPipeline.m_materialFunctors)
                {
                    if (functor)
                    {
                        const MaterialPropertyFlags& materialPropertyDependencies = functor->GetMaterialPropertyDependencies();
                        // None covers case that the client code doesn't register material properties to dependencies,
                        // which will later get caught in Process() when trying to access a property.
                        if (materialPropertyDependencies.none() || functor->NeedsProcess(materialPipelineData.m_materialProperties.GetPropertyDirtyFlags()))
                        {
                            MaterialFunctorAPI::PipelineRuntimeContext processContext = MaterialFunctorAPI::PipelineRuntimeContext(
                                materialPipelineData.m_materialProperties,
                                &materialPropertyDependencies,
                                psoHandling,
                                &materialPipelineData.m_shaderCollection
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

                ProcessInternalDirectConnections();
                ProcessInternalMaterialFunctors();

                m_materialProperties.ClearAllPropertyDirtyFlags();

                for (auto& materialPipelinePair : m_materialPipelineData)
                {
                    materialPipelinePair.second.m_materialProperties.ClearAllPropertyDirtyFlags();
                }

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

        template AZ_DLL_EXPORT bool Material::SetPropertyValue<bool>     (MaterialPropertyIndex index, const bool&     value);
        template AZ_DLL_EXPORT bool Material::SetPropertyValue<int32_t>  (MaterialPropertyIndex index, const int32_t&  value);
        template AZ_DLL_EXPORT bool Material::SetPropertyValue<uint32_t> (MaterialPropertyIndex index, const uint32_t& value);
        template AZ_DLL_EXPORT bool Material::SetPropertyValue<float>    (MaterialPropertyIndex index, const float&    value);
        template AZ_DLL_EXPORT bool Material::SetPropertyValue<Vector2>  (MaterialPropertyIndex index, const Vector2&  value);
        template AZ_DLL_EXPORT bool Material::SetPropertyValue<Vector3>  (MaterialPropertyIndex index, const Vector3&  value);
        template AZ_DLL_EXPORT bool Material::SetPropertyValue<Vector4>  (MaterialPropertyIndex index, const Vector4&  value);
        template AZ_DLL_EXPORT bool Material::SetPropertyValue<Color>    (MaterialPropertyIndex index, const Color&    value);
        template AZ_DLL_EXPORT bool Material::SetPropertyValue<Data::Instance<Image>> (MaterialPropertyIndex index, const Data::Instance<Image>& value);

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

        template AZ_DLL_EXPORT const bool&     Material::GetPropertyValue<bool>     (MaterialPropertyIndex index) const;
        template AZ_DLL_EXPORT const int32_t&  Material::GetPropertyValue<int32_t>  (MaterialPropertyIndex index) const;
        template AZ_DLL_EXPORT const uint32_t& Material::GetPropertyValue<uint32_t> (MaterialPropertyIndex index) const;
        template AZ_DLL_EXPORT const float&    Material::GetPropertyValue<float>    (MaterialPropertyIndex index) const;
        template AZ_DLL_EXPORT const Vector2&  Material::GetPropertyValue<Vector2>  (MaterialPropertyIndex index) const;
        template AZ_DLL_EXPORT const Vector3&  Material::GetPropertyValue<Vector3>  (MaterialPropertyIndex index) const;
        template AZ_DLL_EXPORT const Vector4&  Material::GetPropertyValue<Vector4>  (MaterialPropertyIndex index) const;
        template AZ_DLL_EXPORT const Color&    Material::GetPropertyValue<Color>    (MaterialPropertyIndex index) const;
        template AZ_DLL_EXPORT const Data::Instance<Image>& Material::GetPropertyValue<Data::Instance<Image>>(MaterialPropertyIndex index) const;
        
        const MaterialPropertyFlags& Material::GetPropertyDirtyFlags() const
        {
            return m_materialProperties.GetPropertyDirtyFlags();
        }

        RHI::ConstPtr<MaterialPropertiesLayout> Material::GetMaterialPropertiesLayout() const
        {
            return m_materialProperties.GetMaterialPropertiesLayout();
        }
        Data::Instance<RPI::ShaderResourceGroup> Material::GetShaderResourceGroup()
        {
            return m_shaderResourceGroup;
        }
    } // namespace RPI
} // namespace AZ
