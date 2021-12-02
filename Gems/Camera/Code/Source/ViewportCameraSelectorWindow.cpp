/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "ViewportCameraSelectorWindow.h"
#include "ViewportCameraSelectorWindow_Internals.h"
#include <AzToolsFramework/API/EditorCameraBus.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <QLabel>
#include <QListView>
#include <QScopedValueRollback>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>
#include <qmetatype.h>

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

        CameraListItem::~CameraListItem()
        {
            if (m_cameraId.IsValid())
            {
                AZ::EntityBus::Handler::BusDisconnect(m_cameraId);
            }
        }

        bool CameraListItem::operator<(const CameraListItem& rhs)
        {
            return m_cameraId < rhs.m_cameraId;
        }

        CameraListModel::CameraListModel(QWidget* myParent)
            : QAbstractListModel(myParent)
        {
            m_lastActiveCamera = AZ::EntityId();
            m_cameraItems.push_back(AZ::EntityId());
            CameraNotificationBus::Handler::BusConnect();
        }

        CameraListModel::~CameraListModel()
        {
            m_firstEntry = true;
            // set the view entity id back to Invalid, thus enabling the editor camera
            EditorCameraRequests::Bus::Broadcast(&EditorCameraRequests::SetViewFromEntityPerspective, AZ::EntityId());

            CameraNotificationBus::Handler::BusDisconnect();
        }

        int CameraListModel::rowCount([[maybe_unused]] const QModelIndex& parent) const
        {
            return static_cast<int>(m_cameraItems.size());
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
            // If the camera entity is not an editor camera entity, don't add it to the list.
            // This occurs when we're in simulation mode.

            //We reset the m_firstEntry value so we can update m_lastActiveCamera when we remove from the cameras list
            m_firstEntry = true;

            bool isEditorEntity = false;
            AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                isEditorEntity, &AzToolsFramework::EditorEntityContextRequests::IsEditorEntity, cameraId);
            if (!isEditorEntity)
            {
                return;
            }

            beginInsertRows(QModelIndex(), rowCount(), rowCount());
            m_cameraItems.push_back(cameraId);
            endInsertRows();

            if (m_lastActiveCamera.IsValid() && m_lastActiveCamera == cameraId)
            {
                Camera::CameraRequestBus::Event(cameraId, &Camera::CameraRequestBus::Events::MakeActiveView);
            }
        }

        void CameraListModel::OnCameraRemoved(const AZ::EntityId& cameraId)
        {
            //Check it is the first time we remove a camera from the list before any other addition
            //So we don't end up with the wrong camera ID.
            if (m_firstEntry)
            {
                CameraSystemRequestBus::BroadcastResult(m_lastActiveCamera, &CameraSystemRequestBus::Events::GetActiveCamera);
                m_firstEntry = false;
            }

            auto cameraIt = AZStd::find_if(
                m_cameraItems.begin(), m_cameraItems.end(),
                [&cameraId](const CameraListItem& entry)
                {
                    return entry.m_cameraId == cameraId;
                });
            if (cameraIt != m_cameraItems.end())
            {
                int listIndex = static_cast<int>(cameraIt - m_cameraItems.begin());
                beginRemoveRows(QModelIndex(), listIndex, listIndex);
                m_cameraItems.erase(cameraIt);
                endRemoveRows();
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
            connect(
                m_cameraList, &CameraListModel::rowsInserted, this,
                [sortedProxyModel](const QModelIndex&, int, int)
                {
                    sortedProxyModel->sortColumn();
                });

            // highlight the current selected camera entity
            AZ::EntityId currentSelection;
            EditorCameraRequestBus::BroadcastResult(currentSelection, &EditorCameraRequestBus::Events::GetCurrentViewEntityId);
            OnViewportViewEntityChanged(currentSelection);

            // bus connections
            EditorCameraNotificationBus::Handler::BusConnect();
            AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        }

        ViewportCameraSelectorWindow::~ViewportCameraSelectorWindow()
        {
            AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
            EditorCameraNotificationBus::Handler::BusDisconnect();
        }

        void ViewportCameraSelectorWindow::currentChanged(const QModelIndex& current, const QModelIndex& previous)
        {
            if (current.row() != previous.row())
            {
                bool lockCameraMovement = false;

                QScopedValueRollback<bool> rb(m_ignoreViewportViewEntityChanged, true);
                AZ::EntityId entityId = selectionModel()->currentIndex().data(Qt::CameraIdRole).value<AZ::EntityId>();
                EditorCameraRequests::Bus::Broadcast(
                    &EditorCameraRequests::SetViewAndMovementLockFromEntityPerspective, entityId, lockCameraMovement);
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

        // swallow mouse move events so we can disable sloppy selection
        void ViewportCameraSelectorWindow::mouseMoveEvent(QMouseEvent*)
        {
        }

        // double click selects the entity
        void ViewportCameraSelectorWindow::mouseDoubleClickEvent([[maybe_unused]] QMouseEvent* event)
        {
            AZ::EntityId entityId = selectionModel()->currentIndex().data(Qt::CameraIdRole).value<AZ::EntityId>();
            if (entityId.IsValid())
            {
                AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                    &AzToolsFramework::ToolsApplicationRequestBus::Events::SetSelectedEntities, AzToolsFramework::EntityIdList{ entityId });
            }
            else
            {
                AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                    &AzToolsFramework::ToolsApplicationRequestBus::Events::SetSelectedEntities, AzToolsFramework::EntityIdList{});
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
            auto label = new QLabel(
                "Select the camera you wish to view and navigate through.  Closing this window will return you to the default editor "
                "camera.",
                this);
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
        AzToolsFramework::ViewPaneOptions viewOptions;
        viewOptions.isPreview = true;
        viewOptions.showInMenu = true;
        viewOptions.preferedDockingArea = Qt::DockWidgetArea::LeftDockWidgetArea;
        AzToolsFramework::EditorRequestBus::Broadcast(
            &AzToolsFramework::EditorRequestBus::Events::RegisterViewPane, s_viewportCameraSelectorName, "Viewport", viewOptions,
            &Internal::CreateNewSelectionWindow);
    }
} // namespace Camera
