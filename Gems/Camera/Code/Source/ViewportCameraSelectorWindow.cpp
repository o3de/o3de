/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Camera_precompiled.h"
#include "ViewportCameraSelectorWindow.h"
#include "ViewportCameraSelectorWindow_Internals.h"
#include <IEditor.h>
#include <qmetatype.h>
#include <QScopedValueRollback>
#include <QListView>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>
#include <QLabel>
#include <ViewManager.h>
#include <Maestro/Bus/EditorSequenceBus.h>
#include <Maestro/Bus/SequenceComponentBus.h>

namespace Qt
{
    enum
    {
        CameraIdRole = Qt::UserRole + 1,
    };
}
Q_DECLARE_METATYPE(AZ::EntityId);

namespace Camera
{
    struct ViewportCameraSelectorWindow;
    struct CameraListItem;
    struct CameraListModel;
    struct ViewportSelectorHolder;

    namespace Internal
    {
        CameraListItem::CameraListItem(const AZ::EntityId& cameraId)
            : m_cameraId(cameraId)
            , m_sequenceId(AZ::EntityId())
        {
            if (cameraId.IsValid())
            {
                AZ::ComponentApplicationBus::BroadcastResult(m_cameraName, &AZ::ComponentApplicationRequests::GetEntityName, cameraId);
                AZ::EntityBus::Handler::BusConnect(cameraId);
            }
            else
            {
                m_cameraName = "Editor camera";
            }
        }

        // Used for a virtual camera that is really whatever camera is being
        // used for a Track View Sequence.
        CameraListItem::CameraListItem(const char* cameraName, const AZ::EntityId& sequenceId)
            : m_cameraName(cameraName)
            , m_sequenceId(sequenceId)
        {
            Maestro::SequenceComponentNotificationBus::Handler::BusConnect(sequenceId);
        }

        CameraListItem::~CameraListItem()
        {
            Maestro::SequenceComponentNotificationBus::Handler::BusDisconnect();

            if (m_cameraId.IsValid())
            {
                AZ::EntityBus::Handler::BusDisconnect(m_cameraId);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        /// Maestro::SequenceComponentNotificationBus::Handler
        void CameraListItem::OnCameraChanged(const AZ::EntityId& oldCameraEntityId, const AZ::EntityId& newCameraEntityId)
        {
            AZ_UNUSED(oldCameraEntityId);
            m_cameraId = newCameraEntityId;
        }

        bool CameraListItem::operator<(const CameraListItem& rhs)
        {
            return m_cameraId < rhs.m_cameraId;
        }

        CameraListModel::CameraListModel(QObject* myParent)
            : QAbstractListModel(myParent)
        {
            m_cameraItems.push_back(AZ::EntityId());
            CameraNotificationBus::Handler::BusConnect();
            Maestro::EditorSequenceNotificationBus::Handler::BusConnect();
        }

        CameraListModel::~CameraListModel()
        {
            // set the view entity id back to Invalid, thus enabling the editor camera
            EditorCameraRequests::Bus::Broadcast(&EditorCameraRequests::SetViewFromEntityPerspective, AZ::EntityId());

            Maestro::EditorSequenceNotificationBus::Handler::BusDisconnect();
            CameraNotificationBus::Handler::BusDisconnect();
        }

        int CameraListModel::rowCount([[maybe_unused]] const QModelIndex& parent) const
        {
            return m_cameraItems.size();
        }

        QVariant CameraListModel::data(const QModelIndex& index, int role) const
        {
            if (role == Qt::DisplayRole)
            {
                return m_cameraItems[index.row()].m_cameraName.c_str();
            }
            else if (role == Qt::CameraIdRole)
            {
                return QVariant::fromValue(m_cameraItems[index.row()].m_cameraId);
            }
            return QVariant();
        }

        void CameraListModel::OnCameraAdded(const AZ::EntityId& cameraId)
        {
            beginInsertRows(QModelIndex(), rowCount(), rowCount());
            m_cameraItems.push_back(cameraId);
            endInsertRows();
        }

        void CameraListModel::OnCameraRemoved(const AZ::EntityId& cameraId)
        {
            auto cameraIt = AZStd::find_if(m_cameraItems.begin(), m_cameraItems.end(),
                [&cameraId](const CameraListItem& entry)
                {
                    return entry.m_cameraId == cameraId;
                });
            if (cameraIt != m_cameraItems.end())
            {
                int listIndex = cameraIt - m_cameraItems.begin();
                beginRemoveRows(QModelIndex(), listIndex, listIndex);
                m_cameraItems.erase(cameraIt);
                endRemoveRows();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        /// Maestro::EditorSequenceNotificationBus::Handler
        void CameraListModel::OnSequenceSelected(const AZ::EntityId& sequenceEntityId)
        {
            // Add or Remove the Sequence Camera option if a valid
            // sequence is selected in Track View.

            // Check to see if the Sequence Camera option is already present
            bool found = false;
            int index = 0;
            for (const CameraListItem& cameraItem : m_cameraItems)
            {
                if (cameraItem.m_cameraName == m_sequenceCameraName)
                {
                    found = true;
                    break;
                }
                ++index;
            }

            // If it is present, but no sequence is selected, removed it.
            if (found && !sequenceEntityId.IsValid())
            {
                beginRemoveRows(QModelIndex(), index, index);
                m_cameraItems.erase(m_cameraItems.begin() + index);
                endRemoveRows();
            }
            // If it is not present, and there is a sequence selected show it.
            else if (!found && sequenceEntityId.IsValid())
            {
                beginInsertRows(QModelIndex(), rowCount(), rowCount());
                m_cameraItems.push_back(CameraListItem(m_sequenceCameraName, sequenceEntityId));
                endInsertRows();
            }
        }

        QModelIndex CameraListModel::GetIndexForEntityId(const AZ::EntityId entityId)
        {
            int row = 0;
            for (const CameraListItem& cameraItem : m_cameraItems)
            {
                if (cameraItem.m_cameraId == entityId)
                {
                    break;
                }
                ++row;
            }
            return index(row, 0);
        }

        const char* CameraListModel::m_sequenceCameraName = "Sequence camera";

        ViewportCameraSelectorWindow::ViewportCameraSelectorWindow(QWidget* parent)
            : m_ignoreViewportViewEntityChanged(false)
        {
            qRegisterMetaType<AZ::EntityId>("AZ::EntityId");
            setParent(parent);
            setSelectionMode(QAbstractItemView::SingleSelection);
            setViewMode(ViewMode::ListMode);

            // display camera list
            m_cameraList = new CameraListModel(this);

            // sort by entity id
            QSortFilterProxyModel* sortedProxyModel = new QSortFilterProxyModel(this);
            sortedProxyModel->setSourceModel(m_cameraList);
            setModel(sortedProxyModel);
            sortedProxyModel->setSortRole(Qt::CameraIdRole);

            // use the stylesheet for elements in a set where one item must be selected at all times
            setProperty("class", "SingleRequiredSelection");
            connect(m_cameraList, &CameraListModel::rowsInserted, this, [sortedProxyModel](const QModelIndex&, int, int) { sortedProxyModel->sortColumn(); });

            // highlight the current selected camera entity
            AZ::EntityId currentSelection;
            EditorCameraRequestBus::BroadcastResult(currentSelection, &EditorCameraRequestBus::Events::GetCurrentViewEntityId);
            OnViewportViewEntityChanged(currentSelection);

            // bus connections
            EditorCameraNotificationBus::Handler::BusConnect();
            AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
            Maestro::EditorSequenceNotificationBus::Handler::BusConnect();
        }

        ViewportCameraSelectorWindow::~ViewportCameraSelectorWindow()
        {
            if (Maestro::SequenceComponentNotificationBus::Handler::BusIsConnected())
            {
                Maestro::SequenceComponentNotificationBus::Handler::BusDisconnect();
            }

            Maestro::EditorSequenceNotificationBus::Handler::BusDisconnect();
            AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
            EditorCameraNotificationBus::Handler::BusDisconnect();
        }

        void ViewportCameraSelectorWindow::currentChanged(const QModelIndex& current, const QModelIndex& previous)
        {
            if (current.row() != previous.row())
            {
                // Lock camera editing when in sequence camera mode.
                const AZStd::string& selectedCameraName = selectionModel()->currentIndex().data(Qt::DisplayRole).toString().toUtf8().data();
                bool lockCameraMovement = (selectedCameraName == CameraListModel::m_sequenceCameraName);

                QScopedValueRollback<bool> rb(m_ignoreViewportViewEntityChanged, true);
                AZ::EntityId entityId = selectionModel()->currentIndex().data(Qt::CameraIdRole).value<AZ::EntityId>();
                EditorCameraRequests::Bus::Broadcast(&EditorCameraRequests::SetViewAndMovementLockFromEntityPerspective, entityId, lockCameraMovement);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        /// EditorCameraNotificationBus::Handler
        void ViewportCameraSelectorWindow::OnViewportViewEntityChanged(const AZ::EntityId& newViewId)
        {
            if (!m_ignoreViewportViewEntityChanged)
            {
                QModelIndex potentialIndex = m_cameraList->GetIndexForEntityId(newViewId);
                if (model()->hasIndex(potentialIndex.row(), potentialIndex.column()))
                {
                    selectionModel()->setCurrentIndex(potentialIndex, QItemSelectionModel::SelectionFlag::ClearAndSelect);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        /// AzToolsFramework::EditorEntityContextRequestBus::Handler
        // make sure we can only use this window while in Edit mode
        void ViewportCameraSelectorWindow::OnStartPlayInEditor()
        {
            setDisabled(true);
        }

        void ViewportCameraSelectorWindow::OnStopPlayInEditor()
        {
            setDisabled(false);
        }

        //////////////////////////////////////////////////////////////////////////
        /// Maestro::EditorSequenceNotificationBus::Handler
        void ViewportCameraSelectorWindow::OnSequenceSelected(const AZ::EntityId& sequenceEntityId)
        {
            // Connect to the Sequence Component Bus when a sequence is selected for OnCameraChanged.
            if (Maestro::SequenceComponentNotificationBus::Handler::BusIsConnected())
            {
                Maestro::SequenceComponentNotificationBus::Handler::BusDisconnect();
            }
            if (sequenceEntityId.IsValid())
            {
                Maestro::SequenceComponentNotificationBus::Handler::BusConnect(sequenceEntityId);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        /// Maestro::SequenceComponentNotificationBus::Handler
        void ViewportCameraSelectorWindow::OnCameraChanged(const AZ::EntityId& oldCameraEntityId, const AZ::EntityId& newCameraEntityId)
        {
            AZ_UNUSED(oldCameraEntityId);

            // If the Sequence camera option is selected, respond to camera changes by selecting the camera used by the sequence.
            const AZStd::string& selectedCameraName = selectionModel()->currentIndex().data(Qt::DisplayRole).toString().toUtf8().data();
            if (selectedCameraName == CameraListModel::m_sequenceCameraName)
            {
                QScopedValueRollback<bool> rb(m_ignoreViewportViewEntityChanged, true);
                EditorCameraRequests::Bus::Broadcast(&EditorCameraRequests::SetViewAndMovementLockFromEntityPerspective, newCameraEntityId, true);
            }
        }

        // swallow mouse move events so we can disable sloppy selection
        void ViewportCameraSelectorWindow::mouseMoveEvent(QMouseEvent*) {}

        // double click selects the entity
        void ViewportCameraSelectorWindow::mouseDoubleClickEvent([[maybe_unused]] QMouseEvent* event)
        {
            AZ::EntityId entityId = selectionModel()->currentIndex().data(Qt::CameraIdRole).value<AZ::EntityId>();
            if (entityId.IsValid())
            {
                AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequestBus::Events::SetSelectedEntities, AzToolsFramework::EntityIdList { entityId });
            }
            else
            {
                AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequestBus::Events::SetSelectedEntities, AzToolsFramework::EntityIdList {});
            }
        }

        // handle up/down arrows to make a circular list
        QModelIndex ViewportCameraSelectorWindow::moveCursor(CursorAction cursorAction, [[maybe_unused]] Qt::KeyboardModifiers modifiers)
        {
            switch (cursorAction)
            {
            case CursorAction::MoveUp:
            {
                return GetPreviousIndex();
            }
            case CursorAction::MoveDown:
            {
                return GetNextIndex();
            }
            case CursorAction::MoveNext:
            {
                return GetNextIndex();
            }
            case CursorAction::MovePrevious:
            {
                return GetPreviousIndex();
            }
            default:
                return currentIndex();
            }
        }

        QModelIndex ViewportCameraSelectorWindow::GetPreviousIndex() const
        {
            QModelIndex current = currentIndex();
            int previousRow = current.row() - 1;
            int rowCount = qobject_cast<QSortFilterProxyModel*>(model())->sourceModel()->rowCount();
            if (previousRow < 0)
            {
                previousRow = rowCount - 1;
            }
            return model()->index(previousRow, 0);
        }

        QModelIndex ViewportCameraSelectorWindow::GetNextIndex() const
        {
            QModelIndex current = currentIndex();
            int nextRow = current.row() + 1;
            int rowCount = qobject_cast<QSortFilterProxyModel*>(model())->sourceModel()->rowCount();
            if (nextRow >= rowCount)
            {
                nextRow = 0;
            }
            return model()->index(nextRow, 0);
        }

        ViewportSelectorHolder::ViewportSelectorHolder(QWidget* parent)
            : QWidget(parent)
        {
            setLayout(new QVBoxLayout(this));
            auto label = new QLabel("Select the camera you wish to view and navigate through.  Closing this window will return you to the default editor camera.", this);
            label->setWordWrap(true);
            layout()->addWidget(label);
            layout()->addWidget(new ViewportCameraSelectorWindow(this));
        }

        // simple factory method
        ViewportSelectorHolder* CreateNewSelectionWindow(QWidget* parent)
        {
            return new ViewportSelectorHolder(parent);
        }
    } // namespace Camera::Internal

    void RegisterViewportCameraSelectorWindow()
    {
        QtViewOptions viewOptions;
        viewOptions.isPreview = true;
        viewOptions.showInMenu = true;
        viewOptions.preferedDockingArea = Qt::DockWidgetArea::LeftDockWidgetArea;
        AzToolsFramework::EditorRequestBus::Broadcast(&AzToolsFramework::EditorRequestBus::Events::RegisterViewPane, s_viewportCameraSelectorName, "Viewport", viewOptions, &Internal::CreateNewSelectionWindow);
    }
} // namespace Camera
