/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Asset/AssetCatalogBus.h>
#include <Editor/View/Windows/Tools/UpgradeTool/ModelTraits.h>
#include <Editor/View/Windows/Tools/UpgradeTool/Modifier.h>
#include <Editor/View/Windows/Tools/UpgradeTool/Scanner.h>
#include <Editor/View/Windows/Tools/UpgradeTool/VersionExplorerLog.h>
#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        //! Handles model change requests, state queries; sends state change notifications
        class Model
            : public ModelRequestsBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(Model, AZ::SystemAllocator, 0);

            Model();

            const AZStd::vector<AZStd::string>* GetLogs();
            void Scan(const ScanConfiguration& config) override;

        private:
            enum class State 
            {
                Idle,
                Scanning,
                Modifying,
            };

            State m_state = State::Idle;
            Log m_log;
            
            AZStd::unique_ptr<Modifier> m_modifier;
            AZStd::unique_ptr<Scanner> m_scanner;
            AZStd::unique_ptr<ScriptCanvas::Grammar::SettingsCache> m_settingsCache;

            void CacheSettings();
            bool IsWorking() const;
            void OnScanComplete();
            void RestoreSettings();
        };
    }
}
