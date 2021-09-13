/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManagerBus.h>
#include <Editor/View/Windows/Tools/UpgradeTool/Model.h>
#include <Editor/Assets/ScriptCanvasAssetHelpers.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>

namespace ModifierCpp
{

}

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        Model::Model()
        {
            ModelRequestsBus::Handler::BusConnect();
        }

        void Model::CacheSettings()
        {
            m_settingsCache = AZStd::make_unique<ScriptCanvas::Grammar::SettingsCache>();
            ScriptCanvas::Grammar::g_saveRawTranslationOuputToFile = false;
            ScriptCanvas::Grammar::g_printAbstractCodeModel = false;
            ScriptCanvas::Grammar::g_saveRawTranslationOuputToFile = false;
        }

        const AZStd::vector<AZStd::string>* Model::GetLogs()
        {
            return &m_log.GetEntries();
        }
        
        bool Model::IsWorking() const
        {
            return m_state != State::Idle;
        }

        void Model::OnScanComplete()
        {
            ModelNotificationsBus::Broadcast(&ModelNotificationsTraits::OnScanComplete, m_scanner->GetResult());
            m_state = State::Idle;
        }

        void Model::Scan(const ScanConfiguration& config)
        {
            if (IsWorking())
            {
                AZ_Warning(ScriptCanvas::k_VersionExplorerWindow.data(), false, "Explorer is already working");
                return;
            }

            m_state = State::Scanning;
            m_scanner = AZStd::make_unique<Scanner>(config, [this](){ OnScanComplete(); });
        }

        void Model::RestoreSettings()
        {
            m_settingsCache.reset();
        }
    }
}
