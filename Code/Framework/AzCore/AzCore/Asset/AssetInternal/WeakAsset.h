/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Asset/AssetCommon.h>

namespace AZ {
    namespace Data {
        class AssetData;
    }
}

namespace AZ::Data::AssetInternal
{
    /// WeakAsset keeps a reference to AssetData but will not cause an asset to load
    /// If an asset is only referenced by WeakAssets, any pending load will be canceled and the asset should be released shortly after
    /// This class is only intended for use in AssetManager systems
    template<class T>
    class WeakAsset
    {
    public:
        static constexpr bool EnableAssetCancellation = false;

        WeakAsset() = default;

        WeakAsset(AssetData* assetData, AssetLoadBehavior assetReferenceLoadBehavior);
        explicit WeakAsset(const Asset<AssetData>& asset);

        WeakAsset(const WeakAsset& rhs);
        WeakAsset(WeakAsset&& rhs);

        WeakAsset& operator=(const WeakAsset& rhs);
        WeakAsset& operator=(WeakAsset&& rhs);

        ~WeakAsset();

        void SetData(AssetData* assetData);

        AssetId GetId() const;

        /// Attempts to get a full reference to the AssetData as long as there is at least 1 existing Asset<T> reference 
        Asset<T> GetStrongReference() const;

        explicit operator bool() const;

    private:

        AssetId m_assetId{};
        AssetData* m_assetData{ nullptr };
        AssetLoadBehavior m_assetLoadBehavior{ AssetLoadBehavior::Default };
    };

    template <class T>
    WeakAsset<T>::WeakAsset(AssetData* assetData, AssetLoadBehavior assetReferenceLoadBehavior)
        : m_assetLoadBehavior(assetReferenceLoadBehavior)

    {
        SetData(assetData);
    }

    template <class T>
    WeakAsset<T>::WeakAsset(const Asset<AssetData>& asset)
        : m_assetLoadBehavior(asset.GetAutoLoadBehavior())
    {
        SetData(asset.GetData());
    }

    template <class T>
    WeakAsset<T>::WeakAsset(const WeakAsset& rhs)
        : m_assetLoadBehavior(rhs.m_assetLoadBehavior)
    {
        SetData(rhs.m_assetData);
    }

    template <class T>
    WeakAsset<T>::WeakAsset(WeakAsset&& rhs)
        : m_assetData(AZStd::move(rhs.m_assetData))
        , m_assetLoadBehavior(rhs.m_assetLoadBehavior)
    {
        rhs.m_assetData = nullptr;

        if (m_assetData)
        {
            m_assetId = AZStd::move(rhs.m_assetId);
        }
    }

    template <class T>
    WeakAsset<T>& WeakAsset<T>::operator=(const WeakAsset& rhs)
    {
        m_assetLoadBehavior = rhs.m_assetLoadBehavior;
        SetData(rhs.m_assetData);

        return *this;
    }

    template <class T>
    WeakAsset<T>& WeakAsset<T>::operator=(WeakAsset&& rhs)
    {
        m_assetLoadBehavior = rhs.m_assetLoadBehavior;

        // Make sure the assetData ptr getting replaced releases its weak reference.  Otherwise this will "leak" a weak reference:
        // - If the left side is different than the right, the left side will have one less reference when it gets overwritten
        // - If the left and right sides are the same, clearing the right side's reference means one less reference will exist
        if (m_assetData)
        {
            if constexpr (EnableAssetCancellation)
            {
                m_assetData->ReleaseWeak();
            }
            else
            {
                m_assetData->Release();
            }
        }
        m_assetData = AZStd::move(rhs.m_assetData);
        rhs.m_assetData = nullptr;

        if (m_assetData)
        {
            m_assetId = AZStd::move(rhs.m_assetId);
        }
        else
        {
            m_assetId.SetInvalid();
        }

        return *this;
    }

    template <class T>
    WeakAsset<T>::~WeakAsset()
    {
        SetData(nullptr);
    }

    template <class T>
    void WeakAsset<T>::SetData(AssetData* assetData)
    {
        m_assetId.SetInvalid();

        if (assetData)
        {
            if constexpr (EnableAssetCancellation)
            {
                assetData->AcquireWeak();
            }
            else
            {
                assetData->Acquire();
            }
            m_assetId = assetData->GetId();
        }

        if (m_assetData)
        {
            if constexpr (EnableAssetCancellation)
            {
                m_assetData->ReleaseWeak();
            }
            else
            {
                m_assetData->Release();
            }
        }

        m_assetData = assetData;
    }

    template <class T>
    AssetId WeakAsset<T>::GetId() const
    {
        return m_assetId;
    }

    template <class T>
    Asset<T> WeakAsset<T>::GetStrongReference() const
    {
        if (!m_assetData || m_assetData->GetUseCount() <= 0)
        {
            return Asset<T>(m_assetId, AssetType::CreateNull());
        }

        return Asset<T>(m_assetData, m_assetLoadBehavior);
    }

    template <class T>
    WeakAsset<T>::operator bool() const
    {
        return m_assetData != nullptr;
    }
}
