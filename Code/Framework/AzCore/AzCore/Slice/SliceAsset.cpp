/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Slice/SliceBus.h>
#include <AzCore/Asset/AssetManager.h>

namespace AZ
{
    //=========================================================================
    // SliceAsset
    //=========================================================================
    SliceAsset::SliceAsset(const Data::AssetId& assetId /*= Data::AssetId()*/)
        : AssetData(assetId)
        , m_entity(nullptr)
        , m_component(nullptr)
        , m_ignoreNextAutoReload(false)
    {
    }

    //=========================================================================
    // ~SliceAsset
    //=========================================================================
    SliceAsset::~SliceAsset()
    {
        delete m_entity;
    }

    //=========================================================================
    // SetData
    //=========================================================================
    bool SliceAsset::SetData(Entity* entity, SliceComponent* component, bool deleteExisting)
    {
        AZ_Assert((entity == nullptr && component == nullptr) ||
            (entity->FindComponent<SliceComponent>() == component), "Slice component doesn't belong to the entity!");

        if (IsLoading())
        {
            // we can't set the data while loading
            return false;
        }

        if (deleteExisting)
        {
            delete m_entity; // delete the entity if we have one, it should delete the component too
        }

        m_entity = entity;
        m_component = component;
        if (m_entity && m_component)
        {
            m_status = AssetStatus::Ready;
        }
        else
        {
            m_status = AssetStatus::NotLoaded;
        }
        return true;
    }

    //=========================================================================
    // Clone
    //=========================================================================
    SliceAsset* SliceAsset::Clone()
    {
        return aznew SliceAsset(GetId());
    }

    bool SliceAsset::HandleAutoReload()
    {
        bool isAutoReloadable = !m_ignoreNextAutoReload;

        // We have handled this reload
        // We must set this flag to true again
        // to prevent further reloads
        m_ignoreNextAutoReload = false;

        return isAutoReloadable;
    }

    namespace Data
    {
        //=========================================================================
        // AssetFilterSourceSlicesOnly
        //=========================================================================
        bool AssetFilterSourceSlicesOnly(const Data::AssetFilterInfo& filterInfo)
        {
            // Expand regular slice references (but not dynamic slice references).
            return (filterInfo.m_assetType == AZ::AzTypeInfo<AZ::SliceAsset>::Uuid());
        }
    }

} // namespace AZ
