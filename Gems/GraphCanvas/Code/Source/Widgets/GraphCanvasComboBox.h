/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#if !defined(Q_MOC_RUN)
#include <QCompleter>
#include <QDialog>
#include <QLineEdit>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <QTimer>
#include <QWidget>
AZ_POP_DISABLE_WARNING

#include <AzCore/Memory/SystemAllocator.h>

#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Utils/StateControllers/StackStateController.h>
#include <GraphCanvas/Widgets/ComboBox/ComboBoxItemModelInterface.h>
#endif

namespace GraphCanvas
{
    class GraphCanvasComboBoxFilterProxyModel
        : public QSortFilterProxyModel        
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(GraphCanvasComboBoxFilterProxyModel, AZ::SystemAllocator, 0);

        GraphCanvasComboBoxFilterProxyModel(QObject* parent = nullptr);

        // QSortfilterProxyModel
        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
        ////

        void SetFilter(const QString& filter);

        void BeginModelReset()
        {
            beginResetModel();
        }

        void EndModelReset()
        {
            endResetModel();
            invalidate();
        }

    private:
        QString m_filter;
        QRegExp m_testRegex;
    };

    class GraphCanvasComboBoxMenu
        : public QDialog        
    {
        Q_OBJECT
    public:
        GraphCanvasComboBoxMenu(ComboBoxItemModelInterface* model, QWidget* parent = nullptr);
        ~GraphCanvasComboBoxMenu() override;

        ComboBoxItemModelInterface* GetInterface();
        const ComboBoxItemModelInterface* GetInterface() const;

        GraphCanvasComboBoxFilterProxyModel* GetProxyModel();
        const GraphCanvasComboBoxFilterProxyModel* GetProxyModel() const;

        void ShowMenu();
        void HideMenu();

        void reject() override;
        bool eventFilter(QObject* object, QEvent* event) override;

        void focusInEvent(QFocusEvent* focusEvent) override;
        void focusOutEvent(QFocusEvent* focusEvent) override;

        void showEvent(QShowEvent* showEvent) override;
        void hideEvent(QHideEvent* hideEvent) override;

        GraphCanvas::StateController<bool>* GetDisableHidingStateController();

        void SetSelectedIndex(QModelIndex index);
        QModelIndex GetSelectedIndex() const;
        QModelIndex GetSelectedSourceIndex() const;

        void OnTableClicked(const QModelIndex& modelIndex);

    Q_SIGNALS:

        void OnIndexSelected(QModelIndex index);
        void VisibilityChanged(bool visibility);

        void CancelMenu();

        void OnFocusIn();
        void OnFocusOut();

    private:

        void HandleFocusIn();
        void HandleFocusOut();

        QTimer m_closeTimer;
        QMetaObject::Connection m_closeConnection;

        QTableView m_tableView;

        ComboBoxItemModelInterface* m_modelInterface;
        GraphCanvasComboBoxFilterProxyModel m_filterProxyModel;

        GraphCanvas::StateSetter<bool> m_disableHidingStateSetter;
        GraphCanvas::StackStateController<bool> m_disableHiding;

        bool m_ignoreNextFocusIn;
    };

    class GraphCanvasComboBox
        : public QLineEdit
        , public ViewNotificationBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(GraphCanvasComboBox, AZ::SystemAllocator, 0);
        
        GraphCanvasComboBox(ComboBoxItemModelInterface* comboBoxModel, QWidget* parent = nullptr);
        ~GraphCanvasComboBox();

        void RegisterViewId(const ViewId& viewId);
        void SetAnchorPoint(QPoint globalPoint);
        void SetMenuWidth(qreal width);
        
        void SetSelectedIndex(const QModelIndex& sourceIndex);
        QModelIndex GetSelectedIndex() const;

        // Weird issue where the background color gets dropped when provide the override. Going to pass it in for now.
        void SetOutline(const QBrush& brush, const QColor& backgroundColor);
        void SetOutlineColor(const QColor& color, const QColor& backgroundColor);
        void SetOutlineColor(const QGradient& gradient, const QColor& backgroundColor);
        void ClearOutlineColor();

        void ResetComboBox();
        void CancelInput();
        void HideMenu();

        bool IsMenuVisible() const;

        // QLineEdit
        void focusInEvent(QFocusEvent* focusEvent) override;
        void focusOutEvent(QFocusEvent* focusEvent) override;

        void keyPressEvent(QKeyEvent* keyEvent) override;
        ////

        // ViewNotificationBus
        void OnViewScrolled() override;
        void OnViewCenteredOnArea() override;
        void OnZoomChanged(qreal zoomLevel) override;
        ////
        
    Q_SIGNALS:
       
        void SelectedIndexChanged(const QModelIndex& index);
        void OnUserActionComplete();

        void OnMenuAboutToDisplay();

        void OnFocusIn();
        void OnFocusOut();
        
    protected:

        void UserSelectedIndex(const QModelIndex& proxyIndex);
        
        void OnTextChanged();
        void OnOptionsClicked();
        void OnReturnPressed();
        void OnEditComplete();

        void ClearFilter();
        void UpdateFilter();
        void CloseMenu();

        void OnMenuFocusIn();
        void OnMenuFocusOut();

        void HandleFocusState();
        
    private:

        enum class CloseMenuState
        {
            Reject,
            Accept
        };

        bool DisplayIndex(const QModelIndex& index);
    
        bool SubmitData(bool allowRest = true);
        
        void DisplayMenu();
        QString GetUserInputText();

        void UpdateMenuPosition();

        QPoint m_anchorPoint;
        qreal m_displayWidth;

        bool m_lineEditInFocus;
        bool m_popUpMenuInFocus;

        QTimer m_focusTimer;
        bool m_hasFocus;
        
        QTimer m_filterTimer;

        QTimer m_closeTimer;
        CloseMenuState m_closeState;

        ViewId m_viewId;
        
        bool m_ignoreNextComplete;
        bool m_recursionBlocker;

        QString m_selectedName;
        
        QCompleter m_completer;
        GraphCanvasComboBoxMenu m_comboBoxMenu;

        ComboBoxItemModelInterface* m_modelInterface;
        
        GraphCanvas::StateSetter<bool> m_disableHidingStateSetter;
    };
}
