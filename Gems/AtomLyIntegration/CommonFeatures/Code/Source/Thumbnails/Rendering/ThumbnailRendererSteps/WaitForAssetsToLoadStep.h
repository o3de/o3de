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

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <Thumbnails/Rendering/ThumbnailRendererSteps/ThumbnailRendererStep.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            //! WaitForAssetsToLoadStep pauses further rendering until all assets used for rendering a thumbnail have been loaded
            class WaitForAssetsToLoadStep
                : public ThumbnailRendererStep
                , private Data::AssetBus::Handler
                , private TickBus::Handler
            {
            public:
                WaitForAssetsToLoadStep(ThumbnailRendererContext* context);

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
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ

