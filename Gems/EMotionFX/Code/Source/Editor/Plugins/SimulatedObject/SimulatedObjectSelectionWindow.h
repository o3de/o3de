/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <MCore/Source/StandardHeaders.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <Editor/Plugins/SimulatedObject/SimulatedObjectSelectionWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/StandardPluginsConfig.h>
#include <QDialog>
#endif


namespace EMStudio
{
    class SimulatedObjectSelectionWindow
        : public QDialog
    {
        Q_OBJECT // AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(SimulatedObjectSelectionWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH)

    public:
        SimulatedObjectSelectionWindow(QWidget* parent);

        SimulatedObjectSelectionWidget* GetSimulatedObjectSelectionWidget()                                     { return m_simulatedObjectSelectionWidget; }
        void Update(EMotionFX::Actor* actor, const AZStd::vector<AZStd::string>& selectedSimulatedObjects)      { m_simulatedObjectSelectionWidget->Update(actor, selectedSimulatedObjects); }

    private:
        SimulatedObjectSelectionWidget*     m_simulatedObjectSelectionWidget = nullptr;
        QPushButton*                        m_OKButton = nullptr;
        QPushButton*                        m_cancelButton = nullptr;
        bool                                m_accepted = false;
    };
} // namespace EMStudio
