/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "TVEventsDialog.h"

// Qt
#include <QInputDialog>
#include <QMessageBox>

// CryCommon
#include <CryCommon/Maestro/Types/AnimNodeType.h>
#include <CryCommon/Maestro/Types/AnimParamType.h>

// Editor
#include "AnimationContext.h"


AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <TrackView/ui_TVEventsDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

// CTVEventsDialog dialog

class TVEventsModel
    : public QAbstractTableModel
{
public:
    TVEventsModel(QObject* parent = nullptr)
        : QAbstractTableModel(parent)
    {
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        if (parent.isValid())
        {
            return 0;
        }
        CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
        AZ_Assert(sequence, "Sequence is null");
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

        CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
        AZ_Assert(sequence, "Sequence is null");

        AzToolsFramework::ScopedUndoBatch undo("Remove Track Event");

        for (int r = row, max = row + count; r < max; ++r)
        {
            const QString eventName = index(r, 0).data().toString();
            bool remove = true;
            float timeFirstUsed;
            int usageCount = GetNumberOfUsageAndFirstTimeUsed(eventName.toStdString().c_str(), timeFirstUsed);
            if (usageCount > 0)
            {
                remove = QMessageBox::warning(
                             nullptr,
                             tr("Remove Event"),
                             tr("Remove \"") + eventName + tr("\" event might cause some link breakages in Flow Graph.\nStill continue?"),
                             QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
            }

            if (remove)
            {
                beginRemoveRows(QModelIndex(), r, r);
                result &= sequence->RemoveTrackEvent(eventName.toUtf8().data());
                endRemoveRows();
                undo.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
            }
        }

        return result;
    }

    bool AddRow(const QString& name)
    {
        CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
        AZ_Assert(sequence, "Sequence is null");
        const int index = rowCount();
        beginInsertRows(QModelIndex(), index, index);
        bool result = false;

        AzToolsFramework::ScopedUndoBatch undo("Add Track Event");
        result = sequence->AddTrackEvent(name.toUtf8().data());
        undo.MarkEntityDirty(sequence->GetSequenceComponentEntityId());

        endInsertRows();
        if (!result)
        {
            beginRemoveRows(QModelIndex(), index, index);
            endRemoveRows();
        }
        return result;
    }

    bool MoveRow(const QModelIndex& index, bool up)
    {
        CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
        AZ_Assert(sequence, "Sequence is null");
        if (!index.isValid() || (up && index.row() == 0) || (!up && index.row() == rowCount() - 1))
        {
            return false;
        }

        bool result = false;

        AzToolsFramework::ScopedUndoBatch undo("Move Track Event");
        if (up)
        {
            beginMoveRows(QModelIndex(), index.row(), index.row(), QModelIndex(), index.row() - 1);
            result = sequence->MoveUpTrackEvent(index.sibling(index.row(), 0).data().toString().toUtf8().data());
        }
        else
        {
            beginMoveRows(QModelIndex(), index.row() + 1, index.row() + 1, QModelIndex(), index.row());
            result = sequence->MoveDownTrackEvent(index.sibling(index.row(), 0).data().toString().toUtf8().data());
        }
        undo.MarkEntityDirty(sequence->GetSequenceComponentEntityId());

        endMoveRows();

        return result;
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
        AZ_Assert(sequence, "Sequence is null");
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
        CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
        AZ_Assert(sequence, "Sequence is null");
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

        AzToolsFramework::ScopedUndoBatch undo("Set Track Event Data");
        result = sequence->RenameTrackEvent(oldName.toUtf8().data(), newName.toUtf8().data());
        undo.MarkEntityDirty(sequence->GetSequenceComponentEntityId());

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

CTVEventsDialog::CTVEventsDialog(QWidget* pParent /*=nullptr*/)
    : QDialog(pParent)
    , m_ui(new Ui::TVEventsDialog)
{
    m_ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    OnInitDialog();

    connect(m_ui->buttonAddEvent, &QPushButton::clicked, this, &CTVEventsDialog::OnBnClickedButtonAddEvent);
    connect(m_ui->buttonRemoveEvent, &QPushButton::clicked, this, &CTVEventsDialog::OnBnClickedButtonRemoveEvent);
    connect(m_ui->buttonRenameEvent, &QPushButton::clicked, this, &CTVEventsDialog::OnBnClickedButtonRenameEvent);
    connect(m_ui->buttonUpEvent, &QPushButton::clicked, this, &CTVEventsDialog::OnBnClickedButtonUpEvent);
    connect(m_ui->buttonDownEvent, &QPushButton::clicked, this, &CTVEventsDialog::OnBnClickedButtonDownEvent);
    connect(m_ui->m_List->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CTVEventsDialog::OnListItemChanged);
}

CTVEventsDialog::~CTVEventsDialog()
{
}

// CTVEventsDialog message handlers

void CTVEventsDialog::OnBnClickedButtonAddEvent()
{
    const QString add = QInputDialog::getText(this, tr("Track Event Name"), QString());
    if (!add.isEmpty() && static_cast<TVEventsModel*>(m_ui->m_List->model())->AddRow(add))
    {
        m_lastAddedEvent = add;
        m_ui->m_List->setCurrentIndex(m_ui->m_List->model()->index(m_ui->m_List->model()->rowCount() - 1, 0));
    }
    m_ui->m_List->setFocus();
}

void CTVEventsDialog::OnBnClickedButtonRemoveEvent()
{
    QList<QPersistentModelIndex> indexes;
    for (auto index : m_ui->m_List->selectionModel()->selectedRows())
    {
        indexes.push_back(index);
    }

    for (auto index : indexes)
    {
        m_ui->m_List->model()->removeRow(index.row());
    }
    m_ui->m_List->setFocus();
}

void CTVEventsDialog::OnBnClickedButtonRenameEvent()
{
    const QModelIndex index = m_ui->m_List->currentIndex();
    QString oldName = m_ui->m_List->model()->index(index.row(), 0).data().toString();
    if (index.isValid())
    {
        const QString newName = QInputDialog::getText(this, tr("Track Event Name"), QString(), QLineEdit::Normal, oldName);
        if (!newName.isEmpty())
        {
            m_ui->m_List->model()->setData(index.sibling(index.row(), 0), newName);
        }
    }
    m_ui->m_List->setFocus();
}

void CTVEventsDialog::OnBnClickedButtonUpEvent()
{
    static_cast<TVEventsModel*>(m_ui->m_List->model())->MoveRow(m_ui->m_List->currentIndex(), true);
    UpdateButtons();
    m_ui->m_List->setFocus();
}

void CTVEventsDialog::OnBnClickedButtonDownEvent()
{
    static_cast<TVEventsModel*>(m_ui->m_List->model())->MoveRow(m_ui->m_List->currentIndex(), false);
    UpdateButtons();
    m_ui->m_List->setFocus();
}

void CTVEventsDialog::OnInitDialog()
{
    m_ui->m_List->setModel(new TVEventsModel(this));
    m_ui->m_List->header()->resizeSections(QHeaderView::ResizeToContents);

    AZ_Assert(GetIEditor()->GetAnimation()->GetSequence(), "Current sequence is null");

    UpdateButtons();
}

void CTVEventsDialog::OnListItemChanged()
{
    UpdateButtons();
}

void CTVEventsDialog::UpdateButtons()
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

const QString& CTVEventsDialog::GetLastAddedEvent()
{
    return m_lastAddedEvent;
}

int TVEventsModel::GetNumberOfUsageAndFirstTimeUsed(const char* eventName, float& timeFirstUsed) const
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();

    int usageCount = 0;
    float firstTime = std::numeric_limits<float>::max();

    CTrackViewAnimNodeBundle nodeBundle = sequence->GetAnimNodesByType(AnimNodeType::Event);
    const unsigned int numNodes = nodeBundle.GetCount();

    for (unsigned int currentNode = 0; currentNode < numNodes; ++currentNode)
    {
        CTrackViewAnimNode* pCurrentNode = nodeBundle.GetNode(currentNode);

        CTrackViewTrackBundle tracks = pCurrentNode->GetTracksByParam(AnimParamType::TrackEvent);
        const unsigned int numTracks = tracks.GetCount();

        for (unsigned int currentTrack = 0; currentTrack < numTracks; ++currentTrack)
        {
            CTrackViewTrack* pTrack = tracks.GetTrack(currentTrack);

            for (unsigned int currentKey = 0; currentKey < pTrack->GetKeyCount(); ++currentKey)
            {
                CTrackViewKeyHandle keyHandle = pTrack->GetKey(currentKey);

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

#include <TrackView/moc_TVEventsDialog.cpp>
