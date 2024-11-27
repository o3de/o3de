/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/Debug/TraceContextMultiStackHandler.h>
#include <AzQtComponents/Components/StyledDetailsTableModel.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <SceneAPI/SceneUI/SceneUIConfiguration.h>
#include <QMap>
#include <QPair>
#include <QString>
#include <QVector>
#endif

class QSvgWidget;

namespace AzQtComponents
{
    class StyledDetailsTableView;
    class TableView;
}

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            class ProcessingHandler;
        }
    }
}

namespace AzToolsFramework
{
    namespace Logging
    {
        class LogEntry;
    }
}


class QPushButton;

class SceneSettingsCardHeader : public AzQtComponents::CardHeader
{
    Q_OBJECT
public:
    SceneSettingsCardHeader(QWidget* parent = nullptr);

    void SetCanClose(bool canClose);

private:
    void triggerCloseButton();

    QPushButton* m_closeButton = nullptr;
    QSvgWidget* m_busySpinner = nullptr;
    
    friend class SceneSettingsCard;
};

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
class SCENE_UI_API SceneSettingsCard : public AzQtComponents::Card, public AZ::Debug::TraceMessageBus::Handler
{
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
    Q_OBJECT
public:
    enum class Layout
    {
        Loading,
        Resetting,
        Exporting
    };

    SceneSettingsCard(AZ::Uuid traceTag, QString fileTracked, Layout layout, QWidget* parent = nullptr);
    ~SceneSettingsCard();

    void SetAndStartProcessingHandler(const AZStd::shared_ptr<AZ::SceneAPI::SceneUI::ProcessingHandler>& handler);
    
    bool OnPrintf(const char* window, const char* message) override;
    bool OnError(const char* window, const char* message) override;
    bool OnWarning(const char* window, const char* message) override;
    bool OnAssert(const char* message) override;

    enum class State
    {
        Loading,
        Processing,
        Done,
    };
    void SetState(State newState);

    
public Q_SLOTS:
    void AddLogEntry(const AzToolsFramework::Logging::LogEntry& logEntry);
    void OnSetStatusMessage(const AZStd::string& message);
    void OnProcessingComplete();
    
signals:
    void ProcessingCompleted();

private:
    enum class CompletionState
    {
        Success,
        Warning,
        Error,
        Failure
    };

    bool ShouldProcessMessage();
    void UpdateCompletionState(CompletionState newState);
    void CopyTraceContext(AzQtComponents::StyledDetailsTableModel::TableEntry& entry) const;
    QString GetTimeNowAsString();
    void ShowLogContextMenu(const QPoint& pos);
    void AddLogTableEntry(AzQtComponents::StyledDetailsTableModel::TableEntry& entry);

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QMap<int, QVector<QPair<QString, QString>>> m_additionalLogDetails;
    
    AzToolsFramework::Debug::TraceContextMultiStackHandler m_traceStackHandler;
    AZ::Uuid m_traceTag;
    AzQtComponents::StyledDetailsTableModel* m_reportModel = nullptr;
    AzQtComponents::TableView* m_reportView = nullptr;
    AZStd::shared_ptr<AZ::SceneAPI::SceneUI::ProcessingHandler> m_targetHandler;
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    SceneSettingsCardHeader* m_settingsHeader = nullptr;
    CompletionState m_completionState = CompletionState::Success;
    State m_sceneCardState = State::Loading;
    QString m_fileTracked;
    int m_warningCount = 0;
    int m_errorCount = 0;
};
