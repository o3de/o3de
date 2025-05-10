/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetBus.h>

AZCORE_INSTANTIATE_EBUS_MULTI_ADDRESS(AZ::Data::AssetEvents);

namespace AZ::Data
{
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

} // namespace AZ::Data
