/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "../StandardPluginsConfig.h"
#include <MCore/Source/StandardHeaders.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include "../StandardPluginsConfig.h"
#include "ParameterWidget.h"
#include <QDialog>
#endif


namespace EMStudio
{
    /*
     * How to use this dialog?
     * 1. Use the rejected() signal to catch when the X on the window or the cancel button is pressed.
     * 2. Use the itemSelectionChanged() signal of the GetNodeHierarchyWidget()->GetTreeWidget() to detect when the user adjusts the selection in the node hierarchy widget.
     * 3. Use the OnSelectionDone() in the GetNodeHierarchyWidget() to detect when the user finished selecting and pressed the OK button.
     * Example:
     * connect( m_parameterSelectionWindow,                                              SIGNAL(rejected()),             this, SLOT(UserWantsToCancel_1()) );
     * connect( m_parameterSelectionWindow->GetNodeHierarchyWidget()->GetTreeWidget(),   SIGNAL(itemSelectionChanged()), this, SLOT(SelectionChanged_2()) );
     * connect( m_parameterSelectionWindow->GetNodeHierarchyWidget(),                    SIGNAL(OnSelectionDone(AZStd::vector<SelectionItem>)), this, SLOT(FinishedSelectionAndPressedOK_3(AZStd::vector<SelectionItem>)) );
    */
    class ParameterSelectionWindow
        : public QDialog
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(ParameterSelectionWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH)

    public:
        ParameterSelectionWindow(QWidget* parent, bool useSingleSelection);
        virtual ~ParameterSelectionWindow();

        MCORE_INLINE ParameterWidget* GetParameterWidget()                                                      { return m_parameterWidget; }
        void Update(EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& selectedParameters)    { m_parameterWidget->Update(animGraph, selectedParameters); }

    public slots:
        void OnAccept();
        void OnDoubleClicked(const AZStd::string& item);

    private:
        ParameterWidget*                    m_parameterWidget;
        QPushButton*                        m_okButton;
        QPushButton*                        m_cancelButton;
        bool                                m_useSingleSelection;
        bool                                m_accepted;
    };
} // namespace EMStudio
