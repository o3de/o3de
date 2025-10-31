/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/Components/Widgets/Card.h>
#include <EMotionStudio/EMStudioSDK/Source/DockWidgetPlugin.h>
#include <EMotionStudio/EMStudioSDK/Source/MotionEventPresetManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/StandardPluginsConfig.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewPlugin.h>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#endif

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QShortcut)


namespace EMStudio
{
    class TimeViewPlugin;

    class MotionEventPresetsWidget
        : public AzQtComponents::Card
    {
        Q_OBJECT // AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(MotionEventPresetsWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MotionEventPresetsWidget(QWidget* parent, TimeViewPlugin* plugin);

        // overloaded main init function
        void Init();
        void UpdateInterface();

        QTableWidget* GetMotionEventPresetsTable()                                                          { return m_tableWidget; }

        bool CheckIfIsPresetReadyToDrop();

    public slots:
        void ReInit();
        void AddMotionEventPreset();
        void RemoveSelectedMotionEventPresets();
        void RemoveMotionEventPreset(uint32 index);
        void ClearMotionEventPresetsButton();
        void SavePresets(bool showSaveDialog = false);
        void SaveWithDialog();
        void LoadPresets(bool showDialog = true);
        void SelectionChanged()                                                                             { UpdateInterface(); }

    private:
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

        void ClearMotionEventPresets();
        void contextMenuEvent(QContextMenuEvent* event) override;

        // widgets
        class DragTableWidget
            : public QTableWidget
        {
        public:
            DragTableWidget(int rows = 0, int columns = 0, QWidget* parent = nullptr)
                : QTableWidget(rows, columns, parent)
            {
                this->setDragEnabled(true);
            }

        protected:
            void dragEnterEvent(QDragEnterEvent* event) override
            {
                event->acceptProposedAction();
            }
            void dragLeaveEvent(QDragLeaveEvent* event) override
            {
                event->accept();
            }

            void dragMoveEvent(QDragMoveEvent* event) override
            {
                event->accept();
            }
        };

        DragTableWidget* m_tableWidget = nullptr;
        QAction* m_addAction = nullptr;
        QAction* m_saveMenuAction = nullptr;
        QAction* m_saveAction = nullptr;
        QAction* m_saveAsAction = nullptr;
        QAction* m_loadAction = nullptr;
        TimeViewPlugin* m_timeViewPlugin = nullptr;
    };
} // namespace EMStudio
