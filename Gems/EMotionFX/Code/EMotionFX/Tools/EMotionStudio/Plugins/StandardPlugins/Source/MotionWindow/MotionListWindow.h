/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <MCore/Source/MemoryCategoriesCore.h>
#include <AzCore/std/containers/vector.h>
#include "../StandardPluginsConfig.h"
#include "MotionWindowPlugin.h"
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionData/MotionData.h>
#include <EMotionFX/Source/PlayBackInfo.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/SelectionList.h>
#include <QWidget>
#include <QTableWidget>
#include <QDialog>
#endif


// forward declarations
QT_FORWARD_DECLARE_CLASS(QContextMenuEvent)
QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace EMStudio
{
    // forward declarations
    class MotionWindowPlugin;


    class MotionListRemoveMotionsFailedWindow
        : public QDialog
    {
        MCORE_MEMORYOBJECTCATEGORY(MotionListRemoveMotionsFailedWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MotionListRemoveMotionsFailedWindow(QWidget* parent, const AZStd::vector<EMotionFX::Motion*>& motions);
    };


    class MotionTableWidget
        : public QTableWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(MotionTableWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MotionTableWidget(MotionWindowPlugin* parentPlugin, QWidget* parent);
        virtual ~MotionTableWidget();

    protected:
        // used for drag and drop support for the blend tree
        QMimeData* mimeData(const QList<QTableWidgetItem*> items) const override;
        QStringList mimeTypes() const override;
        Qt::DropActions supportedDropActions() const override;

        MotionWindowPlugin* m_plugin;
    };


    class MotionListWindow
        : public QWidget
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(MotionListWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MotionListWindow(QWidget* parent, MotionWindowPlugin* motionWindowPlugin);
        ~MotionListWindow();

        void Init();
        void ReInit();
        void UpdateInterface();

        QTableWidget* GetMotionTable() { return m_motionTable; }

        bool AddMotionByID(uint32 motionID);
        bool RemoveMotionByID(uint32 motionID);
        uint32 GetMotionID(uint32 rowIndex);
        uint32 FindRowByMotionID(uint32 motionID);
        bool CheckIfIsMotionVisible(MotionWindowPlugin::MotionTableEntry* entry);
        void OnTextFilterChanged(const QString& text);

    signals:
        void MotionSelectionChanged();
        void SaveRequested();
        void RemoveMotionsRequested();

    private slots:
        void cellDoubleClicked(int row, int column);
        void itemSelectionChanged();
        void OnAddMotionsInSelectedMotionSets();
        void OnOpenInFileBrowser();

    private:
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

        void contextMenuEvent(QContextMenuEvent* event) override;
        void UpdateSelection(const CommandSystem::SelectionList& selectionList);

    private:
        AZStd::vector<uint32>                                   m_selectedMotionIDs;
        AZStd::vector<MotionWindowPlugin::MotionTableEntry*>    m_shownMotionEntries;
        QVBoxLayout*                                            m_vLayout;
        MotionTableWidget*                                      m_motionTable;
        MotionWindowPlugin*                                     m_motionWindowPlugin;
        AZStd::string                                           m_searchWidgetText;
    };

} // namespace EMStudio
