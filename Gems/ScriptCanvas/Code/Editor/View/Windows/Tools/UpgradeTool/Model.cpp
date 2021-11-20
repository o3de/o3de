/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManagerBus.h>
#include <CryCommon/CrySystemBus.h>
#include <Editor/Assets/ScriptCanvasAssetHelpers.h>
#include <Editor/View/Windows/Tools/UpgradeTool/Model.h>
#include <IConsole.h>
#include <ISystem.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>

namespace ModifierCpp
{

}

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        EditorKeepAlive::EditorKeepAlive()
        {
            ISystem* system = nullptr;
            CrySystemRequestBus::BroadcastResult(system, &CrySystemRequestBus::Events::GetCrySystem);

            if (system)
            {
                m_edKeepEditorActive = system->GetIConsole()->GetCVar("ed_KeepEditorActive");

                if (m_edKeepEditorActive)
                {
                    m_keepEditorActive = m_edKeepEditorActive->GetIVal();
                    m_edKeepEditorActive->Set(1);
                }
            }            
        }

        EditorKeepAlive::~EditorKeepAlive()
        {
            if (m_edKeepEditorActive)
            {
                m_edKeepEditorActive->Set(m_keepEditorActive);
            }
        }

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

        const ModificationResults* Model::GetResults()
        {
            return !IsWorking() ? &m_modResults : nullptr;
        }

        void Model::Idle()
        {
            m_state = State::Idle;
            m_keepEditorAlive.reset();
            m_log.Deactivate();
        }

        bool Model::IsReadyToModify() const
        {
            if (IsWorking())
            {
                return false;
            }

            if (!m_scanner)
            {
                return false;
            }

            return !m_scanner->GetResult().m_unfiltered.empty();
        }

        bool Model::IsWorking() const
        {
            return m_state != State::Idle;
        }

        void Model::Modify(const ModifyConfiguration& modification)
        {
            if (!IsReadyToModify())
            {
                AZ_Warning(ScriptCanvas::k_VersionExplorerWindow.data(), false, "Explorer is not ready to modify graphs.");
                return;
            }

            if (modification.modifySingleAsset.m_assetId.IsValid())
            {
                const auto& results = m_scanner->GetResult();
                auto iter = AZStd::find_if
                    ( results.m_unfiltered.begin()
                    , results.m_unfiltered.end()
                    , [&modification](const auto& candidate)
                    {
                        return candidate.info.m_assetId == modification.modifySingleAsset.m_assetId;
                    });

                if (iter == results.m_unfiltered.end())
                {
                    AZ_Warning(ScriptCanvas::k_VersionExplorerWindow.data(), false, "Requested upgrade graph not found in scanned list.");
                    return;
                }


                m_state = State::ModifySingle;
                m_modifier = AZStd::make_unique<Modifier>(modification, WorkingAssets{ *iter }, [this]() { OnModificationComplete(); });
            }
            else
            {
                auto results = m_scanner->TakeResult();
                m_state = State::ModifyAll;
                m_modifier = AZStd::make_unique<Modifier>(modification, AZStd::move(results.m_unfiltered), [this]() { OnModificationComplete(); });
            }

            m_modResults = {};
            m_log.Activate();
            m_keepEditorAlive = AZStd::make_unique<EditorKeepAlive>();            
        }

        void Model::OnModificationComplete()
        {
            ModelNotificationsBus::Broadcast(&ModelNotificationsTraits::OnUpgradeComplete, m_modifier->GetResult());
            m_modifier.reset();

            if (m_state == State::ModifyAll)
            {
                m_scanner.reset();
            }

            Idle();
        }

        void Model::OnScanComplete()
        {
            ModelNotificationsBus::Broadcast(&ModelNotificationsTraits::OnScanComplete, m_scanner->GetResult());
            Idle();
        }

        void Model::Scan(const ScanConfiguration& config)
        {
            if (IsWorking())
            {
                AZ_Warning(ScriptCanvas::k_VersionExplorerWindow.data(), false, "Explorer is already working");
                return;
            }

            m_state = State::Scanning;
            m_log.Activate();
            m_keepEditorAlive = AZStd::make_unique<EditorKeepAlive>();
            m_scanner = AZStd::make_unique<Scanner>(config, [this](){ OnScanComplete(); });
        }

        void Model::RestoreSettings()
        {
            m_settingsCache.reset();
        }
    }
}
