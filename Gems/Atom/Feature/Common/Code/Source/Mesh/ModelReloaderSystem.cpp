/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Mesh/ModelReloaderSystem.h>
#include <Mesh/ModelReloader.h>
#include <AzCore/std/parallel/scoped_lock.h>

namespace AZ
{
    namespace Render
    {
        void ModelReloaderSystem::ReloadModel(Data::Asset<RPI::ModelAsset> modelAsset, ModelReloadedEvent::Handler& onReloadedEventHandler)
        {
            AZStd::scoped_lock lock(m_pendingReloadMutex);
            if (m_pendingReloads.find(modelAsset.GetId()) == m_pendingReloads.end())
            {
                ModelReloader* reloader = new ModelReloader(modelAsset);
                m_pendingReloads[modelAsset.GetId()] = reloader;
            }

            m_pendingReloads[modelAsset.GetId()]->ConnectOnReloadedEventHandler(onReloadedEventHandler);
        }

        void ModelReloaderSystem::RemoveReloader(const Data::AssetId& assetId)
        {
            AZStd::scoped_lock lock(m_pendingReloadMutex);
            // We don't delete the ModelReloader here, because its in the middle of signaling this RemoveReloader event.
            // We only remove it from the pending reloads here.
            // The ModelReloader will delete itself after it finishes firing this event.
            m_pendingReloads.erase(assetId);
        }
    } // namespace Render
} // namespace AZ
