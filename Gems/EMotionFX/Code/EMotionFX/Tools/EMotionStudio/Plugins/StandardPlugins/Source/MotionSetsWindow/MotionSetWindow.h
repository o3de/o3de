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
#include <AzCore/std/containers/vector.h>
#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#include <MysticQt/Source/DialogStack.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/MotionSetCommands.h>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QTreeWidget>
#include <QLineEdit>
#include <QDialog>
#endif

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QComboBox)

namespace AzQtComponents
{
    class FilteredSearchWidget;
}

namespace EMStudio
{
    // forward declaration
    class MotionSetsWindowPlugin;

    class MotionSetRemoveMotionsFailedWindow
        : public QDialog
    {
        MCORE_MEMORYOBJECTCATEGORY(MotionSetRemoveMotionsFailedWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MotionSetRemoveMotionsFailedWindow(QWidget* parent, const AZStd::vector<EMotionFX::Motion*>& motions);
    };


    class RenameMotionEntryWindow
        : public QDialog
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(RenameMotionEntryWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        RenameMotionEntryWindow(QWidget* parent, EMotionFX::MotionSet* motionSet, const AZStd::string& motionId);

    private slots:
        void TextEdited(const QString& text);
        void Accepted();

    private:
        EMotionFX::MotionSet*           m_motionSet;
        AZStd::vector<AZStd::string>    m_existingIds;
        AZStd::string                   m_motionId;
        QLineEdit*                      m_lineEdit;
        QPushButton*                    m_okButton;
    };


    class MotionEditStringIDWindow
        : public QDialog
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(MotionEditStringIDWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        MotionEditStringIDWindow(QWidget* parent, EMotionFX::MotionSet* motionSet, const AZStd::vector<AZStd::string>& motionIDs);

    public slots:
        void Accepted();
        void StringABChanged(const QString& text);
        void CurrentIndexChanged(int index);

    private:
        void UpdateTableAndButton();

    private:
        EMotionFX::MotionSet*           m_motionSet;
        AZStd::vector<AZStd::string>    m_motionIDs;
        AZStd::vector<AZStd::string>    m_modifiedMotionIDs;
        AZStd::vector<size_t>           m_motionToModifiedMap;
        AZStd::vector<size_t>           m_valids;
        QTableWidget*                   m_tableWidget;
        QLineEdit*                      m_stringALineEdit;
        QLineEdit*                      m_stringBLineEdit;
        QPushButton*                    m_applyButton;
        QLabel*                         m_numMotionIDsLabel;
        QLabel*                         m_numModifiedIDsLabel;
        QLabel*                         m_numDuplicateIDsLabel;
        QComboBox*                      m_comboBox;
    };


    class MotionSetTableWidget
        : public QTableWidget
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(MotionSetTableWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MotionSetTableWidget(MotionSetsWindowPlugin* parentPlugin, QWidget* parent);
        virtual ~MotionSetTableWidget();

    protected:
        // used for drag and drop support for the blend tree
        QMimeData* mimeData(const QList<QTableWidgetItem*> items) const override;
        QStringList mimeTypes() const override;
        Qt::DropActions supportedDropActions() const override;

        void dropEvent(QDropEvent* event) override;
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dragMoveEvent(QDragMoveEvent* event) override;

        MotionSetsWindowPlugin* m_plugin;
    };


    class MotionSetWindow
        : public QWidget
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(MotionSetWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MotionSetWindow(MotionSetsWindowPlugin* parentPlugin, QWidget* parent);
        ~MotionSetWindow();

        bool Init();
        void ReInit();

        bool AddMotion(EMotionFX::MotionSet* motionSet, EMotionFX::MotionSet::MotionEntry* motionEntry);
        bool UpdateMotion(EMotionFX::MotionSet* motionSet, EMotionFX::MotionSet::MotionEntry* motionEntry, const AZStd::string& oldMotionId);
        bool RemoveMotion(EMotionFX::MotionSet* motionSet, EMotionFX::MotionSet::MotionEntry* motionEntry);
        void PlayMotion(EMotionFX::Motion* motion);

    signals:
        void MotionSelectionChanged();

    public slots:
        void UpdateInterface();

        void OnAddNewEntry();
        void OnLoadEntries();
        void OnRemoveMotions();
        void RenameEntry(QTableWidgetItem* item);
        void OnRenameEntry();
        void OnUnassignMotions();
        void OnCopyMotionID();
        void OnClearMotions();
        void OnEditButton();

        void OnTextFilterChanged(const QString& text);

        void OnEntryDoubleClicked(QTableWidgetItem* item);

        void dropEvent(QDropEvent* event) override;
        void dragEnterEvent(QDragEnterEvent* event) override;

    private:
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

        bool InsertRow(EMotionFX::MotionSet* motionSet, EMotionFX::MotionSet::MotionEntry* motionEntry, QTableWidget* tableWidget, bool readOnly);
        bool FillRow(EMotionFX::MotionSet* motionSet, EMotionFX::MotionSet::MotionEntry* motionEntry, uint32 rowIndex, QTableWidget* tableWidget, bool readOnly);
        bool RemoveRow(EMotionFX::MotionSet* motionSet, EMotionFX::MotionSet::MotionEntry* motionEntry, QTableWidget* tableWidget);

        void UpdateMotionSetTable(QTableWidget* tableWidget, EMotionFX::MotionSet* motionSet, bool readOnly = false);
        void AddMotions(const AZStd::vector<AZStd::string>& filenames);
        void contextMenuEvent(QContextMenuEvent* event) override;

        EMotionFX::MotionSet::MotionEntry* FindMotionEntry(QTableWidgetItem* item) const;

        void GetRowIndices(const QList<QTableWidgetItem*>& items, AZStd::vector<int>& outRowIndices);
        size_t CalcNumMotionEntriesUsingMotionExcluding(const AZStd::string& motionFilename, EMotionFX::MotionSet* excludedMotionSet);

    private:
        MotionSetTableWidget* m_tableWidget = nullptr;

        QAction* m_addAction = nullptr;
        QAction* m_loadAction = nullptr;
        QAction* m_editAction = nullptr;

        AzQtComponents::FilteredSearchWidget* m_searchWidget = nullptr;
        AZStd::string m_searchWidgetText;
        MotionSetsWindowPlugin* m_plugin = nullptr;
    };
} // namespace EMStudio
