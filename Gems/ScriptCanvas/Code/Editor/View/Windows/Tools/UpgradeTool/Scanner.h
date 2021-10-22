/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Core.h>
#include <AzCore/Component/TickBus.h>
#include <Editor/View/Windows/Tools/UpgradeTool/ModelTraits.h>

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        class Scanner
            : private AZ::SystemTickBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(Scanner, AZ::SystemAllocator, 0);

            Scanner(const ScanConfiguration& config, AZStd::function<void()> onComplete);

            const ScanResult& GetResult() const;
            ScanResult&& TakeResult();

        private:           
            size_t m_catalogAssetIndex = 0;
            AZStd::function<void()> m_onComplete;
            ScanConfiguration m_config;
            ScanResult m_result;

            void FilterAsset(AZ::Data::Asset<AZ::Data::AssetData>);
            const AZ::Data::AssetInfo& GetCurrentAsset() const;
            AZ::Data::Asset<AZ::Data::AssetData> LoadAsset();
            void OnSystemTick() override;
        };
    }    
}
