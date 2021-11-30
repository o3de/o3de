/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QAbstractListModel>

#include <AzCore/UserSettings/UserSettings.h>

#include <AzFramework/TargetManagement/TargetManagementAPI.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

#include <Editor/View/Widgets/LoggingPanel/LoggingWindowSession.h>
#include <Editor/View/Widgets/LoggingPanel/LiveWindowSession/LiveLoggingDataAggregator.h>
#endif

namespace ScriptCanvasEditor
{
    class TargetManagerModel
        : public QAbstractListModel
    {
    public:

        AZ_CLASS_ALLOCATOR(TargetManagerModel, AZ::SystemAllocator, 0);

        TargetManagerModel();

        // QAbstarctItemModel
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        ////

        void TargetJoinedNetwork(AzFramework::TargetInfo info);
        void TargetLeftNetwork(AzFramework::TargetInfo info);

        AzFramework::TargetInfo FindTargetInfoForRow(int row);
        int GetRowForTarget(AZ::u32 targetId);

    private:

        void ScrapeTargetInfo();

        AzFramework::TargetInfo                  m_selfInfo;
        AZStd::vector< AzFramework::TargetInfo > m_targetInfo;
    };

    class LiveLoggingUserSettings
        : public AZ::UserSettings
    {
    public:
        static AZStd::intrusive_ptr<LiveLoggingUserSettings> FindSettingsInstance();

        AZ_RTTI(LiveLoggingUserSettings, "{2E32C949-5766-480D-B569-781BE9166B2E}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(LiveLoggingUserSettings, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);

        LiveLoggingUserSettings() = default;

        void SetAutoCaptureEnabled(bool enabled);
        bool IsAutoCaptureEnabled() const;

        void SetLiveUpdates(bool enabled);
        bool IsLiveUpdating() const;

    private:

        bool m_isAutoCaptureEnabled = true;
        bool m_enableLiveUpdates = true;
    };
    
    class LiveLoggingWindowSession
        : public LoggingWindowSession
        , public AzFramework::TargetManagerClient::Bus::Handler
        , public AzToolsFramework::EditorEntityContextNotificationBus::Handler
        , public ScriptCanvas::Debugger::ServiceNotificationsBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(LiveLoggingWindowSession, AZ::SystemAllocator, 0);
        
        LiveLoggingWindowSession(QWidget* parent = nullptr);
        ~LiveLoggingWindowSession() override;
        
        // AzFramework::TargetManagerClient
        void DesiredTargetChanged(AZ::u32 newId, AZ::u32 oldId) override;
        void DesiredTargetConnected(bool connected) override;

        void TargetJoinedNetwork(AzFramework::TargetInfo info) override;
        void TargetLeftNetwork(AzFramework::TargetInfo info) override;
        ////

        // AzToolsFramework::EditorEntityContextNotificationBus::Handler
        void OnStartPlayInEditorBegin() override;
        void OnStopPlayInEditor() override;
        ////

        // ScriptCavnas::Debugger::ServiceNotificationsBus
        void Connected(const ScriptCanvas::Debugger::Target& target) override;
        ////
        
    protected:

        void OnCaptureButtonPressed() override;
        void OnPlaybackButtonPressed() override;
        void OnOptionsButtonPressed() override;
    
        void OnTargetChanged(int currentIndex) override;
        
    private:

        void OnAutoCaptureToggled(bool checked);
        void OnLiveUpdateToggled(bool checked);

        void StartDataCapture();
        void StopDataCapture();

        void ConfigureScriptTarget(ScriptCanvas::Debugger::ScriptTarget& captureInfo);

        void SetIsCapturing(bool isCapturing);

        TargetManagerModel* m_targetManagerModel;

        bool m_startedSession;
        bool m_encodeStaticEntities;
        
        bool m_isCapturing;
        LiveLoggingDataAggregator m_liveDataAggregator;

        ScriptCanvas::Debugger::Target m_targetConfiguration;
        AZStd::intrusive_ptr<LiveLoggingUserSettings> m_userSettings;
    };
}
