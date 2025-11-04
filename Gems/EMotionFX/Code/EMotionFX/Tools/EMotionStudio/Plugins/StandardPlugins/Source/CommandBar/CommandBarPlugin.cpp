/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CommandBarPlugin.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <MCore/Source/LogManager.h>
#include <AzQtComponents/Components/Widgets/Slider.h>
#include <QAction>
#include <QDir>
#include <QLineEdit>
#include <QApplication>
#include <QAction>
#include <QLabel>
#include <QHBoxLayout>

namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(CommandBarPlugin::ProgressHandler, EMotionFX::EventHandlerAllocator)

    CommandBarPlugin::CommandBarPlugin()
        : EMStudio::ToolBarPlugin()
    {
    }

    CommandBarPlugin::~CommandBarPlugin()
    {
        delete m_lockEnabledIcon;
        delete m_lockDisabledIcon;

        if (m_toggleLockSelectionCallback)
        {
            GetCommandManager()->RemoveCommandCallback(m_toggleLockSelectionCallback, false);
            delete m_toggleLockSelectionCallback;
        }

        if (m_progressHandler)
        {
            EMotionFX::GetEventManager().RemoveEventHandler(m_progressHandler);
            delete m_progressHandler;
        }
    }

    const char* CommandBarPlugin::GetName() const
    {
        return "Command Bar";
    }

    uint32 CommandBarPlugin::GetClassID() const
    {
        return CommandBarPlugin::CLASS_ID;
    }

    // init after the parent dock window has been created
    bool CommandBarPlugin::Init()
    {
        m_toggleLockSelectionCallback = new CommandToggleLockSelectionCallback(false);
        GetCommandManager()->RegisterCommandCallback("ToggleLockSelection", m_toggleLockSelectionCallback);

        QDir dataDir{ QString(MysticQt::GetDataDir().c_str()) };
        m_lockEnabledIcon    = new QIcon(dataDir.filePath("Images/Icons/LockEnabled.svg"));
        m_lockDisabledIcon   = new QIcon(dataDir.filePath("Images/Icons/LockDisabled.svg"));

        m_commandEdit = new QLineEdit();
        m_commandEdit->setPlaceholderText("Enter command");
        connect(m_commandEdit, &QLineEdit::returnPressed, this, &CommandBarPlugin::OnEnter);
        m_commandEditAction = m_bar->addWidget(m_commandEdit);

        m_resultEdit = new QLineEdit();
        m_resultEdit->setReadOnly(true);
        m_commandResultAction = m_bar->addWidget(m_resultEdit);

        m_globalSimSpeedSlider = new AzQtComponents::SliderDouble(Qt::Horizontal);
        m_globalSimSpeedSlider->setMaximumWidth(80);
        m_globalSimSpeedSlider->setMinimumWidth(30);
        m_globalSimSpeedSlider->setRange(0.005, 2.0);
        m_globalSimSpeedSlider->setValue(1.0);
        m_globalSimSpeedSlider->setToolTip("The global simulation speed factor.\nA value of 1.0 means the normal speed, which is when the slider handle is in the center.\nPress the button on the right of this slider to reset to the normal speed.");
        connect(m_globalSimSpeedSlider, &AzQtComponents::SliderDouble::valueChanged, this, &CommandBarPlugin::OnGlobalSimSpeedChanged);
        m_globalSimSpeedSliderAction = m_bar->addWidget(m_globalSimSpeedSlider);

        m_globalSimSpeedResetAction = m_bar->addAction(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Reset.svg"),
            tr("Reset the global simulation speed factor to its normal speed"),
            this, &CommandBarPlugin::ResetGlobalSimSpeed);

        m_progressText = new QLabel();
        m_progressText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_progressText->setAlignment(Qt::AlignRight);
        m_progressText->setStyleSheet("padding-right: 1px; color: rgb(140, 140, 140);");
        m_progressTextAction = m_bar->addWidget(m_progressText);
        m_progressTextAction->setVisible(false);

        m_progressBar = new QProgressBar();
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(0);
        m_progressBar->setMaximumWidth(300);
        m_progressBar->setStyleSheet("padding-right: 2px;");
        m_progressBarAction = m_bar->addWidget(m_progressBar);
        m_progressBarAction->setVisible(false);

        m_lockSelectionAction = m_bar->addAction(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Reset.svg"),
            tr("Lock or unlock the selection of actor instances"),
            this, &CommandBarPlugin::OnLockSelectionButton);

        UpdateLockSelectionIcon();

        m_progressHandler = aznew ProgressHandler(this);
        EMotionFX::GetEventManager().AddEventHandler(m_progressHandler);

        return true;
    }

    void CommandBarPlugin::OnLockSelectionButton()
    {
        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand("ToggleLockSelection", result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        UpdateLockSelectionIcon();
    }


    void CommandBarPlugin::UpdateLockSelectionIcon()
    {
        if (EMStudio::GetCommandManager()->GetLockSelection())
        {
            m_lockSelectionAction->setIcon(*m_lockEnabledIcon);
        }
        else
        {
            m_lockSelectionAction->setIcon(*m_lockDisabledIcon);
        }
    }

    // execute the command when enter is pressed
    void CommandBarPlugin::OnEnter()
    {
        QLineEdit* edit = qobject_cast<QLineEdit*>(sender());
        assert(edit == m_commandEdit);

        // get the command string trimmed
        AZStd::string command = FromQtString(edit->text());
        AzFramework::StringFunc::TrimWhiteSpace(command, true, true);

        // don't do anything on an empty command
        if (command.size() == 0)
        {
            edit->clear();
            return;
        }

        // execute the command
        AZStd::string resultString;
        const bool success = EMStudio::GetCommandManager()->ExecuteCommand(command.c_str(), resultString);

        // there was an error
        if (success == false)
        {
            MCore::LogError(resultString.c_str());
            m_resultEdit->setStyleSheet("color: red;");
            m_resultEdit->setText(resultString.c_str());
            m_resultEdit->setToolTip(resultString.c_str());
        }
        else // no error
        {
            m_resultEdit->setStyleSheet("color: rgb(0,255,0);");
            m_resultEdit->setText(resultString.c_str());
        }

        // clear the text of the edit box
        edit->clear();
    }

    void CommandBarPlugin::OnProgressStart()
    {
        m_progressBarAction->setVisible(true);
        m_progressTextAction->setVisible(true);

        m_commandEditAction->setVisible(false);
        m_commandResultAction->setVisible(false);

        m_globalSimSpeedSliderAction->setVisible(false);
        m_globalSimSpeedResetAction->setVisible(false);
        m_lockSelectionAction->setVisible(false);

        EMStudio::GetApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    void CommandBarPlugin::OnProgressEnd()
    {
        m_progressBarAction->setVisible(false);
        m_progressTextAction->setVisible(false);

        m_commandEditAction->setVisible(true);
        m_commandResultAction->setVisible(true);

        m_globalSimSpeedSliderAction->setVisible(true);
        m_globalSimSpeedResetAction->setVisible(true);
        m_lockSelectionAction->setVisible(true);

        EMStudio::GetApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    void CommandBarPlugin::OnProgressText(const char* text)
    {
        m_progressText->setText(text);
        EMStudio::GetApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    void CommandBarPlugin::OnProgressValue(float percentage)
    {
        m_progressBar->setValue(aznumeric_cast<int>(percentage));
        EMStudio::GetApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    void CommandBarPlugin::ResetGlobalSimSpeed()
    {
        m_globalSimSpeedSlider->setValue(1.0);
    }

    void CommandBarPlugin::OnGlobalSimSpeedChanged(double value)
    {
        EMotionFX::GetEMotionFX().SetGlobalSimulationSpeed(aznumeric_cast<float>(value));
    }

    //-----------------------------------------------------------------------------------------
    // command callbacks
    //-----------------------------------------------------------------------------------------

    bool UpdateInterfaceCommandBarPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(CommandBarPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        CommandBarPlugin* commandBarPlugin = (CommandBarPlugin*)plugin;
        commandBarPlugin->UpdateLockSelectionIcon();
        return true;
    }

    bool CommandBarPlugin::CommandToggleLockSelectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)  { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateInterfaceCommandBarPlugin(); }
    bool CommandBarPlugin::CommandToggleLockSelectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)     { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateInterfaceCommandBarPlugin(); }
} // namespace EMStudio
