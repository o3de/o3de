/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/string/conversions.h>

namespace AZ::Data
{
    AssetFilterInfo::AssetFilterInfo(const AssetId& id, const AssetType& assetType, AssetLoadBehavior loadBehavior)
        : m_assetId(id)
        , m_assetType(assetType)
        , m_loadBehavior(loadBehavior)
    {
    }

    AssetFilterInfo::AssetFilterInfo(const Asset<AssetData>& asset)
        : m_assetId(asset.GetId())
        , m_assetType(asset.GetType())
        , m_loadBehavior(asset.GetAutoLoadBehavior())
    {
    }


    AssetId AssetId::CreateString(AZStd::string_view input)
    {
        size_t separatorIdx = input.find(':');
        if (separatorIdx == AZStd::string_view::npos)
        {
            return AssetId();
        }

        AssetId assetId;
        assetId.m_guid = Uuid::CreateString(input.data(), separatorIdx);
        if (assetId.m_guid.IsNull())
        {
            return AssetId();
        }

        assetId.m_subId = static_cast<AZ::u32>(strtoul(&input[separatorIdx + 1], nullptr, 16));

        return assetId;
    }

    void AssetId::Reflect(AZ::ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<Data::AssetId>()
                ->Version(1)
                ->Field("guid", &Data::AssetId::m_guid)
                ->Field("subId", &Data::AssetId::m_subId)
                ;
        }

        if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            behaviorContext->Class<Data::AssetId>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Asset")
                ->Attribute(AZ::Script::Attributes::Module, "asset")
                ->Constructor()
                ->Constructor<const Uuid&, u32>()
                ->Constructor<AZStd::string_view, u32>()
                ->Method("CreateString", &Data::AssetId::CreateString)
                ->Method("IsValid", &Data::AssetId::IsValid)
                    ->Attribute(AZ::Script::Attributes::Alias, "is_valid")
                ->Method("ToString", [](const Data::AssetId* self) { return self->ToString<AZStd::string>(); })
                    ->Attribute(AZ::Script::Attributes::Alias, "to_string")
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ->Method("__repr__", [](const Data::AssetId* self) { return self->ToString<AZStd::string>(); })
                ->Method("IsEqual", [](const Data::AssetId& self, const Data::AssetId& other) { return self == other; })
                    ->Attribute(AZ::Script::Attributes::Alias, "is_equal")
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                ;

            behaviorContext->Class<Data::AssetInfo>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Asset")
                ->Attribute(AZ::Script::Attributes::Module, "asset")
                ->Property("assetId", BehaviorValueGetter(&Data::AssetInfo::m_assetId), nullptr)
                ->Property("assetType", BehaviorValueGetter(&Data::AssetInfo::m_assetType), nullptr)
                ->Property("sizeBytes", BehaviorValueGetter(&Data::AssetInfo::m_sizeBytes), nullptr)
                ->Property("relativePath", BehaviorValueGetter(&Data::AssetInfo::m_relativePath), nullptr)
                ;
        }
    }

    AssetId::FixedString AssetId::ToFixedString() const
    {
        FixedString result;
        result = FixedString::format("%s:%08x", m_guid.ToFixedString().c_str(), m_subId);
        return result;
    }

    namespace AssetInternal
    {
        Asset<AssetData> FindOrCreateAsset(const AssetId& id, const AssetType& type, AssetLoadBehavior assetReferenceLoadBehavior)
        {
            return AssetManager::Instance().FindOrCreateAsset(id, type, assetReferenceLoadBehavior);
        }

        Asset<AssetData> GetAsset(const AssetId& id, const AssetType& type, AssetLoadBehavior assetReferenceLoadBehavior,
            const AssetLoadParameters& loadParams)
        {
            return AssetManager::Instance().GetAsset(id, type, assetReferenceLoadBehavior, loadParams);
        }

        AssetData::AssetStatus BlockUntilLoadComplete(const Asset<AssetData>& asset)
        {
            return AssetManager::Instance().BlockUntilLoadComplete(asset);
        }

        void UpdateAssetInfo(AssetId& id, AZStd::string& assetHint)
        {
            // it is possible that the assetID given is legacy / old and we have a new assetId we can use instead for it.
            // in that case, upgrade the AssetID to the new one, so that future saves are in the new format.
            // this function should only be invoked if the feature is turned on in the asset manager as it can be (slightly) expensive

            if ((!AssetManager::IsReady()) || (!AssetManager::Instance().GetAssetInfoUpgradingEnabled()))
            {
                return;
            }

            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, id);
            if (assetInfo.m_assetId.IsValid())
            {
                id = assetInfo.m_assetId;
                if (!assetInfo.m_relativePath.empty())
                {
                    assetHint = assetInfo.m_relativePath;
                }
            }
        }

        bool ReloadAsset(AssetData* assetData, AssetLoadBehavior assetReferenceLoadBehavior)
        {
            AssetManager::Instance().ReloadAsset(assetData->GetId(), assetReferenceLoadBehavior);
            return true;
        }

        bool SaveAsset(AssetData* assetData, AssetLoadBehavior assetReferenceLoadBehavior)
        {
            AssetManager::Instance().SaveAsset({ assetData, assetReferenceLoadBehavior });
            return true;
        }

        Asset<AssetData> GetAssetData(const AssetId& id, AssetLoadBehavior assetReferenceLoadBehavior)
        {
            if (AssetManager::IsReady())
            {
                AZStd::lock_guard<AZStd::recursive_mutex> assetLock(AssetManager::Instance().m_assetMutex);
                auto it = AssetManager::Instance().m_assets.find(id);
                if (it != AssetManager::Instance().m_assets.end())
                {
                    return { it->second, assetReferenceLoadBehavior };
                }
            }
            return {nullptr, assetReferenceLoadBehavior};
        }

        AssetId ResolveAssetId(const AssetId& id)
        {
            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, id);
            if (assetInfo.m_assetId.IsValid())
            {
                return assetInfo.m_assetId;
            }
            else
            {
                return id;
            }

        }
    }

    AssetData::~AssetData()
    {
        UnregisterWithHandler();
    }

    void AssetData::Reflect(AZ::ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<AZ::Data::AssetData>()
                ->Version(1)
                ;
        }

        if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            behaviorContext->Class<AssetData>("AssetData")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Asset")
                ->Attribute(AZ::Script::Attributes::Module, "asset")
                ->Method("IsReady", &AssetData::IsReady)
                    ->Attribute(AZ::Script::Attributes::Alias, "is_ready")
                ->Method("IsError", &AssetData::IsError)
                    ->Attribute(AZ::Script::Attributes::Alias, "is_error")
                ->Method("IsLoading", &AssetData::IsLoading)
                    ->Attribute(AZ::Script::Attributes::Alias, "is_loading")
                ->Method("GetId", &AssetData::GetId)
                    ->Attribute(AZ::Script::Attributes::Alias, "get_id")
                ->Method("GetUseCount", &AssetData::GetUseCount)
                    ->Attribute(AZ::Script::Attributes::Alias, "get_use_count")
                ;
        }
    }

    void AssetData::Acquire()
    {
        AZ_Assert(m_useCount >= 0, "AssetData has been deleted");

        AcquireWeak();
        ++m_useCount;
    }

    void AssetData::Release()
    {
        AZ_Assert(m_useCount > 0, "Usecount is already 0!");

        if (m_useCount.fetch_sub(1) == 1)
        {
            if (AssetManager::IsReady())
            {
                AssetManager::Instance().OnAssetUnused(this);
            }
            else
            {
                AZ_Assert(false, "Attempting to release asset after AssetManager has been destroyed!");
            }
        }

        ReleaseWeak();
    }

    void AssetData::AcquireWeak()
    {
        AZ_Assert(m_useCount >= 0, "AssetData has been deleted");
        ++m_weakUseCount;
    }

    void AssetData::ReleaseWeak()
    {
        AZ_Assert(m_weakUseCount > 0, "WeakUseCount is already 0");

        AssetId assetId = m_assetId;
        int creationToken = m_creationToken;
        AssetType assetType = GetType();
        bool removeFromHash = IsRegisterReadonlyAndShareable();
        // default creation token implies that the asset was not created by the asset manager and therefore it cannot be in the asset map.
        removeFromHash = creationToken == s_defaultCreationToken ? false : removeFromHash;

        if (m_weakUseCount.fetch_sub(1) == 1)
        {
            if (AssetManager::IsReady())
            {
                AssetManager::Instance().ReleaseAsset(this, assetId, assetType, removeFromHash, creationToken);
            }
            else
            {
                AZ_Assert(false, "Attempting to release asset after AssetManager has been destroyed!");
            }
        }
    }

    bool AssetData::IsLoading(bool includeQueued) const
    {
        auto curStatus = GetStatus();
        return(curStatus == AssetStatus::Loading || curStatus == AssetStatus::LoadedPreReady || curStatus==AssetStatus::StreamReady ||
                (includeQueued && curStatus == AssetStatus::Queued));
    }

    void AssetData::RegisterWithHandler(AssetHandler* handler)
    {
        if (!handler)
        {
            AZ_Error("AssetData", false, "No handler to register with");
            return;
        }
        m_registeredHandler = handler;
    }

    void AssetData::UnregisterWithHandler()
    {
        if (m_registeredHandler)
        {
            m_registeredHandler = nullptr;
        }
    }

    bool AssetData::GetFlag(const AssetDataFlags& checkFlag) const
    {
        return m_flags[aznumeric_cast<AZStd::size_t>(checkFlag)];
    }

    void AssetData::SetFlag(const AssetDataFlags& checkFlag, bool setValue)
    {
        m_flags.set(aznumeric_cast<AZStd::size_t>(checkFlag), setValue);
    }

    bool AssetData::GetRequeue() const
    {
        return GetFlag(AssetDataFlags::Requeue);
    }
    void AssetData::SetRequeue(bool requeue)
    {
        SetFlag(AssetDataFlags::Requeue, requeue);
    }

    void AssetBusCallbacks::SetCallbacks(const AssetReadyCB& readyCB, const AssetMovedCB& movedCB, const AssetReloadedCB& reloadedCB,
        const AssetSavedCB& savedCB, const AssetUnloadedCB& unloadedCB, const AssetErrorCB& errorCB, const AssetCanceledCB& cancelCB)
    {
        m_onAssetReadyCB = readyCB;
        m_onAssetMovedCB = movedCB;
        m_onAssetReloadedCB = reloadedCB;
        m_onAssetSavedCB = savedCB;
        m_onAssetUnloadedCB = unloadedCB;
        m_onAssetErrorCB = errorCB;
        m_onAssetCanceledCB = cancelCB;
    }

    void AssetBusCallbacks::ClearCallbacks()
    {
        SetCallbacks(AssetBusCallbacks::AssetReadyCB(),
            AssetBusCallbacks::AssetMovedCB(),
            AssetBusCallbacks::AssetReloadedCB(),
            AssetBusCallbacks::AssetSavedCB(),
            AssetBusCallbacks::AssetUnloadedCB(),
            AssetBusCallbacks::AssetErrorCB(),
            AssetBusCallbacks::AssetCanceledCB());
    }


    void AssetBusCallbacks::SetOnAssetReadyCallback(const AssetReadyCB& readyCB)
    {
        m_onAssetReadyCB = readyCB;
    }

    void AssetBusCallbacks::SetOnAssetMovedCallback(const AssetMovedCB& movedCB)
    {
        m_onAssetMovedCB = movedCB;
    }

    void AssetBusCallbacks::SetOnAssetReloadedCallback(const AssetReloadedCB& reloadedCB)
    {
        m_onAssetReloadedCB = reloadedCB;
    }

    void AssetBusCallbacks::SetOnAssetSavedCallback(const AssetSavedCB& savedCB)
    {
        m_onAssetSavedCB = savedCB;
    }

    void AssetBusCallbacks::SetOnAssetUnloadedCallback(const AssetUnloadedCB& unloadedCB)
    {
        m_onAssetUnloadedCB = unloadedCB;
    }

    void AssetBusCallbacks::SetOnAssetErrorCallback(const AssetErrorCB& errorCB)
    {
        m_onAssetErrorCB = errorCB;
    }

    void AssetBusCallbacks::SetOnAssetCanceledCallback(const AssetCanceledCB& cancelCB)
    {
        m_onAssetCanceledCB = cancelCB;
    }

    void AssetBusCallbacks::OnAssetReady(Asset<AssetData> asset)
    {
        if (m_onAssetReadyCB)
        {
            m_onAssetReadyCB(asset, *this);
        }
    }

    void AssetBusCallbacks::OnAssetMoved(Asset<AssetData> asset, void* oldDataPointer)
    {
        if (m_onAssetMovedCB)
        {
            m_onAssetMovedCB(asset, oldDataPointer, *this);
        }
    }

    void AssetBusCallbacks::OnAssetReloaded(Asset<AssetData> asset)
    {
        if (m_onAssetReloadedCB)
        {
            m_onAssetReloadedCB(asset, *this);
        }
    }

    void AssetBusCallbacks::OnAssetSaved(Asset<AssetData> asset, bool isSuccessful)
    {
        if (m_onAssetSavedCB)
        {
            m_onAssetSavedCB(asset, isSuccessful, *this);
        }
    }

    void AssetBusCallbacks::OnAssetUnloaded(const AssetId assetId, const AssetType assetType)
    {
        if (m_onAssetUnloadedCB)
        {
            m_onAssetUnloadedCB(assetId, assetType, *this);
        }
    }

    void AssetBusCallbacks::OnAssetError(Asset<AssetData> asset)
    {
        if (m_onAssetErrorCB)
        {
            m_onAssetErrorCB(asset, *this);
        }
    }

    void AssetBusCallbacks::OnAssetCanceled(const AssetId assetId)
    {
        if (m_onAssetCanceledCB)
        {
            m_onAssetCanceledCB(assetId, *this);
        }
    }

    /*static*/ bool AssetFilterNoAssetLoading([[maybe_unused]] const AssetFilterInfo& filterInfo)
    {
        return false;
    }
    namespace ProductDependencyInfo
    {
        AZ::Data::AssetLoadBehavior LoadBehaviorFromFlags(const ProductDependencyFlags& dependencyFlags)
        {
            AZ::u8 loadBehaviorValue = 0;
            for (AZ::u8 thisFlag = aznumeric_cast<AZ::u8>(ProductDependencyFlagBits::LoadBehaviorLow);
                thisFlag <= aznumeric_cast<AZ::u8>(ProductDependencyFlagBits::LoadBehaviorHigh); ++thisFlag)
            {
                if (dependencyFlags[thisFlag])
                {
                    loadBehaviorValue |= (1 << thisFlag);
                }
            }
            return static_cast<AZ::Data::AssetLoadBehavior>(loadBehaviorValue);
        }

        ProductDependencyFlags CreateFlags(AZ::Data::AssetLoadBehavior autoLoadBehavior)
        {
            AZ::Data::ProductDependencyInfo::ProductDependencyFlags returnFlags;
            AZ::u8 loadBehavior = aznumeric_caster(autoLoadBehavior);
            for (AZ::u8 thisFlag = aznumeric_cast<AZ::u8>(ProductDependencyFlagBits::LoadBehaviorLow);
                thisFlag <= aznumeric_cast<AZ::u8>(ProductDependencyFlagBits::LoadBehaviorHigh); ++thisFlag)
            {
                if (loadBehavior & (1 << thisFlag))
                {
                    returnFlags[thisFlag] = true;
                }
            }
            return returnFlags;
        }
    }
} // namespace AZ::Data
