/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "UiAVEventsDialog.h"
#include <Editor/Animation/ui_UiAVEventsDialog.h>
#include "UiAnimViewUndo.h"
#include "UiAnimViewSequence.h"
#include "AnimationContext.h"
#include <limits>

#include <QInputDialog>
#include <QMessageBox>


// CUiAVEventsDialog dialog

class UiAVEventsModel
    : public QAbstractTableModel
{
public:
    UiAVEventsModel(QObject* parent = nullptr)
        : QAbstractTableModel(parent)
    {
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        if (parent.isValid())
        {
            return 0;
        }
        CUiAnimViewSequence* sequence = CUiAnimViewSequenceManager::GetSequenceManager()->GetCurrentSequence();
        assert(sequence);
        return sequence->GetTrackEventsCount();
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 3;
    }

    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override
    {
        if (parent.isValid())
        {
            return false;
        }

        bool result = true;

        CUiAnimViewSequence* sequence = CUiAnimViewSequenceManager::GetSequenceManager()->GetCurrentSequence();
        assert(sequence);

        UiAnimUndo undo("Remove Track Event");
        for (int r = row; r < row + count; ++r)
        {
            const QString eventName = index(r, 0).data().toString();
            UiAnimUndo::Record(new CUndoTrackEventRemove(sequence, eventName));

            beginRemoveRows(QModelIndex(), r, r);
            result &= sequence->RemoveTrackEvent(eventName.toUtf8().data());
            endRemoveRows();
        }

        return result;
    }

    bool addRow(const QString& name)
    {
        CUiAnimViewSequence* sequence = CUiAnimViewSequenceManager::GetSequenceManager()->GetCurrentSequence();
        assert(sequence);
        const int index = rowCount();
        beginInsertRows(QModelIndex(), index, index);
        bool result = false;

        UiAnimUndo undo("Add Track Event");
        UiAnimUndo::Record(new CUndoTrackEventAdd(sequence, name));
        result = sequence->AddTrackEvent(name.toUtf8().data());

        endInsertRows();
        if (!result)
        {
            beginRemoveRows(QModelIndex(), index, index);
            endRemoveRows();
        }
        return result;
    }

    bool moveRow(const QModelIndex& index, bool up)
    {
        CUiAnimViewSequence* sequence = CUiAnimViewSequenceManager::GetSequenceManager()->GetCurrentSequence();
        assert(sequence);
        if (!index.isValid() || (up && index.row() == 0) || (!up && index.row() == rowCount() - 1))
        {
            return false;
        }

        bool result = false;

        UiAnimUndo undo("Move Track Event");
        const QString name = index.sibling(index.row(), 0).data().toString();
        if (up)
        {
            UiAnimUndo::Record(new CUndoTrackEventMoveUp(sequence, name));
            beginMoveRows(QModelIndex(), index.row(), index.row(), QModelIndex(), index.row() - 1);
            result = sequence->MoveUpTrackEvent(name.toUtf8().data());
        }
        else
        {
            UiAnimUndo::Record(new CUndoTrackEventMoveDown(sequence, name));
            beginMoveRows(QModelIndex(), index.row() + 1, index.row() + 1, QModelIndex(), index.row());
            result = sequence->MoveDownTrackEvent(name.toUtf8().data());
        }

        endMoveRows();

        return result;
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        CUiAnimViewSequence* sequence = CUiAnimViewSequenceManager::GetSequenceManager()->GetCurrentSequence();
        assert(sequence);
        if (role != Qt::DisplayRole)
        {
            return QVariant();
        }

        float timeFirstUsed;
        int usageCount = GetNumberOfUsageAndFirstTimeUsed(sequence->GetTrackEvent(index.row()), timeFirstUsed);

        switch (index.column())
        {
        case 0:
            return QString::fromLatin1(sequence->GetTrackEvent(index.row()));
        case 1:
            return usageCount;
        case 2:
            return usageCount > 0 ? QString::number(timeFirstUsed, 'f', 3) : QString();
        default:
            return QVariant();
        }
    }

    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override
    {
        CUiAnimViewSequence* sequence = CUiAnimViewSequenceManager::GetSequenceManager()->GetCurrentSequence();
        assert(sequence);
        if (role != Qt::DisplayRole && role != Qt::EditRole)
        {
            return false;
        }
        if (index.column() != 0 || value.toString().isEmpty())
        {
            return false;
        }

        bool result = false;

        const QString oldName = index.data().toString();
        const QString newName = value.toString();

        UiAnimUndo undo("Rename Track Event");
        UiAnimUndo::Record(new CUndoTrackEventRename(sequence, oldName, newName));
        result = sequence->RenameTrackEvent(oldName.toUtf8().data(), newName.toUtf8().data());

        emit dataChanged(index, index);
        return result;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        {
            return QVariant();
        }

        switch (section)
        {
        case 0:
            return tr("Event");
        case 1:
            return tr("# of use");
        case 2:
            return tr("Time of first usage");
        default:
            return QVariant();
        }
    }

    int GetNumberOfUsageAndFirstTimeUsed(const char* eventName, float& timeFirstUsed) const;
};

CUiAVEventsDialog::CUiAVEventsDialog(QWidget* pParent /*=nullptr*/)
    : QDialog(pParent)
    , m_ui(new Ui::UiAVEventsDialog)
{
    m_ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    OnInitDialog();

    connect(m_ui->buttonAddEvent, &QPushButton::clicked, this, &CUiAVEventsDialog::OnBnClickedButtonAddEvent);
    connect(m_ui->buttonRemoveEvent, &QPushButton::clicked, this, &CUiAVEventsDialog::OnBnClickedButtonRemoveEvent);
    connect(m_ui->buttonRenameEvent, &QPushButton::clicked, this, &CUiAVEventsDialog::OnBnClickedButtonRenameEvent);
    connect(m_ui->buttonUpEvent, &QPushButton::clicked, this, &CUiAVEventsDialog::OnBnClickedButtonUpEvent);
    connect(m_ui->buttonDownEvent, &QPushButton::clicked, this, &CUiAVEventsDialog::OnBnClickedButtonDownEvent);
    connect(m_ui->m_List->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CUiAVEventsDialog::OnListItemChanged);
}

CUiAVEventsDialog::~CUiAVEventsDialog()
{
}

// CTVEventsDialog message handlers

void CUiAVEventsDialog::OnBnClickedButtonAddEvent()
{
    const QString add = QInputDialog::getText(this, tr("Track Event Name"), QString());
    if (!add.isEmpty() && static_cast<UiAVEventsModel*>(m_ui->m_List->model())->addRow(add))
    {
        m_lastAddedEvent = add;
        m_ui->m_List->setCurrentIndex(m_ui->m_List->model()->index(m_ui->m_List->model()->rowCount() - 1, 0));
    }
    m_ui->m_List->setFocus();
}

void CUiAVEventsDialog::OnBnClickedButtonRemoveEvent()
{
    QList<QPersistentModelIndex> indexes;
    for (auto index : m_ui->m_List->selectionModel()->selectedRows())
    {
        indexes.push_back(index);
    }

    for (auto index : indexes)
    {
        if (QMessageBox::warning(this, tr("Remove Event"), tr("This removal will remove all uses of this event.\nAll listeners will fail to trigger.\nStill continue?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
        {
            m_ui->m_List->model()->removeRow(index.row());
        }
    }
    m_ui->m_List->setFocus();
}

void CUiAVEventsDialog::OnBnClickedButtonRenameEvent()
{
    const QModelIndex index = m_ui->m_List->currentIndex();

    if (index.isValid())
    {
        const QString newName = QInputDialog::getText(this, tr("Track Event Name"), QString());
        if (!newName.isEmpty())
        {
            m_lastAddedEvent = newName;
            m_ui->m_List->model()->setData(index.sibling(index.row(), 0), newName);
        }
    }
    m_ui->m_List->setFocus();
}

void CUiAVEventsDialog::OnBnClickedButtonUpEvent()
{
    static_cast<UiAVEventsModel*>(m_ui->m_List->model())->moveRow(m_ui->m_List->currentIndex(), true);
    UpdateButtons();
    m_ui->m_List->setFocus();
}

void CUiAVEventsDialog::OnBnClickedButtonDownEvent()
{
    static_cast<UiAVEventsModel*>(m_ui->m_List->model())->moveRow(m_ui->m_List->currentIndex(), false);
    UpdateButtons();
    m_ui->m_List->setFocus();
}

void CUiAVEventsDialog::OnInitDialog()
{
    m_ui->m_List->setModel(new UiAVEventsModel(this));
    m_ui->m_List->header()->resizeSections(QHeaderView::ResizeToContents);

    UpdateButtons();
}

void CUiAVEventsDialog::OnListItemChanged()
{
    UpdateButtons();
}

void CUiAVEventsDialog::UpdateButtons()
{
    bool bRemove = false, bRename = false, bUp = false, bDown = false;

    int nSelected = m_ui->m_List->selectionModel()->selectedRows().count();
    if (nSelected > 1)
    {
        bRemove = true;
        bRename = false;
    }
    else if (nSelected > 0)
    {
        bRemove = bRename = true;

        const QModelIndex index = m_ui->m_List->selectionModel()->selectedRows().first();
        if (index.row() > 0)
        {
            bUp = true;
        }
        if (index.row() < m_ui->m_List->model()->rowCount() - 1)
        {
            bDown = true;
        }
    }

    m_ui->buttonRemoveEvent->setEnabled(bRemove);
    m_ui->buttonRenameEvent->setEnabled(bRename);
    m_ui->buttonUpEvent->setEnabled(bUp);
    m_ui->buttonDownEvent->setEnabled(bDown);
}

const QString& CUiAVEventsDialog::GetLastAddedEvent()
{
    return m_lastAddedEvent;
}

int UiAVEventsModel::GetNumberOfUsageAndFirstTimeUsed(const char* eventName, float& timeFirstUsed) const
{
    CUiAnimViewSequence* sequence = CUiAnimViewSequenceManager::GetSequenceManager()->GetCurrentSequence();

    int usageCount = 0;
    float firstTime = std::numeric_limits<float>::max();

    CUiAnimViewAnimNodeBundle nodeBundle = sequence->GetAnimNodesByType(eUiAnimNodeType_Event);
    const unsigned int numNodes = nodeBundle.GetCount();

    for (unsigned int currentNode = 0; currentNode < numNodes; ++currentNode)
    {
        CUiAnimViewAnimNode* pCurrentNode = nodeBundle.GetNode(currentNode);

        CUiAnimViewTrackBundle tracks = pCurrentNode->GetTracksByParam(eUiAnimParamType_TrackEvent);
        const unsigned int numTracks = tracks.GetCount();

        for (unsigned int currentTrack = 0; currentTrack < numTracks; ++currentTrack)
        {
            CUiAnimViewTrack* pTrack = tracks.GetTrack(currentTrack);

            for (unsigned int currentKey = 0; currentKey < pTrack->GetKeyCount(); ++currentKey)
            {
                CUiAnimViewKeyHandle keyHandle = pTrack->GetKey(currentKey);

                IEventKey key;
                keyHandle.GetKey(&key);

                if (strcmp(key.event.c_str(), eventName) == 0) // If it has a key with the specified event set
                {
                    ++usageCount;
                    if (key.time < firstTime)
                    {
                        firstTime = key.time;
                    }
                }
            }
        }
    }

    if (usageCount > 0)
    {
        timeFirstUsed = firstTime;
    }
    return usageCount;
}

#include <Animation/moc_UiAVEventsDialog.cpp>

