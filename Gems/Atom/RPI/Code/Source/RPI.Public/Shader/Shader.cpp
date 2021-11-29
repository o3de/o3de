/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RPI.Public/Shader/Shader.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/PipelineStateCache.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <AtomCore/Instance/InstanceDatabase.h>
#include <Atom/RPI.Public/Shader/ShaderReloadDebugTracker.h>
#include <Atom/RPI.Public/Shader/ShaderSystemInterface.h>
#include <AzCore/Interface/Interface.h>

#include <AzCore/Component/TickBus.h>


namespace AZ
{
    namespace RPI
    {
        Data::Instance<Shader> Shader::FindOrCreate(const Data::Asset<ShaderAsset>& shaderAsset, const Name& supervariantName)
        {
            auto anySupervariantName = AZStd::any(supervariantName);
            Data::Instance<Shader> shaderInstance = Data::InstanceDatabase<Shader>::Instance().FindOrCreate(
                Data::InstanceId::CreateFromAssetId(shaderAsset.GetId()), shaderAsset, &anySupervariantName);

            if (shaderInstance)
            {
                // [GFX TODO][ATOM-15813] Change InstanceDatabase<Shader> to support multiple instances with different supervariants.
                // At this time we do not support multiple supervariants loaded for a shader asset simultaneously, so if this shader
                // is referring to the wrong supervariant we need to change it to the correct one.
                SupervariantIndex supervariantIndex = shaderAsset->GetSupervariantIndex(supervariantName);
                if (supervariantIndex.IsValid() && shaderInstance->GetSupervariantIndex() != supervariantIndex)
                {
                    shaderInstance->ChangeSupervariant(supervariantIndex);
                }
            }

            return shaderInstance;
        }

        Data::Instance<Shader> Shader::FindOrCreate(const Data::Asset<ShaderAsset>& shaderAsset)
        {
            return FindOrCreate(shaderAsset, AZ::Name { "" });
        }

        Data::Instance<Shader> Shader::CreateInternal([[maybe_unused]] ShaderAsset& shaderAsset, const AZStd::any* anySupervariantName)
        {
            AZ_Assert(anySupervariantName != nullptr, "Invalid supervariant name param");
            auto supervariantName = AZStd::any_cast<AZ::Name>(*anySupervariantName);
            auto supervariantIndex = shaderAsset.GetSupervariantIndex(supervariantName);
            if (!supervariantIndex.IsValid())
            {
                AZ_Error("Shader", false, "Supervariant with name %s, was not found in shader %s", supervariantName.GetCStr(), shaderAsset.GetName().GetCStr());
                return nullptr;
            }

            Data::Instance<Shader> shader = aznew Shader(supervariantIndex);
            const RHI::ResultCode resultCode = shader->Init(shaderAsset);
            if (resultCode != RHI::ResultCode::Success)
            {
                return nullptr;
            }
            return shader;
        }

        Shader::~Shader()
        {
            Shutdown();
        }

        static bool GetPipelineLibraryPath(char* pipelineLibraryPath, size_t pipelineLibraryPathLength, const ShaderAsset& shaderAsset)
        {
            if (auto* fileIOBase = IO::FileIOBase::GetInstance())
            {
                const Data::AssetId& assetId = shaderAsset.GetId();

                Name platformName = RHI::Factory::Get().GetName();
                Name shaderName = shaderAsset.GetName();

                AZStd::string uuidString;
                assetId.m_guid.ToString<AZStd::string>(uuidString, false, false);

                char pipelineLibraryPathTemp[AZ_MAX_PATH_LEN];
                azsnprintf(
                    pipelineLibraryPathTemp, AZ_MAX_PATH_LEN, "@user@/Atom/PipelineStateCache/%s/%s_%s_%d.bin", platformName.GetCStr(),
                    shaderName.GetCStr(), uuidString.data(), assetId.m_subId);

                fileIOBase->ResolvePath(pipelineLibraryPathTemp, pipelineLibraryPath, pipelineLibraryPathLength);
                return true;
            }
            return false;
        }

        RHI::ResultCode Shader::Init(ShaderAsset& shaderAsset)
        {
            Data::AssetBus::MultiHandler::BusDisconnect();
            ShaderVariantFinderNotificationBus::Handler::BusDisconnect();

            RHI::RHISystemInterface* rhiSystem = RHI::RHISystemInterface::Get();
            RHI::DrawListTagRegistry* drawListTagRegistry = rhiSystem->GetDrawListTagRegistry();

            m_asset = { &shaderAsset, AZ::Data::AssetLoadBehavior::PreLoad };
            m_pipelineStateType = shaderAsset.GetPipelineStateType();

            GetPipelineLibraryPath(m_pipelineLibraryPath, AZ_MAX_PATH_LEN, *m_asset);

            {
                AZStd::unique_lock<decltype(m_variantCacheMutex)> lock(m_variantCacheMutex);
                m_shaderVariants.clear();
            }
            auto rootShaderVariantAsset = shaderAsset.GetRootVariant(m_supervariantIndex);
            m_rootVariant.Init(m_asset, rootShaderVariantAsset, m_supervariantIndex);

            if (m_pipelineLibraryHandle.IsNull())
            {
                // We set up a pipeline library only once for the lifetime of the Shader instance.
                // This should allow the Shader to be reloaded at runtime many times, and cache and reuse PipelineState objects rather than rebuild them.
                // It also fixes a particular TDR crash that occurred on some hardware when hot-reloading shaders and building pipeline states
                // in a new pipeline library every time.

                RHI::PipelineStateCache* pipelineStateCache = rhiSystem->GetPipelineStateCache();
                ConstPtr<RHI::PipelineLibraryData> serializedData = LoadPipelineLibrary();
                RHI::PipelineLibraryHandle pipelineLibraryHandle = pipelineStateCache->CreateLibrary(serializedData.get());

                if (pipelineLibraryHandle.IsNull())
                {
                    AZ_Error("Shader", false, "Failed to create pipeline library from pipeline state cache.");
                    return RHI::ResultCode::Fail;
                }

                m_pipelineLibraryHandle = pipelineLibraryHandle;
                m_pipelineStateCache = pipelineStateCache;
            }

            const Name& drawListName = shaderAsset.GetDrawListName();
            if (!drawListName.IsEmpty())
            {
                m_drawListTag = drawListTagRegistry->AcquireTag(drawListName);
                if (!m_drawListTag.IsValid())
                {
                    AZ_Error("Shader", false, "Failed to acquire a DrawListTag. Entries are full.");
                }
            }

            ShaderVariantFinderNotificationBus::Handler::BusConnect(m_asset.GetId());
            Data::AssetBus::MultiHandler::BusConnect(rootShaderVariantAsset.GetId());
            Data::AssetBus::MultiHandler::BusConnect(m_asset.GetId());

            return RHI::ResultCode::Success;
        }

        void Shader::Shutdown()
        {
            ShaderVariantFinderNotificationBus::Handler::BusDisconnect();
            Data::AssetBus::MultiHandler::BusDisconnect();

            if (m_pipelineLibraryHandle.IsValid())
            {
                SavePipelineLibrary();

                m_pipelineStateCache->ReleaseLibrary(m_pipelineLibraryHandle);
                m_pipelineStateCache = nullptr;
                m_pipelineLibraryHandle = {};
            }

            if (m_drawListTag.IsValid())
            {
                RHI::DrawListTagRegistry* drawListTagRegistry = RHI::RHISystemInterface::Get()->GetDrawListTagRegistry();
                drawListTagRegistry->ReleaseTag(m_drawListTag);
                m_drawListTag.Reset();
            }
        }

        ///////////////////////////////////////////////////////////////////////
        // AssetBus overrides
        void Shader::OnAssetReloaded(Data::Asset<Data::AssetData> asset)
        {
            ShaderReloadDebugTracker::ScopedSection reloadSection("{%p}->Shader::OnAssetReloaded %s", this, asset.GetHint().c_str());

            if (asset.GetAs<ShaderVariantAsset>())
            {
                m_reloadedRootShaderVariantAsset = Data::static_pointer_cast<ShaderVariantAsset>(asset);
                if (m_asset->m_buildTimestamp == m_reloadedRootShaderVariantAsset->GetBuildTimestamp())
                {
                    Init(*m_asset.Get());
                    ShaderReloadNotificationBus::Event(asset.GetId(), &ShaderReloadNotificationBus::Events::OnShaderReinitialized, *this);
                }
                return;
            }

            if (asset.GetAs<ShaderAsset>())
            {
                m_asset = Data::static_pointer_cast<ShaderAsset>(asset);
                if (!m_reloadedRootShaderVariantAsset.IsReady())
                {
                    // Do nothing, as We should not re-initilize until the root shader variant asset has been reloaded.
                    return;
                }
                AZ_Assert(m_asset->m_buildTimestamp == m_reloadedRootShaderVariantAsset->GetBuildTimestamp(),
                    "shaderAsset '%s' timeStamp=%lld, but Root ShaderVariantAsset timeStamp=%lld", m_asset.GetHint().c_str(),
                    m_asset->m_buildTimestamp, m_reloadedRootShaderVariantAsset->GetBuildTimestamp());
                m_asset->UpdateRootShaderVariantAsset(m_supervariantIndex, m_reloadedRootShaderVariantAsset);
                m_reloadedRootShaderVariantAsset = {}; // Clear the temporary reference.

                if (ShaderReloadDebugTracker::IsEnabled())
                {
                    auto makeTimeString = [](AZ::u64 timestamp, AZ::u64 now)
                    {
                        AZ::u64 elapsedMillis = now - timestamp;
                        double elapsedSeconds = aznumeric_cast<double>(elapsedMillis / 1'000);
                        AZStd::string timeString = AZStd::string::format("%lld (%f seconds ago)", timestamp, elapsedSeconds);
                        return timeString;
                    };

                    AZStd::sys_time_t now = AZStd::GetTimeNowMicroSecond();

                    const auto shaderVariantAsset = m_asset->GetRootVariant();
                    ShaderReloadDebugTracker::Printf("{%p}->Shader::OnAssetReloaded for shader '%s' [build time %s] found variant '%s' [build time %s]", this,
                        m_asset.GetHint().c_str(), makeTimeString(m_asset->m_buildTimestamp, now).c_str(),
                        shaderVariantAsset.GetHint().c_str(), makeTimeString(shaderVariantAsset->GetBuildTimestamp(), now).c_str());
                }
                Init(*m_asset.Get());
                ShaderReloadNotificationBus::Event(asset.GetId(), &ShaderReloadNotificationBus::Events::OnShaderReinitialized, *this);
            }

        }
        ///////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////
        /// ShaderVariantFinderNotificationBus overrides
        void Shader::OnShaderVariantAssetReady(Data::Asset<ShaderVariantAsset> shaderVariantAsset, bool isError)
        {
            ShaderReloadDebugTracker::ScopedSection reloadSection("{%p}->Shader::OnShaderVariantAssetReady %s", this, shaderVariantAsset.GetHint().c_str());

            AZ_Assert(shaderVariantAsset, "Reloaded ShaderVariantAsset is null");
            const ShaderVariantStableId stableId = shaderVariantAsset->GetStableId();

            // We make a copy of the updated variant because OnShaderVariantReinitialized must not be called inside
            // m_variantCacheMutex or deadlocks may occur.
            // Or if there is an error, we leave this object in its default state to indicate there was an error.
            // [GFX TODO] We really should have a dedicated message/event for this, but that will be covered by a future task where
            // we will merge ShaderReloadNotificationBus messages into one. For now, we just indicate the error by passing an empty ShaderVariant,
            // all our call sites don't use this data anyway.
            ShaderVariant updatedVariant;

            if (isError)
            {
                //Remark: We do not assert if the stableId == RootShaderVariantStableId, because we can not trust in the asset data
                //on error. so it is possible that on error the stbleId == RootShaderVariantStableId;
                if (stableId == RootShaderVariantStableId)
                {
                    return;
                }
                AZStd::unique_lock<decltype(m_variantCacheMutex)> lock(m_variantCacheMutex);
                m_shaderVariants.erase(stableId);
            }
            else
            {
                AZ_Assert(stableId != RootShaderVariantStableId,
                    "The root variant is expected to be updated by the ShaderAsset.");
                AZStd::unique_lock<decltype(m_variantCacheMutex)> lock(m_variantCacheMutex);

                auto iter = m_shaderVariants.find(stableId);
                if (iter != m_shaderVariants.end())
                {
                    ShaderVariant& shaderVariant = iter->second;

                    if (!shaderVariant.Init(m_asset, shaderVariantAsset, m_supervariantIndex))
                    {
                        AZ_Error("Shader", false, "Failed to init shaderVariant with StableId=%u", shaderVariantAsset->GetStableId());
                        m_shaderVariants.erase(stableId);
                    }
                    else
                    {
                        updatedVariant = shaderVariant;
                    }
                }
                else
                {
                    //This is the first time the shader variant asset comes to life.
                    updatedVariant.Init(m_asset, shaderVariantAsset, m_supervariantIndex);
                    m_shaderVariants.emplace(stableId, updatedVariant);
                }
            }

            // [GFX TODO] It might make more sense to call OnShaderReinitialized here
            ShaderReloadNotificationBus::Event(m_asset.GetId(), &ShaderReloadNotificationBus::Events::OnShaderVariantReinitialized, updatedVariant);
        }
        ///////////////////////////////////////////////////////////////////
        
        ConstPtr<RHI::PipelineLibraryData> Shader::LoadPipelineLibrary() const
        { 
            if (m_pipelineLibraryPath[0] != 0)
            {
                return Utils::LoadObjectFromFile<RHI::PipelineLibraryData>(m_pipelineLibraryPath);
            }
            return nullptr;
        }

        void Shader::SavePipelineLibrary() const
        {
            if (m_pipelineLibraryPath[0] != 0)
            {
                RHI::ConstPtr<RHI::PipelineLibraryData> serializedData = m_pipelineStateCache->GetLibrarySerializedData(m_pipelineLibraryHandle);
                if (serializedData)
                {
                    Utils::SaveObjectToFile<RHI::PipelineLibraryData>(m_pipelineLibraryPath, DataStream::ST_BINARY, serializedData.get());
                }
            }
        }
        
        ShaderOptionGroup Shader::CreateShaderOptionGroup() const
        {
            return ShaderOptionGroup(m_asset->GetShaderOptionGroupLayout());
        }

        const ShaderVariant& Shader::GetVariant(const ShaderVariantId& shaderVariantId)
        {
            Data::Asset<ShaderVariantAsset> shaderVariantAsset = m_asset->GetVariant(shaderVariantId, m_supervariantIndex);
            if (!shaderVariantAsset || shaderVariantAsset->IsRootVariant())
            {
                return m_rootVariant;
            }

            return GetVariant(shaderVariantAsset->GetStableId());
        }

        const ShaderVariant& Shader::GetRootVariant()
        {
            return m_rootVariant;
        }

        ShaderVariantSearchResult Shader::FindVariantStableId(const ShaderVariantId& shaderVariantId) const
        {
            ShaderVariantSearchResult variantSearchResult = m_asset->FindVariantStableId(shaderVariantId);
            return variantSearchResult;
        }

        const ShaderVariant& Shader::GetVariant(ShaderVariantStableId shaderVariantStableId)
        {
            const ShaderVariant& variant = GetVariantInternal(shaderVariantStableId);
            
            if (ShaderReloadDebugTracker::IsEnabled())
            {
                auto makeTimeString = [](AZStd::sys_time_t timestamp, AZStd::sys_time_t now)
                {
                    AZStd::sys_time_t elapsedMicroseconds = now - timestamp;
                    double elapsedSeconds = aznumeric_cast<double>(elapsedMicroseconds / 1'000'000);
                    AZStd::string timeString = AZStd::string::format("%lld (%f seconds ago)", timestamp, elapsedSeconds);
                    return timeString;
                };

                AZStd::sys_time_t now = AZStd::GetTimeNowMicroSecond();

                ShaderReloadDebugTracker::Printf("{%p}->Shader::GetVariant for shader '%s' [build time %s] found variant '%s' [build time %s]", this,
                    m_asset.GetHint().c_str(), makeTimeString(m_asset->GetBuildTimestamp(), now).c_str(),
                    variant.GetShaderVariantAsset().GetHint().c_str(), makeTimeString(variant.GetShaderVariantAsset()->GetBuildTimestamp(), now).c_str());
            }

            return variant;
        }

        const ShaderVariant& Shader::GetVariantInternal(ShaderVariantStableId shaderVariantStableId)
        {
            if (!shaderVariantStableId.IsValid() || shaderVariantStableId == ShaderAsset::RootShaderVariantStableId)
            {
                return m_rootVariant;
            }

            {
                AZStd::shared_lock<decltype(m_variantCacheMutex)> lock(m_variantCacheMutex);

                auto findIt = m_shaderVariants.find(shaderVariantStableId);
                if (findIt != m_shaderVariants.end())
                {
                    // When rebuilding shaders we may be in a state where the ShaderAsset and root ShaderVariantAsset have been rebuilt and
                    // reloaded, but some (or all) shader variants haven't been built yet. Since we want to use the latest version of the
                    // shader code, ignore the old variants and fall back to the newer root variant instead. There's no need to report a
                    // warning here because m_asset->GetVariant below will report one.
                    if (findIt->second.GetBuildTimestamp() >= m_asset->GetBuildTimestamp())
                    {
                        return findIt->second;
                    }
                }
            }

            // By calling GetVariant, an asynchronous asset load request is enqueued if the variant
            // is not fully ready.
            Data::Asset<ShaderVariantAsset> shaderVariantAsset = m_asset->GetVariant(shaderVariantStableId, m_supervariantIndex);
            if (!shaderVariantAsset || shaderVariantAsset == m_asset->GetRootVariant())
            {
                // Return the root variant when the requested variant is not ready.
                return m_rootVariant;
            }

            AZStd::unique_lock<decltype(m_variantCacheMutex)> lock(m_variantCacheMutex);

            // For performance reasons We are breaking this function into two locking steps.
            // which means We must check again if the variant is already in the cache.
            auto findIt = m_shaderVariants.find(shaderVariantStableId);
            if (findIt != m_shaderVariants.end())
            {
                if (findIt->second.GetBuildTimestamp() >= m_asset->GetBuildTimestamp())
                {
                    return findIt->second;
                }
                else
                {
                    // This is probably very rare, but if the variant was loaded on another thread and it's out of date
                    // we just return the root variant. Otherwise we could end up replacing the variant in the map below while
                    // it's being used for rendering.
                    AZ_Warning(
                        "Shader", false,
                        "Detected an uncommon state during shader reload. Returning the root variant instead of replacing the old one.");
                    return m_rootVariant;
                }
            }

            ShaderVariant newVariant;
            newVariant.Init(m_asset, shaderVariantAsset, m_supervariantIndex);
            m_shaderVariants.emplace(shaderVariantStableId, newVariant);

            return m_shaderVariants.at(shaderVariantStableId);
        }

        RHI::PipelineStateType Shader::GetPipelineStateType() const
        {
            return m_pipelineStateType;
        }

        const ShaderInputContract& Shader::GetInputContract() const
        {
            return m_asset->GetInputContract(m_supervariantIndex);
        }

        const ShaderOutputContract& Shader::GetOutputContract() const
        {
            return m_asset->GetOutputContract(m_supervariantIndex);
        }

        const RHI::PipelineState* Shader::AcquirePipelineState(const RHI::PipelineStateDescriptor& descriptor) const
        {
            return m_pipelineStateCache->AcquirePipelineState(m_pipelineLibraryHandle, descriptor);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& Shader::FindShaderResourceGroupLayout(const Name& shaderResourceGroupName) const
        {
            return m_asset->FindShaderResourceGroupLayout(shaderResourceGroupName, m_supervariantIndex);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& Shader::FindShaderResourceGroupLayout(uint32_t bindingSlot) const
        {
            return m_asset->FindShaderResourceGroupLayout(bindingSlot, m_supervariantIndex);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& Shader::FindFallbackShaderResourceGroupLayout() const
        {
            return m_asset->FindFallbackShaderResourceGroupLayout(m_supervariantIndex);
        }

        AZStd::array_view<RHI::Ptr<RHI::ShaderResourceGroupLayout>> Shader::GetShaderResourceGroupLayouts() const
        {
            return m_asset->GetShaderResourceGroupLayouts(m_supervariantIndex);
        }

        const Data::Asset<ShaderAsset>& Shader::GetAsset() const
        {
            return m_asset;
        }
        
        RHI::DrawListTag Shader::GetDrawListTag() const
        {
            return m_drawListTag;
        }

        void Shader::ChangeSupervariant(SupervariantIndex supervariantIndex)
        {
            if (supervariantIndex != m_supervariantIndex)
            {
                m_supervariantIndex = supervariantIndex;
                Init(*m_asset);
            }
        }

    } // namespace RPI
} // namespace AZ
