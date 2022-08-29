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
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <SceneAPI/SceneUI/SceneUIConfiguration.h>
#endif

namespace AzQtComponents
{
    class StyledBusyLabel;
    class StyledDetailsTableModel;
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

class QPushButton;

class SceneSettingsCardHeader : public AzQtComponents::CardHeader
{
    Q_OBJECT
public:
    SceneSettingsCardHeader(QWidget* parent = nullptr);

    void SetCanClose(bool canClose);

private:
    void triggerCloseButton();

    AzQtComponents::StyledBusyLabel* m_busyLabel = nullptr;
    QPushButton* m_closeButton = nullptr;
};

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
class SCENE_UI_API SceneSettingsCard : public AzQtComponents::Card, public AZ::Debug::TraceMessageBus::Handler
{
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
    Q_OBJECT
public:
    SceneSettingsCard(AZ::Uuid traceTag, QWidget* parent = nullptr);
    ~SceneSettingsCard();

    void SetAndStartProcessingHandler(const AZStd::shared_ptr<AZ::SceneAPI::SceneUI::ProcessingHandler>& handler);
    
    bool OnPrintf(const char* window, const char* message) override;
    bool OnError(const char* window, const char* message) override;
    bool OnWarning(const char* window, const char* message) override;
    bool OnAssert(const char* message) override;

    enum class SceneSettingsCardState
    {
        Loading,
        Processing,
        Done,
    };
    void SetState(SceneSettingsCardState newState);

    
    public slots:
    //void AddLogEntry(const AzToolsFramework::Logging::LogEntry& logEntry);
    void OnSetStatusMessage(const AZStd::string& message);
    void OnProcessingComplete();
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

    QString GetTimeAsString();
    
    AzToolsFramework::Debug::TraceContextMultiStackHandler m_traceStackHandler;
    AZ::Uuid m_traceTag;
    AzQtComponents::StyledDetailsTableModel* m_reportModel = nullptr;
    AZStd::shared_ptr<AZ::SceneAPI::SceneUI::ProcessingHandler> m_targetHandler;
    SceneSettingsCardHeader* m_settingsHeader = nullptr;
    CompletionState m_completionState = CompletionState::Success;
};
