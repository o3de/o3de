/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QCompleter>
#include <QDialog>
#include <QMenu>
#include <QTableView>
#include <qtimer.h>
#include <qpixmap.h>
#include <QWidget>

#include <AzCore/Memory/SystemAllocator.h>

#include <Editor/View/Widgets/DataTypePalette/DataTypePaletteModel.h>

#include <GraphCanvas/Utils/StateControllers/StackStateController.h>
#endif

namespace Ui
{
    class ContainerTypeLineEdit;
    class ContainerTypeComboBox;
}

namespace ScriptCanvasEditor
{
    class ContainerTypeMenu
        : public QDialog
    {
        Q_OBJECT
    public:
        ContainerTypeMenu(QWidget* parent = nullptr);
        ~ContainerTypeMenu() override;

        DataTypePaletteModel* GetModel();
        const DataTypePaletteModel* GetModel() const;

        DataTypePaletteSortFilterProxyModel* GetProxyModel();
        const DataTypePaletteSortFilterProxyModel* GetProxyModel() const;

        void ShowMenu();
        void HideMenu();
        
        void reject() override;
        bool eventFilter(QObject* object, QEvent* event) override;

        void focusInEvent(QFocusEvent* focusEvent) override;
        void focusOutEvent(QFocusEvent* focusEvent) override;

        void showEvent(QShowEvent* showEvent) override;
        void hideEvent(QHideEvent* hideEvent) override;

        GraphCanvas::StateController<bool>* GetStateController();
        
        void SetSelectedRow(int row);
        int GetSelectedRow() const;

        AZ::TypeId GetSelectedTypeId() const;

        void OnTableClicked(const QModelIndex& modelIndex);

    Q_SIGNALS:
        void ContainerTypeSelected(const AZ::TypeId& typeId);
        void VisibilityChanged(bool visibility);

    private:

        void HandleFocusIn();
        void HandleFocusOut();

        QTableView                          m_tableView;

        DataTypePaletteSortFilterProxyModel m_proxyModel;
        DataTypePaletteModel                m_model;

        GraphCanvas::StateSetter<bool> m_disableHidingStateSetter;
        GraphCanvas::StackStateController<bool> m_disableHiding;

        bool m_ignoreNextFocusIn;
    };
    
    class ContainerTypeLineEdit
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ContainerTypeLineEdit, AZ::SystemAllocator);

        ContainerTypeLineEdit(int index, QWidget* parent = nullptr);
        ~ContainerTypeLineEdit();
        
        void SetDisplayName(AZStd::string_view name);
        void SetDataTypes(const AZStd::unordered_set< AZ::TypeId >& dataTypes);

        AZ::TypeId GetDefaultTypeId() const;
        void SelectType(const AZ::TypeId& typeId);
        bool DisplayType(const AZ::TypeId& typeId);

        QWidget* GetLineEdit() const;

        void ResetLineEdit();
        void CancelDataInput();
        void HideDataTypeMenu();

    Q_SIGNALS:

        void TypeChanged(int index, const AZ::TypeId& typeId);
        void DataTypeMenuVisibilityChanged(bool visible);

    protected:

        bool eventFilter(QObject* obj, QEvent* event);

        void OnTextChanged();
        void OnOptionsClicked();
        void OnReturnPressed();
        void OnEditComplete();

        void UpdateFilter();
        
    private:

        bool SubmitData(bool allowReset = true);

        void DisplayMenu();
        QString GetUserInputText();
    
        AZStd::unique_ptr<Ui::ContainerTypeLineEdit> m_ui;

        QTimer m_filterTimer;

        bool m_ignoreNextComplete;
        bool m_recursionBlocker;

        int m_index;
        AZ::TypeId m_lastId;

        QCompleter m_completer;
        ContainerTypeMenu m_dataTypeMenu;

        GraphCanvas::StateSetter<bool> m_disableHidingStateSetter;
    };
}
