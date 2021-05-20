/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include <Atom/RPI.Public/Shader/Shader2.h>

#include <AzCore/IO/SystemFile.h>

#include <Atom/RHI/RHISystemInterface.h>

#include <Atom/RHI/PipelineStateCache.h>
#include <Atom/RHI/Factory.h>

#include <AtomCore/Instance/InstanceDatabase.h>

#include <AzCore/Interface/Interface.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus2.h>
#include <Atom/RPI.Public/Shader/ShaderReloadDebugTracker.h>

namespace AZ
{
    namespace RPI
    {
        Data::Instance<Shader2> Shader2::FindOrCreate(const Data::Asset<ShaderAsset2>& shaderAsset, const Name& supervariantName)
        {
            Data::Instance<Shader2> shaderInstance = Data::InstanceDatabase<Shader2>::Instance().FindOrCreate(
                Data::InstanceId::CreateFromAssetId(shaderAsset.GetId()),
                shaderAsset);
            if (!shaderInstance)
            {
                return nullptr;
            }

            if (!shaderInstance->SelectSupervariant(supervariantName))
            {
                return nullptr;
            }

            const RHI::ResultCode resultCode = shaderInstance->Init(*shaderAsset.Get());
            if (resultCode != RHI::ResultCode::Success)
            {
                return nullptr;
            }
            return shaderInstance;
        }

        Data::Instance<Shader2> Shader2::CreateInternal([[maybe_unused]] ShaderAsset2& shaderAsset)
        {
            Data::Instance<Shader2> shader = aznew Shader2();
            return shader;
        }

        Shader2::~Shader2()
        {
            Shutdown();
        }

        bool Shader2::SelectSupervariant(const Name& supervariantName)
        {
            if (supervariantName.IsEmpty())
            {
                m_supervariantIndex = DefaultSupervariantIndex;
                return true;
            }

            auto supervariantIndex = m_asset->GetSupervariantIndex(supervariantName);
            if (supervariantIndex == InvalidSupervariantIndex)
            {
                return false;
            }

            m_supervariantIndex = supervariantIndex;
            return true;
        }

        RHI::ResultCode Shader2::Init(ShaderAsset2& shaderAsset)
        {
            AZ_Assert(m_supervariantIndex != InvalidSupervariantIndex, "Invalid supervariant index");

            ShaderVariantFinderNotificationBus2::Handler::BusDisconnect();
            ShaderVariantFinderNotificationBus2::Handler::BusConnect(shaderAsset.GetId());

            RHI::RHISystemInterface* rhiSystem = RHI::RHISystemInterface::Get();
            RHI::DrawListTagRegistry* drawListTagRegistry = rhiSystem->GetDrawListTagRegistry();

            m_asset = { &shaderAsset, AZ::Data::AssetLoadBehavior::PreLoad };
            m_pipelineStateType = shaderAsset.GetPipelineStateType();

            {
                AZStd::unique_lock<decltype(m_variantCacheMutex)> lock(m_variantCacheMutex);
                m_shaderVariants.clear();
            }
            m_rootVariant.Init(shaderAsset, shaderAsset.GetRootVariant(m_supervariantIndex), m_supervariantIndex);

            if (m_pipelineLibraryHandle.IsNull())
            {
                // We set up a pipeline library only once for the lifetime of the Shader2 instance.
                // This should allow the Shader2 to be reloaded at runtime many times, and cache and reuse PipelineState objects rather than rebuild them.
                // It also fixes a particular TDR crash that occurred on some hardware when hot-reloading shaders and building pipeline states
                // in a new pipeline library every time.

                RHI::PipelineStateCache* pipelineStateCache = rhiSystem->GetPipelineStateCache();
                ConstPtr<RHI::PipelineLibraryData> serializedData = LoadPipelineLibrary();
                RHI::PipelineLibraryHandle pipelineLibraryHandle = pipelineStateCache->CreateLibrary(serializedData.get());

                if (pipelineLibraryHandle.IsNull())
                {
                    AZ_Error("Shader2", false, "Failed to create pipeline library from pipeline state cache.");
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
                    AZ_Error("Shader2", false, "Failed to acquire a DrawListTag. Entries are full.");
                }
            }

            Data::AssetBus::Handler::BusConnect(m_asset.GetId());

            return RHI::ResultCode::Success;
        }

        void Shader2::Shutdown()
        {
            ShaderVariantFinderNotificationBus2::Handler::BusDisconnect();
            Data::AssetBus::Handler::BusDisconnect();

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
        void Shader2::OnAssetReloaded(Data::Asset<Data::AssetData> asset)
        {
            ShaderReloadDebugTracker::ScopedSection reloadSection("Shader2::OnAssetReloaded %s", asset.GetHint().c_str());

            if (asset->GetId() == m_asset->GetId())
            {
                Data::Asset<ShaderAsset2> newAsset = { asset.GetAs<ShaderAsset2>(), AZ::Data::AssetLoadBehavior::PreLoad };
                AZ_Assert(newAsset, "Reloaded ShaderAsset2 is null");

                Data::AssetBus::Handler::BusDisconnect();
                Init(*newAsset.Get());
                ShaderReloadNotificationBus2::Event(asset.GetId(), &ShaderReloadNotificationBus2::Events::OnShaderReinitialized, *this);
            }
        }
        ///////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////
        /// ShaderVariantFinderNotificationBus2 overrides
        void Shader2::OnShaderVariantAssetReady(Data::Asset<ShaderVariantAsset2> shaderVariantAsset, bool isError)
        {
            AZ_Assert(shaderVariantAsset, "Reloaded ShaderVariantAsset is null");
            const ShaderVariantStableId stableId = shaderVariantAsset->GetStableId();
            const ShaderVariantId& shaderVariantId = shaderVariantAsset->GetShaderVariantId();

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
                    "The root variant is expected to be updated by the ShaderAsset2.");
                AZStd::unique_lock<decltype(m_variantCacheMutex)> lock(m_variantCacheMutex);

                auto iter = m_shaderVariants.find(stableId);
                if (iter != m_shaderVariants.end())
                {
                    ShaderVariant2& shaderVariant = iter->second;

                    if (!shaderVariant.Init(*m_asset.Get(), shaderVariantAsset, m_supervariantIndex))
                    {
                        AZ_Error("Shader2", false, "Failed to init shaderVariant with StableId=%u", shaderVariantAsset->GetStableId());
                        m_shaderVariants.erase(stableId);
                    }
                }
                else
                {
                    //This is the first time the shader variant asset comes to life.
                    ShaderVariant2 newVariant;
                    newVariant.Init(*m_asset, shaderVariantAsset, m_supervariantIndex);
                    m_shaderVariants.emplace(stableId, newVariant);
                }
            }

            //Even if there was an error, the interested parties should be notified.
            ShaderReloadNotificationBus2::Event(m_asset.GetId(), &ShaderReloadNotificationBus2::Events::OnShaderVariantReinitialized, *this, shaderVariantId, stableId);
        }
        ///////////////////////////////////////////////////////////////////

        ConstPtr<RHI::PipelineLibraryData> Shader2::LoadPipelineLibrary() const
        {
            if (IO::FileIOBase::GetInstance())
            {
                return Utils::LoadObjectFromFile<RHI::PipelineLibraryData>(GetPipelineLibraryPath());
            }
            return nullptr;
        }

        void Shader2::SavePipelineLibrary() const
        {
            if (auto* fileIOBase = IO::FileIOBase::GetInstance())
            {
                RHI::ConstPtr<RHI::PipelineLibraryData> serializedData = m_pipelineStateCache->GetLibrarySerializedData(m_pipelineLibraryHandle);
                if (serializedData)
                {
                    const AZStd::string pipelineLibraryPath = GetPipelineLibraryPath();

                    char pipelineLibraryPathResolved[AZ_MAX_PATH_LEN] = { 0 };
                    fileIOBase->ResolvePath(pipelineLibraryPath.c_str(), pipelineLibraryPathResolved, AZ_MAX_PATH_LEN);
                    Utils::SaveObjectToFile(pipelineLibraryPathResolved, DataStream::ST_BINARY, serializedData.get());
                }
            }
            else
            {
                AZ_Error("Shader2", false, "FileIOBase is not initialized");
            }
        }

        AZStd::string Shader2::GetPipelineLibraryPath() const
        {
            const Data::InstanceId& instanceId = GetId();
            Name platformName = RHI::Factory::Get().GetName();
            Name shaderName = m_asset->GetName();

            AZStd::string uuidString;
            instanceId.m_guid.ToString<AZStd::string>(uuidString, false, false);

            return AZStd::string::format("@user@/Atom/PipelineStateCache/%s/%s_%s_%d.bin", platformName.GetCStr(), shaderName.GetCStr(), uuidString.data(), instanceId.m_subId);
        }

        ShaderOptionGroup Shader2::CreateShaderOptionGroup() const
        {
            return ShaderOptionGroup(m_asset->GetShaderOptionGroupLayout());
        }

        const ShaderVariant2& Shader2::GetVariant(const ShaderVariantId& shaderVariantId)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);
            Data::Asset<ShaderVariantAsset2> shaderVariantAsset = m_asset->GetVariant(shaderVariantId, m_supervariantIndex);
            if (!shaderVariantAsset || shaderVariantAsset->IsRootVariant())
            {
                return m_rootVariant;
            }

            return GetVariant(shaderVariantAsset->GetStableId());
        }

        const ShaderVariant2& Shader2::GetRootVariant()
        {
            return m_rootVariant;
        }

        ShaderVariantSearchResult Shader2::FindVariantStableId(const ShaderVariantId& shaderVariantId) const
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);
            ShaderVariantSearchResult variantSearchResult = m_asset->FindVariantStableId(shaderVariantId);
            return variantSearchResult;
        }

        const ShaderVariant2& Shader2::GetVariant(ShaderVariantStableId shaderVariantStableId)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);

            if (!shaderVariantStableId.IsValid() || shaderVariantStableId == ShaderAsset2::RootShaderVariantStableId)
            {
                return m_rootVariant;
            }

            {
                AZStd::shared_lock<decltype(m_variantCacheMutex)> lock(m_variantCacheMutex);

                auto findIt = m_shaderVariants.find(shaderVariantStableId);
                if (findIt != m_shaderVariants.end())
                {
                    // When rebuilding shaders we may be in a state where the ShaderAsset2 and root ShaderVariantAsset have been rebuilt and
                    // reloaded, but some (or all) shader variants haven't been built yet. Since we want to use the latest version of the
                    // shader code, ignore the old variants and fall back to the newer root variant instead. There's no need to report a
                    // warning here because m_asset->GetVariant below will report one.
                    if (findIt->second.GetBuildTimestamp() >= m_asset->GetShaderAssetBuildTimestamp())
                    {
                        return findIt->second;
                    }
                }
            }

            // By calling GetVariant, an asynchronous asset load request is enqueued if the variant
            // is not fully ready.
            Data::Asset<ShaderVariantAsset2> shaderVariantAsset = m_asset->GetVariant(shaderVariantStableId, m_supervariantIndex);
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
                if (findIt->second.GetBuildTimestamp() >= m_asset->GetShaderAssetBuildTimestamp())
                {
                    return findIt->second;
                }
                else
                {
                    // This is probably very rare, but if the variant was loaded on another thread and it's out of date
                    // we just return the root variant. Otherwise we could end up replacing the variant in the map below while
                    // it's being used for rendering.
                    AZ_Warning(
                        "Shader2", false,
                        "Detected an uncommon state during shader reload. Returning the root variant instead of replacing the old one.");
                    return m_rootVariant;
                }
            }

            ShaderVariant2 newVariant;
            newVariant.Init(*m_asset, shaderVariantAsset, m_supervariantIndex);
            m_shaderVariants.emplace(shaderVariantStableId, newVariant);

            return m_shaderVariants.at(shaderVariantStableId);
        }

        RHI::PipelineStateType Shader2::GetPipelineStateType() const
        {
            return m_pipelineStateType;
        }

        const ShaderInputContract& Shader2::GetInputContract() const
        {
            return m_asset->GetInputContract(m_supervariantIndex);
        }

        const ShaderOutputContract& Shader2::GetOutputContract() const
        {
            return m_asset->GetOutputContract(m_supervariantIndex);
        }

        const RHI::PipelineState* Shader2::AcquirePipelineState(const RHI::PipelineStateDescriptor& descriptor) const
        {
            return m_pipelineStateCache->AcquirePipelineState(m_pipelineLibraryHandle, descriptor);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout> Shader2::FindShaderResourceGroupLayout(const Name& shaderResourceGroupName) const
        {
            return m_asset->FindShaderResourceGroupLayout(shaderResourceGroupName, m_supervariantIndex);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout> Shader2::FindShaderResourceGroupLayout(uint32_t bindingSlot) const
        {
            return m_asset->FindShaderResourceGroupLayout(bindingSlot, m_supervariantIndex);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout> Shader2::FindFallbackShaderResourceGroupLayout() const
        {
            return m_asset->FindFallbackShaderResourceGroupLayout(m_supervariantIndex);
        }

        AZStd::array_view<RHI::Ptr<RHI::ShaderResourceGroupLayout>> Shader2::GetShaderResourceGroupLayouts() const
        {
            return m_asset->GetShaderResourceGroupLayouts(m_supervariantIndex);
        }

        const Data::Asset<ShaderAsset2>& Shader2::GetAsset() const
        {
            return m_asset;
        }
        
        RHI::DrawListTag Shader2::GetDrawListTag() const
        {
            return m_drawListTag;
        }
    } // namespace RPI
} // namespace AZ
