/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <QConnectionsWidget.h>

#include <ACEEnums.h>
#include <AudioControl.h>
#include <AudioControlsEditorPlugin.h>
#include <IAudioSystemControl.h>
#include <IAudioSystemEditor.h>
#include <ImplementationManager.h>

#include <QDropEvent>
#include <QEvent>
#include <QMimeData>
#include <QMessageBox>
#include <QMenu>

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    QConnectionsWidget::QConnectionsWidget(QWidget* parent)
        : QWidget(parent)
        , m_control(nullptr)
        , m_notFoundColor(QColor(0xf3, 0x81, 0x1d))
        , m_localizedColor(QColor(0x42, 0x85, 0xf4))
    {
        setupUi(this);

        m_connectionList->viewport()->installEventFilter(this);
        m_connectionList->installEventFilter(this);

        connect(m_connectionList, SIGNAL(customContextMenuRequested(const QPoint&)), SLOT(ShowConnectionContextMenu(const QPoint&)));
    }

    //-------------------------------------------------------------------------------------------//
    void QConnectionsWidget::CurrentConnectionModified()
    {
        if (m_control)
        {
            m_control->SignalControlModified();
        }
    }

    //-------------------------------------------------------------------------------------------//
    bool QConnectionsWidget::eventFilter(QObject* object, QEvent* event)
    {
        IAudioSystemEditor* audioSystemImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();
        if (audioSystemImpl && m_control && event->type() == QEvent::Drop)
        {
            QDropEvent* dropEvent = static_cast<QDropEvent*>(event);
            const QMimeData* mimeData = dropEvent->mimeData();
            QString format = "application/x-qabstractitemmodeldatalist";
            if (mimeData->hasFormat(format))
            {
                QByteArray encoded = mimeData->data(format);
                QDataStream stream(&encoded, QIODevice::ReadOnly);
                while (!stream.atEnd())
                {
                    int row, col;
                    QMap<int,  QVariant> roleDataMap;
                    stream >> row >> col >> roleDataMap;
                    if (!roleDataMap.isEmpty())
                    {
                        if (IAudioSystemControl* middlewareControl = audioSystemImpl->GetControl(roleDataMap[eMDR_ID].toUInt()))
                        {
                            if (audioSystemImpl->GetCompatibleTypes(m_control->GetType()) & middlewareControl->GetType())
                            {
                                MakeConnectionTo(middlewareControl);
                            }
                        }
                    }
                }
            }
            return true;
        }
        else if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* dropEvent = static_cast<QKeyEvent*>(event);
            if (dropEvent && dropEvent->key() == Qt::Key_Delete && object == m_connectionList)
            {
                RemoveSelectedConnection();
                return true;
            }
        }
        return QWidget::eventFilter(object, event);
    }

    //-------------------------------------------------------------------------------------------//
    void QConnectionsWidget::ShowConnectionContextMenu(const QPoint& pos)
    {
        QMenu contextMenu(tr("Context menu"), this);
        contextMenu.addAction(tr("Remove Connection"), this, SLOT(RemoveSelectedConnection()));
        contextMenu.exec(m_connectionList->mapToGlobal(pos));
    }

    //-------------------------------------------------------------------------------------------//
    void QConnectionsWidget::MakeConnectionTo(IAudioSystemControl* middlewareControl)
    {
        IAudioSystemEditor* audioSystemImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();
        if (m_control && middlewareControl && audioSystemImpl)
        {
            CUndo undo("Connected Audio Control to Audio System");

            TConnectionPtr connection = m_control->GetConnection(middlewareControl);
            if (connection)
            {
                // connection already exists, select it
                const int size = m_connectionList->count();
                for (int i = 0; i < size; ++i)
                {
                    QListWidgetItem* listItem = m_connectionList->item(i);
                    if (listItem && listItem->data(eMDR_ID).toInt() == static_cast<int>(middlewareControl->GetId()))
                    {
                        m_connectionList->clearSelection();
                        listItem->setSelected(true);
                        m_connectionList->setCurrentItem(listItem);
                        m_connectionList->scrollToItem(listItem);
                        break;
                    }
                }
            }
            else
            {
                connection = audioSystemImpl->CreateConnectionToControl(m_control->GetType(), middlewareControl);
                if (connection)
                {
                    m_control->AddConnection(connection);
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void QConnectionsWidget::RemoveSelectedConnection()
    {
        CUndo undo("Disconnected Audio Control from Audio System");
        if (m_control)
        {
            QMessageBox messageBox(this);
            messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            messageBox.setDefaultButton(QMessageBox::Yes);
            messageBox.setWindowTitle("Audio Controls Editor");
            QList<QListWidgetItem*> selected = m_connectionList->selectedItems();
            const int size = selected.length();
            if (size > 0)
            {
                if (size == 1)
                {
                    messageBox.setText("Are you sure you want to delete the connection between \"" + QString(m_control->GetName().c_str()) + "\" and \"" + selected[0]->text() + "\"?");
                }
                else
                {
                    messageBox.setText("Are you sure you want to delete the " + QString::number(size) + " selected connections?");
                }
                if (messageBox.exec() == QMessageBox::Yes)
                {
                    if (IAudioSystemEditor* audioSystemImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl())
                    {
                        AZStd::vector<IAudioSystemControl*> connectedMiddlewareControls;
                        connectedMiddlewareControls.reserve(selected.size());
                        for (int i = 0; i < size; ++i)
                        {
                            CID middlewareControlId = selected[i]->data(eMDR_ID).toInt();
                            connectedMiddlewareControls.push_back(audioSystemImpl->GetControl(middlewareControlId));
                        }

                        for (int i = 0; i < size; ++i)
                        {
                            if (IAudioSystemControl* middlewareControl = connectedMiddlewareControls[i])
                            {
                                audioSystemImpl->ConnectionRemoved(middlewareControl);
                                m_control->RemoveConnection(middlewareControl);
                            }
                        }
                    }
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void QConnectionsWidget::UpdateConnections()
    {
        m_connectionList->clear();
        IAudioSystemEditor* audioSystemImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();
        if (audioSystemImpl && m_control)
        {
            m_connectionList->setEnabled(true);
            const size_t size = m_control->ConnectionCount();
            for (size_t i = 0; i < size; ++i)
            {
                TConnectionPtr connection = m_control->GetConnectionAt(i);
                if (connection)
                {
                    if (IAudioSystemControl* middlewareControl = audioSystemImpl->GetControl(connection->GetID()))
                    {
                        CreateItemFromConnection(middlewareControl);
                    }
                }
            }
        }
        else
        {
            m_connectionList->setEnabled(false);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void QConnectionsWidget::CreateItemFromConnection(IAudioSystemControl* middlewareControl)
    {
        if (IAudioSystemEditor* audioSystemImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl())
        {
            const TImplControlType type = middlewareControl->GetType();

            QIcon icon = QIcon(QString(audioSystemImpl->GetTypeIcon(type).data()));
            icon.addFile(QString(audioSystemImpl->GetTypeIconSelected(type).data()), QSize(), QIcon::Selected);

            QListWidgetItem* listItem = new QListWidgetItem(icon, QString(middlewareControl->GetName().c_str()));
            listItem->setData(eMDR_ID, middlewareControl->GetId());
            listItem->setData(eMDR_LOCALIZED, middlewareControl->IsLocalized());
            if (middlewareControl->IsPlaceholder())
            {
                listItem->setToolTip(tr("Control not found in currently loaded audio system project"));
                listItem->setForeground(m_notFoundColor);
            }
            else if (middlewareControl->IsLocalized())
            {
                listItem->setForeground(m_localizedColor);
            }
            m_connectionList->insertItem(0, listItem);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void QConnectionsWidget::SetControl(CATLControl* control)
    {
        if (m_control != control)
        {
            m_control = control;
            m_connectionList->clear();
            UpdateConnections();
        }
        else if (m_control->ConnectionCount() != m_connectionList->count())
        {
            UpdateConnections();
        }
    }

} // namespace AudioControls

#include <Source/Editor/moc_QConnectionsWidget.cpp>
