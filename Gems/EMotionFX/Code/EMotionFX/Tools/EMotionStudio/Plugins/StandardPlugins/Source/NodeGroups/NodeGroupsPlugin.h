/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <MysticQt/Source/DialogStack.h>
#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>

#include "NodeGroupWidget.h"
#include "NodeGroupManagementWidget.h"
#endif

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)


namespace EMStudio
{
    /**
     * plugin for editing node groups of the selected actor.
     */
    class NodeGroupsPlugin
        : public EMStudio::DockWidgetPlugin
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(NodeGroupsPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        enum
        {
            CLASS_ID = 0x00000055
        };

        NodeGroupsPlugin();
        ~NodeGroupsPlugin();

        // overloaded
        const char* GetName() const override                { return "Node Groups"; }
        uint32 GetClassID() const override                  { return NodeGroupsPlugin::CLASS_ID; }
        bool GetIsClosable() const override                 { return true;  }
        bool GetIsFloatable() const override                { return true;  }
        bool GetIsVertical() const override                 { return false; }

        // overloaded main init function
        bool Init() override;
        EMStudioPlugin* Clone() const override { return new NodeGroupsPlugin(); }

        // update the node groups window based on the current selection
        void ReInit();

        // clear all widgets from the window
        void Clear();

    public slots:
        void WindowReInit(bool visible);
        void UpdateInterface();

    private:
        // declare the callbacks
        MCORE_DEFINECOMMANDCALLBACK(CommandSelectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandUnselectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandClearSelectionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAdjustNodeGroupCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAddNodeGroupCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveNodeGroupCallback);

        CommandSelectCallback*                  m_selectCallback;
        CommandUnselectCallback*                m_unselectCallback;
        CommandClearSelectionCallback*          m_clearSelectionCallback;
        CommandAdjustNodeGroupCallback*         m_adjustNodeGroupCallback;
        CommandAddNodeGroupCallback*            m_addNodeGroupCallback;
        CommandRemoveNodeGroupCallback*         m_removeNodeGroupCallback;

        // current selected actor
        EMotionFX::Actor*                       m_currentActor;

        // the dialog stack widgets
        NodeGroupWidget*                        m_nodeGroupWidget;
        NodeGroupManagementWidget*              m_nodeGroupManagementWidget;

        // some qt stuff
        MysticQt::DialogStack*                  m_dialogStack;
        QLabel*                                 m_infoText;
    };
} // namespace EMStudio
