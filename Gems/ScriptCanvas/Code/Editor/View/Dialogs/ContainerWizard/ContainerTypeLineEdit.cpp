/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <precompiled.h>

#include <qtoolbutton.h>
#include <qscopedvaluerollback.h>

#include <QFocusEvent>
#include <QHeaderView>
#include <QWidgetAction>

#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Widgets/StyledItemDelegates/IconDecoratedNameDelegate.h>

#include <Editor/View/Dialogs/ContainerWizard/ContainerTypeLineEdit.h>
#include <Editor/View/Dialogs/ContainerWizard/ui_ContainerTypeLineEdit.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>

namespace ScriptCanvasEditor
{
    //////////////////////
    // ContainerTypeMenu
    //////////////////////

    ContainerTypeMenu::ContainerTypeMenu(QWidget* parent)
        : QDialog(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
        , m_disableHiding(false)
        , m_ignoreNextFocusIn(false)
    {
        setProperty("HasNoWindowDecorations", true);

        setAttribute(Qt::WA_ShowWithoutActivating);

        m_proxyModel.setSourceModel(GetModel());
        m_proxyModel.sort(DataTypePaletteModel::ColumnIndex::Type);

        m_tableView.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        m_tableView.setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableView.setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);

        m_tableView.setItemDelegateForColumn(DataTypePaletteModel::Type, aznew GraphCanvas::IconDecoratedNameDelegate(this));

        m_tableView.setModel(GetProxyModel());
        m_tableView.verticalHeader()->hide();
        m_tableView.horizontalHeader()->hide();

        m_tableView.horizontalHeader()->setSectionResizeMode(DataTypePaletteModel::ColumnIndex::Pinned, QHeaderView::ResizeMode::ResizeToContents);
        m_tableView.horizontalHeader()->setSectionResizeMode(DataTypePaletteModel::ColumnIndex::Type, QHeaderView::ResizeMode::Stretch);

        m_tableView.installEventFilter(this);
        m_tableView.setFocusPolicy(Qt::FocusPolicy::ClickFocus);

        QVBoxLayout* layout = new QVBoxLayout();
        layout->addWidget(&m_tableView);
        setLayout(layout);

        m_disableHidingStateSetter.AddStateController(GetStateController());

        QObject::connect(&m_tableView, &QTableView::clicked, this, &ContainerTypeMenu::OnTableClicked);
    }

    ContainerTypeMenu::~ContainerTypeMenu()
    {

    }

    DataTypePaletteModel* ContainerTypeMenu::GetModel()
    {
        return &m_model;
    }

    const DataTypePaletteModel* ContainerTypeMenu::GetModel() const
    {
        return &m_model;
    }

    DataTypePaletteSortFilterProxyModel* ContainerTypeMenu::GetProxyModel()
    {
        return &m_proxyModel;
    }

    const DataTypePaletteSortFilterProxyModel* ContainerTypeMenu::GetProxyModel() const
    {
        return &m_proxyModel;
    }

    void ContainerTypeMenu::ShowMenu()
    {
        clearFocus();
        m_tableView.clearFocus();

        show();

        m_disableHidingStateSetter.ReleaseState();
    }

    void ContainerTypeMenu::HideMenu()
    {
        m_disableHidingStateSetter.ReleaseState();

        m_tableView.clearFocus();
        clearFocus();
        reject();
    }

    void ContainerTypeMenu::reject()
    {
        if (!m_disableHiding.GetState())
        {
            QDialog::reject();
        }
    }

    bool ContainerTypeMenu::eventFilter(QObject* object, QEvent* event)
    {
        if (object == &m_tableView)
        {
            if (event->type() == QEvent::FocusOut)
            {
                HandleFocusOut();
            }
            else if (event->type() == QEvent::FocusIn)
            {
                HandleFocusIn();
            }
        }

        return false;
    }

    void ContainerTypeMenu::focusInEvent(QFocusEvent* focusEvent)
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

    void ContainerTypeMenu::focusOutEvent(QFocusEvent* focusEvent)
    {
        QDialog::focusOutEvent(focusEvent);
        HandleFocusOut();
    }

    void ContainerTypeMenu::showEvent(QShowEvent* showEvent)
    {
        QDialog::showEvent(showEvent);

        // So, despite me telling it to not activate, the window still gets a focus in event.
        // But, it doesn't get a foccus out event, since it doesn't actually accept the focus in event?
        m_ignoreNextFocusIn = true;
        m_tableView.selectionModel()->clearSelection();

        Q_EMIT VisibilityChanged(true);
    }

    void ContainerTypeMenu::hideEvent(QHideEvent* hideEvent)
    {
        QDialog::hideEvent(hideEvent);

        clearFocus();

        Q_EMIT VisibilityChanged(false);
        m_tableView.selectionModel()->clearSelection();
    }

    GraphCanvas::StateController<bool>* ContainerTypeMenu::GetStateController()
    {
        return &m_disableHiding;
    }

    void ContainerTypeMenu::SetSelectedRow(int row)
    {
        m_tableView.selectionModel()->clear();

        if (row <= m_proxyModel.rowCount()
            && row >= 0)
        {
            QItemSelection rowSelection(m_proxyModel.index(row, 0), m_proxyModel.index(row, m_proxyModel.columnCount()-1));
            m_tableView.selectionModel()->select(rowSelection, QItemSelectionModel::Select);

            m_tableView.scrollTo(m_proxyModel.index(row, 0));
        }
    }

    int ContainerTypeMenu::GetSelectedRow() const
    {
        if (m_tableView.selectionModel()->hasSelection())
        {
            QModelIndexList selectedIndexes = m_tableView.selectionModel()->selectedIndexes();

            if (!selectedIndexes.empty())
            {
                return selectedIndexes.front().row();
            }
        }

        return -1;
    }

    AZ::TypeId ContainerTypeMenu::GetSelectedTypeId() const
    {
        QModelIndexList selectedIndexes = m_tableView.selectionModel()->selectedIndexes();

        if (!selectedIndexes.empty())
        {
            QModelIndex firstSelection = selectedIndexes.front();

            QModelIndex sourceIndex = m_proxyModel.mapToSource(firstSelection);
            return m_model.FindTypeIdForIndex(sourceIndex);
        }
        
        return AZ::TypeId::CreateNull();
    }

    void ContainerTypeMenu::OnTableClicked(const QModelIndex& modelIndex)
    {
        if (modelIndex.isValid())
        {
            QModelIndex sourceIndex = m_proxyModel.mapToSource(modelIndex);

            AZ::TypeId typeId = m_model.FindTypeIdForIndex(sourceIndex);

            if (!typeId.IsNull())
            {
                Q_EMIT ContainerTypeSelected(typeId);
                QTimer::singleShot(0, [this]() { accept(); });
            }
        }
    }

    void ContainerTypeMenu::HandleFocusIn()
    {
        m_disableHidingStateSetter.SetState(true);
    }

    void ContainerTypeMenu::HandleFocusOut()
    {
        m_disableHidingStateSetter.ReleaseState();
        QTimer::singleShot(0, [this]() { reject(); });
    }    

    //////////////////////////
    // ContainerTypeLineEdit
    //////////////////////////

    ContainerTypeLineEdit::ContainerTypeLineEdit(int index, QWidget* parent)
        : QWidget(parent)
        , m_ui(new Ui::ContainerTypeLineEdit())
        , m_ignoreNextComplete(false)
        , m_recursionBlocker(false)
        , m_index(index)
        , m_lastId(azrtti_typeid<void>())
        , m_dataTypeMenu(parent)
    {
        m_ui->setupUi(this);

        QAction* action = m_ui->variableType->addAction(QIcon(":/ScriptCanvasEditorResources/Resources/triangle.png"), QLineEdit::ActionPosition::TrailingPosition);
        QObject::connect(action, &QAction::triggered, this, &ContainerTypeLineEdit::OnOptionsClicked);

        m_completer.setModel(m_dataTypeMenu.GetModel());
        m_completer.setCompletionColumn(DataTypePaletteModel::ColumnIndex::Type);
        m_completer.setCompletionMode(QCompleter::CompletionMode::InlineCompletion);
        m_completer.setCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);

        m_ui->variableType->installEventFilter(this);
        m_ui->variableType->setCompleter(&m_completer);

        QObject::connect(m_ui->variableType, &QLineEdit::textEdited, this, &ContainerTypeLineEdit::OnTextChanged);
        QObject::connect(m_ui->variableType, &QLineEdit::returnPressed, this, &ContainerTypeLineEdit::OnReturnPressed);
        QObject::connect(m_ui->variableType, &QLineEdit::editingFinished, this, &ContainerTypeLineEdit::OnEditComplete);

        m_filterTimer.setInterval(500);
        QObject::connect(&m_filterTimer, &QTimer::timeout, this, &ContainerTypeLineEdit::UpdateFilter);

        QObject::connect(&m_dataTypeMenu, &ContainerTypeMenu::ContainerTypeSelected, this, &ContainerTypeLineEdit::SelectType);
        QObject::connect(&m_dataTypeMenu, &ContainerTypeMenu::VisibilityChanged, this, &ContainerTypeLineEdit::DataTypeMenuVisibilityChanged);

        m_dataTypeMenu.accept();

        m_disableHidingStateSetter.AddStateController(m_dataTypeMenu.GetStateController());
    }

    ContainerTypeLineEdit::~ContainerTypeLineEdit()
    {
        m_dataTypeMenu.accept();
    }

    void ContainerTypeLineEdit::SetDisplayName(AZStd::string_view name)
    {
        m_ui->nameDisplay->setText(name.data());
    }

    void ContainerTypeLineEdit::SetDataTypes(const AZStd::unordered_set< AZ::TypeId >& dataTypes)
    {
        m_dataTypeMenu.GetModel()->ClearTypes();
        m_dataTypeMenu.GetModel()->PopulateVariablePalette(dataTypes);
    }

    AZ::TypeId ContainerTypeLineEdit::GetDefaultTypeId() const
    {
        const DataTypePaletteSortFilterProxyModel* proxyModel = m_dataTypeMenu.GetProxyModel();

        if (proxyModel->rowCount() > 0)
        {
            QModelIndex index = proxyModel->index(0, 0);
            QModelIndex sourceIndex = proxyModel->mapToSource(index);

            const DataTypePaletteModel* paletteModel = m_dataTypeMenu.GetModel();

            return paletteModel->FindTypeIdForIndex(sourceIndex);
        }

        return azrtti_typeid<void>();
    }

    void ContainerTypeLineEdit::SelectType(const AZ::TypeId& typeId)
    {
        if (DisplayType(typeId))
        {
            Q_EMIT TypeChanged(m_index, typeId);
        }
    }

    bool ContainerTypeLineEdit::DisplayType(const AZ::TypeId& typeId)
    {
        if (!m_dataTypeMenu.GetModel()->HasType(typeId))
        {
            return false;
        }

        QSignalBlocker signalBlocker(m_ui->variableType);
        AZStd::string typeName = m_dataTypeMenu.GetModel()->FindTypeNameForTypeId(typeId);

        if (!typeName.empty())
        {
            m_completer.setCompletionPrefix(typeName.c_str());
            m_ui->variableType->setText(typeName.c_str());
            m_lastId = typeId;
        }
        else
        {
            m_completer.setCompletionPrefix("");
            m_ui->variableType->setText("");
            m_lastId = azrtti_typeid<void>();
        }

        // Clear out any selection since this might be coming from an auto complete
        m_ui->variableType->setSelection(0, 0);

        const QPixmap* pixmapIcon = nullptr;
        GraphCanvas::StyleManagerRequestBus::EventResult(pixmapIcon, ScriptCanvasEditor::AssetEditorId, &GraphCanvas::StyleManagerRequests::GetDataTypeIcon, m_lastId);

        m_ui->iconLabel->setPixmap((*pixmapIcon));

        return !typeName.empty();
    }

    QWidget* ContainerTypeLineEdit::GetLineEdit() const
    {
        return m_ui->variableType;
    }

    void ContainerTypeLineEdit::ResetLineEdit()
    {
        m_disableHidingStateSetter.ReleaseState();

        m_lastId = azrtti_typeid<void>();
        m_completer.setCompletionPrefix("");
        m_dataTypeMenu.GetProxyModel()->SetFilter("");

        HideDataTypeMenu();
    }

    void ContainerTypeLineEdit::CancelDataInput()
    {
        DisplayType(m_lastId);
        HideDataTypeMenu();
    }

    void ContainerTypeLineEdit::HideDataTypeMenu()
    {
        m_dataTypeMenu.HideMenu();
    }

    bool ContainerTypeLineEdit::eventFilter(QObject* obj, QEvent* event)
    {
        if (obj == m_ui->variableType)
        {
            switch (event->type())
            {
            case QEvent::FocusOut:
            {
                m_disableHidingStateSetter.ReleaseState();
                QTimer::singleShot(0, [this]() { this->m_dataTypeMenu.reject(); });
                break;
            }
            case QEvent::KeyPress:
            {
                QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

                if (keyEvent->key() == Qt::Key_Down)
                {
                    if (m_dataTypeMenu.isHidden())
                    {
                        m_dataTypeMenu.GetProxyModel()->SetFilter("");
                        DisplayMenu();
                    }

                    int selectedIndex = m_dataTypeMenu.GetSelectedRow();

                    if (selectedIndex < 0)
                    {
                        selectedIndex = 0;
                    }
                    else
                    {
                        selectedIndex += 1;

                        if (selectedIndex >= m_dataTypeMenu.GetProxyModel()->rowCount())
                        {
                            selectedIndex = 0;
                        }
                    }

                    m_dataTypeMenu.SetSelectedRow(selectedIndex);

                    AZ::TypeId typeId = m_dataTypeMenu.GetSelectedTypeId();

                    AZStd::string typeName = m_dataTypeMenu.GetModel()->FindTypeNameForTypeId(typeId);

                    if (!typeName.empty() && !typeId.IsNull())
                    {
                        m_ui->variableType->setText(typeName.c_str());
                        m_ui->variableType->setSelection(0, static_cast<int>(typeName.size()));

                        m_completer.setCompletionPrefix(typeName.c_str());
                    }

                    return true;
                }
                else if (keyEvent->key() == Qt::Key_Up)
                {
                    if (m_dataTypeMenu.isHidden())
                    {
                        m_dataTypeMenu.GetProxyModel()->SetFilter("");
                        DisplayMenu();
                    }

                    int selectedIndex = m_dataTypeMenu.GetSelectedRow();

                    if (selectedIndex < 0)
                    {
                        selectedIndex = m_dataTypeMenu.GetProxyModel()->rowCount() - 1;
                    }
                    else
                    {
                        selectedIndex -= 1;

                        if (selectedIndex < 0)
                        {
                            selectedIndex = m_dataTypeMenu.GetProxyModel()->rowCount() - 1;
                        }
                    }

                    m_dataTypeMenu.SetSelectedRow(selectedIndex);

                    AZ::TypeId typeId = m_dataTypeMenu.GetSelectedTypeId();

                    AZStd::string typeName = m_dataTypeMenu.GetModel()->FindTypeNameForTypeId(typeId);

                    if (!typeName.empty() && !typeId.IsNull())
                    {
                        m_ui->variableType->setText(typeName.c_str());
                        m_ui->variableType->setSelection(0, static_cast<int>(typeName.size()));

                        m_completer.setCompletionPrefix(typeName.c_str());
                    }

                    return true;
                }
                else if (keyEvent->key() == Qt::Key_Escape)
                {
                    DisplayType(m_lastId);
                }
            }
            default:
                break;
            }
        }

        return false;
    }

    void ContainerTypeLineEdit::OnTextChanged()
    {
        DisplayMenu();
        UpdateFilter();
    }

    void ContainerTypeLineEdit::OnOptionsClicked()
    {
        if (m_dataTypeMenu.isHidden())
        {
            m_dataTypeMenu.GetProxyModel()->SetFilter("");
            DisplayMenu();
        }
        else
        {
            m_dataTypeMenu.accept();
        }
    }

    void ContainerTypeLineEdit::OnReturnPressed()
    {
        const bool allowReset = false;
        if (SubmitData(allowReset))
        {
            m_dataTypeMenu.accept();
        }
        else
        {
            m_ui->variableType->setText("");
            UpdateFilter();
        }

        // When we press enter, we will also get an editing complete signal. We want to ignore that since we handled it here.
        m_ignoreNextComplete = true;
    }

    void ContainerTypeLineEdit::OnEditComplete()
    {
        if (m_ignoreNextComplete)
        {
            m_ignoreNextComplete = false;
            return;
        }

        SubmitData();
        QTimer::singleShot(0, [this]() { m_dataTypeMenu.reject(); });
    }

    void ContainerTypeLineEdit::UpdateFilter()
    {
        m_dataTypeMenu.GetProxyModel()->SetFilter(GetUserInputText());
    }

    bool ContainerTypeLineEdit::SubmitData(bool allowReset)
    {
        AZStd::string typeName = m_ui->variableType->text().toUtf8().data();

        AZ::TypeId typeId = m_dataTypeMenu.GetModel()->FindTypeIdForTypeName(typeName);

        // We didn't input a valid type. So default to our last previously known value.
        if (typeId == azrtti_typeid<void>())
        {
            if (allowReset)
            {
                DisplayType(m_lastId);
                typeId = m_lastId;
            }
        }
        else if (typeId != m_lastId)
        {
            SelectType(typeId);
        }

        return typeId != azrtti_typeid<void>();
    }

    void ContainerTypeLineEdit::DisplayMenu()
    {
        if (!m_recursionBlocker)
        {
            QScopedValueRollback<bool> valueRollback(m_recursionBlocker, true);
            if (m_dataTypeMenu.isHidden())
            {
                m_dataTypeMenu.ShowMenu();
                
                QRect dialogGeometry = m_dataTypeMenu.geometry();
                dialogGeometry.moveTopLeft(m_ui->variableType->mapToGlobal(QPoint(0, m_ui->variableType->height())));
                dialogGeometry.setWidth(m_ui->variableType->width());

                m_dataTypeMenu.setGeometry(dialogGeometry);
            }
        }

        if (!m_disableHidingStateSetter.HasState())
        {
            m_disableHidingStateSetter.SetState(true);
        }
    }

    QString ContainerTypeLineEdit::GetUserInputText()
    {
        QString lineEditText = m_ui->variableType->text();

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
        if (m_ui->variableType->completer()
            && m_ui->variableType->completer()->currentCompletion().compare(lineEditText, Qt::CaseInsensitive) == 0
            && !lineEditText.isEmpty())
        {
            lineEditText = m_ui->variableType->completer()->completionPrefix();
        }

        return lineEditText;
    }
}

#include <Editor/View/Dialogs/ContainerWizard/moc_ContainerTypeLineEdit.cpp>
