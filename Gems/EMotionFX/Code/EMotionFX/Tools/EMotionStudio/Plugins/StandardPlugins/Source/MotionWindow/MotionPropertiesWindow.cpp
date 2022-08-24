/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/StringFunc/StringFunc.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionExtractionWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionPropertiesWindow.h>
#include <Editor/InspectorBus.h>
#include <QPushButton>
#include <QVBoxLayout>

namespace EMStudio
{
    MotionPropertiesWindow::MotionPropertiesWindow(QWidget* parent)
        : QWidget(parent)
    {
        QVBoxLayout* motionPropertiesLayout = new QVBoxLayout(this);
        motionPropertiesLayout->setMargin(0);
        motionPropertiesLayout->setSpacing(0);

        m_motionExtractionWindow = new MotionExtractionWindow(this);
        m_motionExtractionWindow->Init();
        AddSubProperties(m_motionExtractionWindow);

        FinalizeSubProperties();

        GetCommandManager()->RegisterCommandCallback<CommandSelectCallback>("Select", m_callbacks, this, false);
    }

    MotionPropertiesWindow::~MotionPropertiesWindow()
    {
        // Clear the inspector in case this window is currently shown.
        EMStudio::InspectorRequestBus::Broadcast(&EMStudio::InspectorRequestBus::Events::ClearIfShown, this);

        for (auto* callback : m_callbacks)
        {
            GetCommandManager()->RemoveCommandCallback(callback, true);
        }
        m_callbacks.clear();
    }

    void MotionPropertiesWindow::UpdateInterface()
    {
        const CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();
        const bool hasSelectedMotions = selection.GetNumSelectedMotions() > 0;

        m_motionExtractionWindow->UpdateInterface();

        AZStd::string motionFileName = "Motion";
        if (hasSelectedMotions)
        {
            const EMotionFX::Motion* motion = selection.GetMotion(0);
            AZ::StringFunc::Path::GetFullFileName(motion->GetFileName(), motionFileName);
        }

        EMStudio::InspectorRequestBus::Broadcast(
            &EMStudio::InspectorRequestBus::Events::UpdateWithHeader, motionFileName.c_str(), s_headerIcon, this);
    }

    void MotionPropertiesWindow::AddSubProperties(QWidget* widget)
    {
        layout()->addWidget(widget);
    }

    void MotionPropertiesWindow::FinalizeSubProperties()
    {
        layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
    }

    MotionPropertiesWindow::CommandSelectCallback::CommandSelectCallback(
        MotionPropertiesWindow* window, bool executePreUndo, bool executePreCommand)
        : MCore::Command::Callback(executePreUndo, executePreCommand)
        , m_window(window)
    {
    }

    bool MotionPropertiesWindow::CommandSelectCallback::Execute(
        [[maybe_unused]] MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        if (CommandSystem::CheckIfHasMotionSelectionParameter(commandLine))
        {
            m_window->UpdateInterface();
        }

        return true;
    }

    bool MotionPropertiesWindow::CommandSelectCallback::Undo(
        [[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        return true;
    }
} // namespace EMStudio
