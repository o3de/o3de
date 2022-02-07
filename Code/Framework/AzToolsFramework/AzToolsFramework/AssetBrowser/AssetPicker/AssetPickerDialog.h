/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>

#include <QDialog>
#include <QScopedPointer>
#endif

class QKeyEvent;
class QModelIndex;
class QItemSelection;

namespace Ui
{
    class AssetPickerDialogClass;
}

namespace AzToolsFramework
{
    class QWidgetSavedState;

    namespace AssetBrowser
    {
        class ProductAssetBrowserEntry;
        class AssetBrowserFilterModel;
        class AssetBrowserTableModel;
        class AssetBrowserModel;
        class AssetSelectionModel;

        class AssetPickerDialog
            : public QDialog
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(AssetPickerDialog, AZ::SystemAllocator, 0);
            explicit AssetPickerDialog(AssetSelectionModel& selection, QWidget* parent = nullptr);
            virtual ~AssetPickerDialog();

        Q_SIGNALS:
            void SizeChangedSignal(int newWidth);

        protected:
            //////////////////////////////////////////////////////////////////////////
            // QDialog
            //////////////////////////////////////////////////////////////////////////
            void accept() override;
            void reject() override;
            void keyPressEvent(QKeyEvent* e) override;
            void resizeEvent(QResizeEvent* resizeEvent) override;

        private Q_SLOTS:
            void DoubleClickedSlot(const QModelIndex& index);
            void SelectionChangedSlot();
            void RestoreState();
            void OnFilterUpdated();

        private:
            //! Evaluate whether current selection is valid.
            //! Valid selection requires exactly one item to be selected, must be source or product type, and must match the wildcard filter
            bool EvaluateSelection() const;
            void UpdatePreview() const;
            void SaveState();

            QScopedPointer<Ui::AssetPickerDialogClass> m_ui;
            AssetBrowserModel* m_assetBrowserModel = nullptr;
            QScopedPointer<AssetBrowserFilterModel> m_filterModel;
            QScopedPointer<AssetBrowserTableModel> m_tableModel;
            AssetSelectionModel& m_selection;
            bool m_hasFilter;
            AZStd::unique_ptr<TreeViewState> m_filterStateSaver;
            AZStd::intrusive_ptr<QWidgetSavedState> m_persistentState;
        };
    } // AssetBrowser
} // AzToolsFramework
