/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Core.h>
#include <Editor/View/Windows/Tools/UpgradeTool/ModelTraits.h>
#include <AzCore/Component/TickBus.h>

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        class Modifier
            : private AZ::SystemTickBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(Modifier, AZ::SystemAllocator, 0);

            Modifier
                ( const ModifyConfiguration& modification
                , AZStd::vector<AZ::Data::AssetInfo>&& assets
                , AZStd::function<void()> onComplete);

        private:
            size_t m_assetIndex = 0;
            AZStd::function<void()> m_onComplete;
            ModifyConfiguration m_config;
            ModifyConfiguration m_result;

            void OnSystemTick() override;
        };
    }
}
