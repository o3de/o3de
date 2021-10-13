/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <SharedPreview/SharedPreviewRendererState.h>

namespace AZ
{
    namespace LyIntegration
    {
        //! SharedPreviewRendererLoadState pauses further rendering until all assets used for rendering a thumbnail have been loaded
        class SharedPreviewRendererLoadState
            : public SharedPreviewRendererState
            , private Data::AssetBus::Handler
            , private TickBus::Handler
        {
        public:
            SharedPreviewRendererLoadState(SharedPreviewRendererContext* context);

            void Start() override;
            void Stop() override;

        private:
            void LoadNextAsset();

            // AZ::Data::AssetBus::Handler
            void OnAssetReady(Data::Asset<Data::AssetData> asset) override;
            void OnAssetError(Data::Asset<Data::AssetData> asset) override;
            void OnAssetCanceled(Data::AssetId assetId) override;

            //! AZ::TickBus::Handler interface overrides...
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            static constexpr float TimeOutS = 3.0f;
            Data::AssetId m_assetId;
            float m_timeRemainingS = 0;
        };
    } // namespace LyIntegration
} // namespace AZ
