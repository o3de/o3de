/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/StringFunc/StringFunc.h>
#include <Editor/InspectorBus.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionExtractionWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionListWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionRetargetingWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionPropertiesWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionWindowPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetsWindowPlugin.h>
#include <QPushButton>
#include <QVBoxLayout>

namespace EMStudio
{
    MotionPropertiesWindow::MotionPropertiesWindow(QWidget* parent, MotionWindowPlugin* motionWindowPlugin)
        : QWidget(parent)
        , m_motionWindowPlugin(motionWindowPlugin)
    {
        // motion properties
        QVBoxLayout* motionPropertiesLayout = new QVBoxLayout(this);
        motionPropertiesLayout->setMargin(0);
        motionPropertiesLayout->setSpacing(0);

        // add the motion extraction stack window
        m_motionExtractionWindow = new MotionExtractionWindow(this, m_motionWindowPlugin);
        m_motionExtractionWindow->Init();
        AddSubProperties(m_motionExtractionWindow);

        // add the motion retargeting stack window
        m_motionRetargetingWindow = new MotionRetargetingWindow(this, m_motionWindowPlugin);
        m_motionRetargetingWindow->Init();
        AddSubProperties(m_motionRetargetingWindow);

        FinalizeSubProperties();
    }

    MotionPropertiesWindow::~MotionPropertiesWindow()
    {
        // Clear the inspector in case this window is currently shown.
        EMStudio::InspectorRequestBus::Broadcast(&EMStudio::InspectorRequestBus::Events::ClearIfShown, this);
    }

    void MotionPropertiesWindow::UpdateMotions()
    {
        m_motionRetargetingWindow->UpdateMotions();
    }

    void MotionPropertiesWindow::UpdateInterface()
    {
        const CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();
        const bool hasSelectedMotions = selection.GetNumSelectedMotions() > 0;

        m_motionExtractionWindow->UpdateInterface();
        m_motionRetargetingWindow->UpdateInterface();

        AZStd::string motionFileName = "Motion";
        MotionWindowPlugin::MotionTableEntry* entry = hasSelectedMotions ? m_motionWindowPlugin->FindMotionEntryByID(selection.GetMotion(0)->GetID()) : nullptr;
        if (entry)
        {
            const EMotionFX::Motion* motion = entry->m_motion;
            AZ::StringFunc::Path::GetFullFileName(motion->GetFileName(), motionFileName);
        }

        EMStudio::InspectorRequestBus::Broadcast(&EMStudio::InspectorRequestBus::Events::UpdateWithHeader,
            motionFileName.c_str(),
            s_headerIcon,
            this);
    }

    void MotionPropertiesWindow::AddSubProperties(QWidget* widget)
    {
        layout()->addWidget(widget);
    }

    void MotionPropertiesWindow::FinalizeSubProperties()
    {
        layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
    }
} // namespace EMStudio
