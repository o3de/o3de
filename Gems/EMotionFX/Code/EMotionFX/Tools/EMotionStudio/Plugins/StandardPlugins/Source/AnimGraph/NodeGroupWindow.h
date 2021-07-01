/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/Array.h>

#include <AzCore/Math/Color.h>

#include <EMotionFX/Source/NodeGroup.h>

#include "../StandardPluginsConfig.h"
#include "AttributesWindow.h"
#include <QWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTableWidget>

#include <EMotionFX/CommandSystem/Source/AnimGraphConnectionCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphParameterCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphNodeCommands.h>
#endif

namespace AzQtComponents
{
    class FilteredSearchWidget;
}

namespace EMStudio
{
    // forward declarations
    class AnimGraphPlugin;

    class NodeGroupRenameWindow
        : public QDialog
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(NodeGroupRenameWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        NodeGroupRenameWindow(QWidget* parent, EMotionFX::AnimGraph* animGraph, const AZStd::string& nodeGroup);

    private slots:
        void TextEdited(const QString& text);
        void Accepted();

    private:
        EMotionFX::AnimGraph*   mAnimGraph;
        AZStd::string           mNodeGroup;
        QLineEdit*              mLineEdit;
        QPushButton*            mOKButton;
    };

    class NodeGroupWindow
        : public QWidget
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(NodeGroupWindow, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        NodeGroupWindow(AnimGraphPlugin* plugin);
        ~NodeGroupWindow();

        void Init();

    private slots:
        void OnAddNodeGroup();
        void OnRemoveSelectedGroups();
        void OnRenameSelectedNodeGroup();
        void OnClearNodeGroups();
        void OnIsVisible(int state, int row);
        //void OnNameEdited(QTableWidgetItem* item);
        void OnColorChanged(const AZ::Color& color);
        void OnItemChanged(QTableWidgetItem* item);
        //void OnCellChanged(int row, int column);
        void OnTextFilterChanged(const QString& text);
        void UpdateInterface();

    private:
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

        void contextMenuEvent(QContextMenuEvent* event) override;

        uint32 FindGroupIndexByWidget(QObject* widget) const;
        //bool ValidateName(EMotionFX::AnimGraphNodeGroup* nodeGroup, const char* newName) const;

        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphAddNodeGroupCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphAdjustNodeGroupCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphRemoveNodeGroupCallback);

        CommandAnimGraphAddNodeGroupCallback*      mCreateCallback;
        CommandAnimGraphAdjustNodeGroupCallback*   mAdjustCallback;
        CommandAnimGraphRemoveNodeGroupCallback*   mRemoveCallback;

        struct WidgetLookup
        {
            MCORE_MEMORYOBJECTCATEGORY(NodeGroupWindow::WidgetLookup, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
            QObject*    mWidget;
            uint32      mGroupIndex;

            WidgetLookup(QObject* widget, uint32 index)
            {
                mWidget     = widget;
                mGroupIndex = index;
            }
        };

        AnimGraphPlugin*               mPlugin;
        QTableWidget*                   mTableWidget;
        QVBoxLayout*                    mVerticalLayout;
        QAction*                        mAddAction;
        AzQtComponents::FilteredSearchWidget* m_searchWidget;
        AZStd::string                   m_searchWidgetText;
        MCore::Array<WidgetLookup>      mWidgetTable;
    };
} // namespace EMStudio
