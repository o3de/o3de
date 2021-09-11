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
#include <Editor/View/Windows/Tools/UpgradeTool/VersionExplorerLog.h>

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

            const AZStd::vector<AZStd::string>* GetLogs();

        private:
            Log m_log;

            AZStd::unique_ptr<ScriptCanvas::Grammar::SettingsCache> m_settingsCache;
        };
    }
}
