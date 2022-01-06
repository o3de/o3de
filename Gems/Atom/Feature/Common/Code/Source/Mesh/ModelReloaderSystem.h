/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Mesh/ModelReloaderSystemInterface.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ
{
    namespace Render
    {
        class ModelReloader;

        using RemoveModelFromReloaderSystemEvent = Event<const Data::AssetId&>;
        
        class ModelReloaderSystem
            : public ModelReloaderSystemInterface
        {
        public:
            AZ_RTTI(Render::ModelReloaderSystem, "{8C85ECCD-B6C8-4949-B26C-9C4F1020F2B8}", Render::ModelReloaderSystemInterface);
            
            void ReloadModel(Data::Asset<RPI::ModelAsset> modelAsset, ModelReloadedEvent::Handler& onReloadedEventHandler) override;

        private:
            void RemoveReloader(const Data::AssetId& assetId);

            // Keep track of all the pending reloads so there are no duplicates
            AZStd::unordered_map<Data::AssetId, ModelReloader*> m_pendingReloads;
            AZStd::mutex m_pendingReloadMutex;

            RemoveModelFromReloaderSystemEvent::Handler m_removeModelHandler{
                [&](const Data::AssetId& assetId)
                {
                    RemoveReloader(assetId);
                } };

            friend class ModelReloader;
        };

    } // namespace Render
} // namespace AZ
