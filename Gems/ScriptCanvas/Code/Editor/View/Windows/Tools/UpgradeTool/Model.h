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

struct ICVar;

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        //! Scoped utility to set and restore the "ed_KeepEditorActive" CVar in order to allow
        //! the upgrade tool to work even if the editor is not in the foreground
        class EditorKeepAlive
        {
        public:
            EditorKeepAlive();
            ~EditorKeepAlive();

        private:
            int m_keepEditorActive;
            ICVar* m_edKeepEditorActive;
        };

        //! Handles model change requests, state queries; sends state change notifications
        class Model
            : public ModelRequestsBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(Model, AZ::SystemAllocator, 0);

            Model();

            const ModificationResults* GetResults() override;

            void Modify(const ModifyConfiguration& modification) override;

            void Scan(const ScanConfiguration& config) override;

        private:
            enum class State 
            {
                Idle,
                Scanning,
                ModifyAll,
                ModifySingle
            };

            State m_state = State::Idle;
            Log m_log;

            // these two are managed by the same class because the modifer will only operate on the results of the scanner
            AZStd::unique_ptr<Modifier> m_modifier;
            AZStd::unique_ptr<Scanner> m_scanner;
            AZStd::unique_ptr<ScriptCanvas::Grammar::SettingsCache> m_settingsCache;
            AZStd::unique_ptr<EditorKeepAlive> m_keepEditorAlive;

            ModificationResults m_modResults;

            void CacheSettings();
            void Idle();
            bool IsReadyToModify() const;
            bool IsWorking() const;
            void OnModificationComplete();
            void OnScanComplete();
            void RestoreSettings();
        };
    }
}
