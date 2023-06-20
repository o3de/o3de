/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>

#include <QEvent>
#include <QFocusEvent>
#include <QHeaderView>
#include <QScopedValueRollback>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>

#include <Widgets/GraphCanvasComboBox.h>

namespace GraphCanvas
{
    ////////////////////////////////////////
    // GraphCanvasComboBoxFilterProxyModel
    ////////////////////////////////////////    

    GraphCanvasComboBoxFilterProxyModel::GraphCanvasComboBoxFilterProxyModel(QObject*)
    {
    }

    bool GraphCanvasComboBoxFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        if (m_filter.isEmpty())
        {
            return true;
        }

        QAbstractItemModel* model = sourceModel();

        if (!model)
        {
            return false;
        }

        QModelIndex index = model->index(sourceRow, filterKeyColumn(), sourceParent);
        QString test = model->data(index, filterRole()).toString();

        return (test.lastIndexOf(m_testRegex) >= 0);
    }

    void GraphCanvasComboBoxFilterProxyModel::SetFilter(const QString& filter)
    {
        m_filter = filter;
        m_testRegex = QRegExp(m_filter, Qt::CaseInsensitive);
        invalidateFilter();

        if (m_filter.isEmpty())
        {
            sort(-1);
        }
        else
        {
            sort(filterKeyColumn());
        }
    }

    ////////////////////////////
    // GraphCanvasComboBoxMenu
    ////////////////////////////

    GraphCanvasComboBoxMenu::GraphCanvasComboBoxMenu(ComboBoxItemModelInterface* model, QWidget* parent)
        : QDialog(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
        , m_disableHiding(false)
        , m_ignoreNextFocusIn(false)
        , m_modelInterface(model)
    {
        setProperty("HasNoWindowDecorations", true);

        setAttribute(Qt::WA_ShowWithoutActivating);

        m_filterProxyModel.setSourceModel(m_modelInterface->GetDropDownItemModel());
        m_filterProxyModel.sort(m_modelInterface->GetSortColumn());
        m_filterProxyModel.setFilterKeyColumn(m_modelInterface->GetCompleterColumn());

        m_tableView.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        m_tableView.setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableView.setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);

        m_tableView.setModel(GetProxyModel());
        m_tableView.verticalHeader()->hide();
        m_tableView.verticalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);

        m_tableView.horizontalHeader()->hide();
        m_tableView.horizontalHeader()->setStretchLastSection(true);

        m_tableView.installEventFilter(this);
        m_tableView.setFocusPolicy(Qt::FocusPolicy::ClickFocus);

        m_tableView.setMinimumWidth(0);

        QVBoxLayout* layout = new QVBoxLayout();
        layout->addWidget(&m_tableView);
        setLayout(layout);

        m_disableHidingStateSetter.AddStateController(GetDisableHidingStateController());

        QObject::connect(&m_tableView, &QTableView::clicked, this, &GraphCanvasComboBoxMenu::OnTableClicked);

        m_closeTimer.setInterval(0);

        QAction* escapeAction = new QAction(this);
        escapeAction->setShortcut(Qt::Key_Escape);

        addAction(escapeAction);
        connect(this, &QDialog::finished, this, [&]() {
            Q_EMIT OnFocusOut();
        });

        QObject::connect(escapeAction, &QAction::triggered, this, &GraphCanvasComboBoxMenu::CancelMenu);
    }

    GraphCanvasComboBoxMenu::~GraphCanvasComboBoxMenu()
    {
    }

    ComboBoxItemModelInterface* GraphCanvasComboBoxMenu::GetInterface()
    {
        return m_modelInterface;
    }

    const ComboBoxItemModelInterface* GraphCanvasComboBoxMenu::GetInterface() const
    {
        return m_modelInterface;
    }

    GraphCanvasComboBoxFilterProxyModel* GraphCanvasComboBoxMenu::GetProxyModel()
    {
        return &m_filterProxyModel;
    }

    const GraphCanvasComboBoxFilterProxyModel* GraphCanvasComboBoxMenu::GetProxyModel() const
    {
        return &m_filterProxyModel;
    }

    void GraphCanvasComboBoxMenu::ShowMenu()
    {
        clearFocus();
        m_tableView.clearFocus();
        m_tableView.selectionModel()->clearSelection();

        m_filterProxyModel.BeginModelReset();
        m_modelInterface->OnDropDownAboutToShow();
        m_filterProxyModel.EndModelReset();

        show();

        m_disableHidingStateSetter.ReleaseState();

        int rowHeight = m_tableView.rowHeight(0);

        if (rowHeight > 0)
        {
            // Generic padding of like 20 pixels.
            setMinimumHeight(aznumeric_cast<int>(rowHeight * 4.5 + 20));
            setMaximumHeight(aznumeric_cast<int>(rowHeight * 4.5 + 20));
        }
    }

    void GraphCanvasComboBoxMenu::HideMenu()
    {
        m_disableHidingStateSetter.ReleaseState();

        m_tableView.clearFocus();
        m_tableView.selectionModel()->clearSelection();

        clearFocus();
        reject();

        m_filterProxyModel.BeginModelReset();
        m_modelInterface->OnDropDownHidden();
        m_filterProxyModel.EndModelReset();
    }

    void GraphCanvasComboBoxMenu::reject()
    {
        if (!m_disableHiding.GetState())
        {
            QDialog::reject();
        }
    }

    bool GraphCanvasComboBoxMenu::eventFilter(QObject* object, QEvent* event)
    {
        if (object == &m_tableView)
        {
            switch (event->type())
            {
            case QEvent::FocusOut:
                HandleFocusOut();
                break;
            case QEvent::FocusIn:
                HandleFocusIn();
                break;
            }
        }

        return false;
    }

    void GraphCanvasComboBoxMenu::focusInEvent(QFocusEvent* focusEvent)
    {
        QDialog::focusInEvent(focusEvent);

        if (focusEvent->isAccepted())
        {
            if (!m_ignoreNextFocusIn)
            {
                HandleFocusIn();
            }
            else
            {
                m_ignoreNextFocusIn = false;
            }
        }
    }

    void GraphCanvasComboBoxMenu::focusOutEvent(QFocusEvent* focusEvent)
    {
        QDialog::focusOutEvent(focusEvent);
        HandleFocusOut();
    }

    void GraphCanvasComboBoxMenu::showEvent(QShowEvent* showEvent)
    {
        QDialog::showEvent(showEvent);

        // So, despite me telling it to not activate, the window still gets a focus in event.
        // But, it doesn't get a focus out event, since it doesn't actually accept the focus in event?
        m_ignoreNextFocusIn = true;
        m_tableView.selectionModel()->clearSelection();

        Q_EMIT VisibilityChanged(true);
    }

    void GraphCanvasComboBoxMenu::hideEvent(QHideEvent* hideEvent)
    {
        QDialog::hideEvent(hideEvent);

        clearFocus();

        Q_EMIT VisibilityChanged(false);
        m_tableView.selectionModel()->clearSelection();
        m_filterProxyModel.invalidate();
    }

    GraphCanvas::StateController<bool>* GraphCanvasComboBoxMenu::GetDisableHidingStateController()
    {
        return &m_disableHiding;
    }

    void GraphCanvasComboBoxMenu::SetSelectedIndex(QModelIndex index)
    {
        m_tableView.selectionModel()->clear();

        if (index.isValid()
            && index.row() >= 0
            && index.row() < m_filterProxyModel.rowCount())
        {
            QItemSelection rowSelection(m_filterProxyModel.index(index.row(), 0, index.parent()), m_filterProxyModel.index(index.row(), m_filterProxyModel.columnCount() - 1, index.parent()));
            m_tableView.selectionModel()->select(rowSelection, QItemSelectionModel::Select);

            m_tableView.scrollTo(m_filterProxyModel.index(index.row(), 0, index.parent()));
        }
    }

    QModelIndex GraphCanvasComboBoxMenu::GetSelectedIndex() const
    {
        if (m_tableView.selectionModel()->hasSelection())
        {
            QModelIndexList selectedIndexes = m_tableView.selectionModel()->selectedIndexes();

            if (!selectedIndexes.empty())
            {
                return selectedIndexes.front();
            }
        }

        return QModelIndex();
    }

    QModelIndex GraphCanvasComboBoxMenu::GetSelectedSourceIndex() const
    {
        if (m_tableView.selectionModel()->hasSelection())
        {
            QModelIndexList selectedIndexes = m_tableView.selectionModel()->selectedIndexes();

            if (!selectedIndexes.empty())
            {
                return m_filterProxyModel.mapToSource(selectedIndexes.front());
            }
        }

        return QModelIndex();
    }

    void GraphCanvasComboBoxMenu::OnTableClicked(const QModelIndex& modelIndex)
    {
        if (modelIndex.isValid())
        {
            QModelIndex sourceIndex = m_filterProxyModel.mapToSource(modelIndex);

            if (sourceIndex.isValid())
            {
                Q_EMIT OnIndexSelected(sourceIndex);

                QObject::disconnect(m_closeConnection);
                m_closeConnection = QObject::connect(&m_closeTimer, &QTimer::timeout, this, &QDialog::accept);

                m_closeTimer.start();
            }
        }
    }

    void GraphCanvasComboBoxMenu::HandleFocusIn()
    {
        m_disableHidingStateSetter.SetState(true);

        emit OnFocusIn();
    }

    void GraphCanvasComboBoxMenu::HandleFocusOut()
    {
        m_disableHidingStateSetter.ReleaseState();

        QObject::disconnect(m_closeConnection);
        m_closeConnection = QObject::connect(&m_closeTimer, &QTimer::timeout, this, &QDialog::reject);

        m_closeTimer.start();
    }

    ////////////////////////
    // GraphCanvasComboBox
    ////////////////////////

    GraphCanvasComboBox::GraphCanvasComboBox(ComboBoxItemModelInterface* modelInterface, QWidget* parent)
        : QLineEdit(parent)
        , m_lineEditInFocus(false)
        , m_popUpMenuInFocus(false)
        , m_hasFocus(false)
        , m_ignoreNextComplete(false)
        , m_recursionBlocker(false)
        , m_comboBoxMenu(modelInterface)
        , m_modelInterface(modelInterface)
    {
        setObjectName("ComboBoxLineEdit");
        setProperty("HasNoWindowDecorations", true);
        setProperty("DisableFocusWindowFix", true);

        QAction* action = addAction(QIcon(":/GraphCanvasEditorResources/settings_icon.png"), QLineEdit::ActionPosition::TrailingPosition);
        QObject::connect(action, &QAction::triggered, this, &GraphCanvasComboBox::OnOptionsClicked);        

        QAction* escapeAction = new QAction(this);
        escapeAction->setShortcut(Qt::Key_Escape);

        addAction(escapeAction);

        QObject::connect(escapeAction, &QAction::triggered, this, &GraphCanvasComboBox::ResetComboBox);

        m_completer.setModel(m_modelInterface->GetCompleterItemModel());
        m_completer.setCompletionColumn(m_modelInterface->GetCompleterColumn());
        m_completer.setCompletionMode(QCompleter::CompletionMode::InlineCompletion);
        m_completer.setCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);

        QObject::connect(this, &QLineEdit::textEdited, this, &GraphCanvasComboBox::OnTextChanged);
        QObject::connect(this, &QLineEdit::returnPressed, this, &GraphCanvasComboBox::OnReturnPressed);
        QObject::connect(this, &QLineEdit::editingFinished, this, &GraphCanvasComboBox::OnEditComplete);

        m_filterTimer.setInterval(500);
        QObject::connect(&m_filterTimer, &QTimer::timeout, this, &GraphCanvasComboBox::UpdateFilter);

        m_closeTimer.setInterval(0);
        QObject::connect(&m_closeTimer, &QTimer::timeout, this, &GraphCanvasComboBox::CloseMenu);

        m_focusTimer.setInterval(0);
        QObject::connect(&m_focusTimer, &QTimer::timeout, this, &GraphCanvasComboBox::HandleFocusState);

        QObject::connect(&m_comboBoxMenu, &GraphCanvasComboBoxMenu::OnIndexSelected, this, &GraphCanvasComboBox::UserSelectedIndex);
        QObject::connect(&m_comboBoxMenu, &GraphCanvasComboBoxMenu::OnFocusIn, this, &GraphCanvasComboBox::OnMenuFocusIn);
        QObject::connect(&m_comboBoxMenu, &GraphCanvasComboBoxMenu::OnFocusOut, this, &GraphCanvasComboBox::OnMenuFocusOut);
        QObject::connect(&m_comboBoxMenu, &GraphCanvasComboBoxMenu::CancelMenu, this, &GraphCanvasComboBox::ResetComboBox);

        m_comboBoxMenu.accept();

        m_disableHidingStateSetter.AddStateController(m_comboBoxMenu.GetDisableHidingStateController());
    }

    GraphCanvasComboBox::~GraphCanvasComboBox()
    {
    }

    void GraphCanvasComboBox::RegisterViewId(const ViewId& viewId)
    {
        m_viewId = viewId;
    }

    void GraphCanvasComboBox::SetAnchorPoint(QPoint globalPoint)
    {
        m_anchorPoint = globalPoint;
        UpdateMenuPosition();
    }

    void GraphCanvasComboBox::SetMenuWidth(qreal width)
    {
        m_displayWidth = width;
        UpdateMenuPosition();
    }

    void GraphCanvasComboBox::SetSelectedIndex(const QModelIndex& selectedIndex)
    {
        QModelIndex previousIndex = m_modelInterface->FindIndexForName(m_selectedName);

        if (previousIndex != selectedIndex)
        {
            m_selectedName = m_modelInterface->GetNameForIndex(selectedIndex);

            if (DisplayIndex(selectedIndex))
            {
                Q_EMIT SelectedIndexChanged(selectedIndex);
            }
        }
    }
    
    QModelIndex GraphCanvasComboBox::GetSelectedIndex() const
    {
        return m_modelInterface->FindIndexForName(m_selectedName);
    }

    void GraphCanvasComboBox::ClearOutlineColor()
    {
        setStyleSheet("");
    }

    void GraphCanvasComboBox::ResetComboBox()
    {        
        HideMenu();

        QModelIndex selectedIndex = m_modelInterface->FindIndexForName(m_selectedName);
        DisplayIndex(selectedIndex);

        m_selectedName.clear();
    }
    
    void GraphCanvasComboBox::CancelInput()
    {
        QModelIndex selectedIndex = m_modelInterface->FindIndexForName(m_selectedName);
        DisplayIndex(selectedIndex);
        HideMenu();
    }
    
    void GraphCanvasComboBox::HideMenu()
    {
        m_disableHidingStateSetter.ReleaseState();

        m_comboBoxMenu.HideMenu();

        ViewNotificationBus::Handler::BusDisconnect(m_viewId);
    }

    bool GraphCanvasComboBox::IsMenuVisible() const
    {
        return !m_comboBoxMenu.isHidden();
    }

    void GraphCanvasComboBox::focusInEvent(QFocusEvent* focusEvent)
    {
        QLineEdit::focusInEvent(focusEvent);

        m_lineEditInFocus = true;
        m_focusTimer.start();

        grabKeyboard();
    }

    void GraphCanvasComboBox::focusOutEvent(QFocusEvent* focusEvent)
    {
        QLineEdit::focusOutEvent(focusEvent);

        m_lineEditInFocus = false;
        m_focusTimer.start();

        releaseKeyboard();
    }

    void GraphCanvasComboBox::keyPressEvent(QKeyEvent* keyEvent)
    {
        QLineEdit::keyPressEvent(keyEvent);

        switch(keyEvent->key()) 
        {
            case Qt::Key_Down:
            {
                if (m_comboBoxMenu.isHidden())
                {
                    ClearFilter();
                    DisplayMenu();
                }

                QModelIndex selectedIndex = m_comboBoxMenu.GetSelectedSourceIndex();

                if (!selectedIndex.isValid())
                {
                    selectedIndex = m_modelInterface->FindIndexForName(m_selectedName);
                }

                QModelIndex mappedIndex;
                QModelIndex sourceIndex;

                if (selectedIndex.isValid())
                {
                    sourceIndex = m_modelInterface->GetNextIndex(selectedIndex);
                }
                else
                {
                    sourceIndex = m_modelInterface->GetDefaultIndex();
                }

                while (sourceIndex != selectedIndex)
                {
                    mappedIndex = m_comboBoxMenu.GetProxyModel()->mapFromSource(sourceIndex);

                    if (mappedIndex.isValid())
                    {
                        break;
                    }

                    sourceIndex = m_modelInterface->GetNextIndex(sourceIndex);
                }

                selectedIndex = sourceIndex;
                m_comboBoxMenu.SetSelectedIndex(mappedIndex);

                QString typeName = m_modelInterface->GetNameForIndex(selectedIndex);

                if (!typeName.isEmpty())
                {
                    setText(typeName);
                    setSelection(0, static_cast<int>(typeName.size()));

                    m_completer.setCompletionPrefix(typeName);
                }

                keyEvent->accept();
            }
            break;
            case Qt::Key_Up:
            {
                if (m_comboBoxMenu.isHidden())
                {
                    m_comboBoxMenu.GetProxyModel()->SetFilter("");
                    DisplayMenu();
                }

                QModelIndex selectedIndex = m_comboBoxMenu.GetSelectedSourceIndex();

                if (!selectedIndex.isValid())
                {
                    selectedIndex = m_modelInterface->FindIndexForName(m_selectedName);
                }

                QModelIndex mappedIndex;
                QModelIndex sourceIndex;

                if (selectedIndex.isValid())
                {
                    sourceIndex = m_modelInterface->GetPreviousIndex(selectedIndex);
                }
                else
                {
                    sourceIndex = m_modelInterface->GetPreviousIndex(m_modelInterface->GetDefaultIndex());
                }

                while (sourceIndex != selectedIndex)
                {
                    mappedIndex = m_comboBoxMenu.GetProxyModel()->mapFromSource(sourceIndex);

                    if (mappedIndex.isValid())
                    {
                        break;
                    }

                    sourceIndex = m_modelInterface->GetPreviousIndex(sourceIndex);
                }

                selectedIndex = sourceIndex;
                m_comboBoxMenu.SetSelectedIndex(mappedIndex);

                QString typeName = m_comboBoxMenu.GetInterface()->GetNameForIndex(sourceIndex);

                if (!typeName.isEmpty())
                {
                    setText(typeName);
                    setSelection(0, static_cast<int>(typeName.size()));

                    m_completer.setCompletionPrefix(typeName);
                }

                keyEvent->accept();
            }
            break;
            case Qt::Key_Escape:
                ResetComboBox();
                keyEvent->accept();
            break;
            default:
                break;

        }
    }

    void GraphCanvasComboBox::OnViewScrolled()
    {
        ResetComboBox();
    }

    void GraphCanvasComboBox::OnViewCenteredOnArea()
    {
        ResetComboBox();
    }

    void GraphCanvasComboBox::OnZoomChanged(qreal /*zoomLevel*/)
    {
    }

    void GraphCanvasComboBox::UserSelectedIndex(const QModelIndex& selectedIndex)
    {
        QModelIndex previousIndex = m_modelInterface->FindIndexForName(m_selectedName);

        if (previousIndex != selectedIndex)
        {
            SetSelectedIndex(selectedIndex);

            Q_EMIT OnUserActionComplete();
        }
    }

    void GraphCanvasComboBox::OnTextChanged()
    {
        DisplayMenu();
        UpdateFilter();
    }
    
    void GraphCanvasComboBox::OnOptionsClicked()
    {
        if (m_comboBoxMenu.isHidden())
        {
            m_comboBoxMenu.GetProxyModel()->SetFilter("");
            DisplayMenu();
        }
        else
        {
            m_comboBoxMenu.accept();
        }
    }
    
    void GraphCanvasComboBox::OnReturnPressed()
    {
        const bool allowReset = false;
        if (SubmitData(allowReset))
        {
            m_comboBoxMenu.accept();
        }
        else
        {
            setText("");
            UpdateFilter();
        }

        Q_EMIT OnUserActionComplete();

        // When we press enter, we will also get an editing complete signal. We want to ignore that since we handled it here.
        m_ignoreNextComplete = true;
    }
    
    void GraphCanvasComboBox::OnEditComplete()
    {
        if (m_ignoreNextComplete)
        {
            m_ignoreNextComplete = false;
            return;
        }

        SubmitData(false);

        m_closeState = CloseMenuState::Reject;
        m_closeTimer.start();
    }

    void GraphCanvasComboBox::ClearFilter()
    {
        m_comboBoxMenu.GetProxyModel()->SetFilter("");
    }
    
    void GraphCanvasComboBox::UpdateFilter()
    {
        m_comboBoxMenu.GetProxyModel()->SetFilter(GetUserInputText());
    }

    void GraphCanvasComboBox::CloseMenu()
    {
        switch (m_closeState)
        {
        case CloseMenuState::Accept:
            m_comboBoxMenu.accept();
            break;
        default:
            m_comboBoxMenu.reject();
            break;
        }

        m_closeState = CloseMenuState::Reject;
    }

    void GraphCanvasComboBox::OnMenuFocusIn()
    {
        m_popUpMenuInFocus = true;
        m_focusTimer.start();
    }

    void GraphCanvasComboBox::OnMenuFocusOut()
    {
        m_popUpMenuInFocus = false;
        m_focusTimer.start();
    }

    void GraphCanvasComboBox::HandleFocusState()
    {
        bool focusState = m_lineEditInFocus || m_popUpMenuInFocus;

        if (focusState != m_hasFocus)
        {
            m_hasFocus = focusState;

            if (m_hasFocus)
            {
                Q_EMIT OnFocusIn();
            }
            else
            {
                Q_EMIT OnFocusOut();
                HideMenu();
            }
        }
    }

    bool GraphCanvasComboBox::DisplayIndex(const QModelIndex& index)
    {
        QSignalBlocker signalBlocker(this);

        QString name = m_modelInterface->GetNameForIndex(index);

        if (!name.isEmpty())
        {
            m_completer.setCompletionPrefix(name);
            setText(name);
            ClearFilter();
        }
        else
        {
            if (!m_selectedName.isEmpty())
            {
                QModelIndex currentIndex = m_modelInterface->FindIndexForName(m_selectedName);

                if (currentIndex != index)
                {
                    DisplayIndex(currentIndex);
                }
            }
            else
            {
                m_completer.setCompletionPrefix("");
                setText("");
                UpdateFilter();
            }
        }

        return !m_selectedName.isEmpty();
    }
    
    bool GraphCanvasComboBox::SubmitData(bool allowReset)
    {
        QString inputName = text();

        QModelIndex inputIndex = m_modelInterface->FindIndexForName(inputName);

        // We didn't input a valid type. So default to our last previously known value.
        if (!inputIndex.isValid())
        {
            if (allowReset)
            {
                QModelIndex lastIndex = m_modelInterface->FindIndexForName(m_selectedName);
                DisplayIndex(lastIndex);

                inputIndex = lastIndex;
            }
        }
        else
        {
            SetSelectedIndex(inputIndex);
        }

        return inputIndex.isValid();
    }
    
    void GraphCanvasComboBox::DisplayMenu()
    {
        if (!m_recursionBlocker)
        {
            QScopedValueRollback<bool> valueRollback(m_recursionBlocker, true);
            if (m_comboBoxMenu.isHidden())
            {
                Q_EMIT OnMenuAboutToDisplay();
                ViewNotificationBus::Handler::BusConnect(m_viewId);

                qreal zoomLevel = 1.0;
                ViewRequestBus::EventResult(zoomLevel, m_viewId, &ViewRequests::GetZoomLevel);

                if (zoomLevel < 1.0)
                {
                    zoomLevel = 1.0;
                }

                m_comboBoxMenu.GetInterface()->SetFontScale(zoomLevel);
                m_comboBoxMenu.ShowMenu();

                UpdateMenuPosition();
            }
        }

        if (!m_disableHidingStateSetter.HasState())
        {
            m_disableHidingStateSetter.SetState(true);
        }
    }
    
    QString GraphCanvasComboBox::GetUserInputText()
    {
        QString lineEditText = text();

        // The QCompleter doesn't seem to update the completion prefix when you delete anything, only when things are added.
        // To get it to update correctly when the user deletes something, I'm using the combination of things:
        //
        // 1) If we have a completion, that text will be auto filled into the quick filter because of the completion model.
        // So, we will compare those two values, and if they match, we know we want to search using the completion prefix.
        //
        // 2) If they don't match, it means that user deleted something, and the Completer didn't update it's internal state, so we'll just
        // use whatever is in the text box.
        //
        // 3) When the text field is set to empty, the current completion gets invalidated, but the prefix doesn't, so that gets special cased out.
        //
        // Extra fun: If you type in something, "Like" then delete a middle character, "Lie", and then put the k back in. It will auto complete the E
        // visually but the completion prefix will be the entire word.
        if (completer()
            && completer()->currentCompletion().compare(lineEditText, Qt::CaseInsensitive) == 0
            && !lineEditText.isEmpty())
        {
            lineEditText = completer()->completionPrefix();
        }

        return lineEditText;
    }

    void GraphCanvasComboBox::UpdateMenuPosition()
    {
        if (!m_comboBoxMenu.isHidden())
        {
            QRect dialogGeometry = m_comboBoxMenu.geometry();

            dialogGeometry.moveTopLeft(m_anchorPoint);
            dialogGeometry.setWidth(aznumeric_cast<int>(m_displayWidth));

            m_comboBoxMenu.setGeometry(dialogGeometry);
        }
    }
}

#include <Source/Widgets/moc_GraphCanvasComboBox.cpp>
