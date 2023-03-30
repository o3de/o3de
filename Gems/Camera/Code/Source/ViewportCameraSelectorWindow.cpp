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
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
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
        CameraListItem::CameraListItem(const AZ::EntityId cameraId)
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

        bool CameraListItem::operator<(const CameraListItem& rhs) const
        {
            return m_cameraId < rhs.m_cameraId;
        }

        CameraListModel::CameraListModel(QWidget* myParent)
            : QAbstractListModel(myParent)
        {
            Reset();
            CameraNotificationBus::Handler::BusConnect();
        }

        CameraListModel::~CameraListModel()
        {
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
            // if the camera entity is not an editor camera entity, don't add it to the list
            // this occurs when we're in simulation mode.

            bool isEditorEntity = false;
            AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                isEditorEntity, &AzToolsFramework::EditorEntityContextRequests::IsEditorEntity, cameraId);

            if (!isEditorEntity)
            {
                return;
            }

            if (auto cameraIt = AZStd::find_if(
                    m_cameraItems.begin(),
                    m_cameraItems.end(),
                    [cameraId](const CameraListItem& entry)
                    {
                        return entry.m_cameraId == cameraId;
                    });
                cameraIt != m_cameraItems.end())
            {
                return;
            }

            beginInsertRows(QModelIndex(), rowCount(), rowCount());
            m_cameraItems.push_back(CameraListItem(cameraId));
            endInsertRows();

            if (m_lastActiveCamera.IsValid() && m_lastActiveCamera == cameraId)
            {
                Camera::CameraRequestBus::Event(cameraId, &Camera::CameraRequestBus::Events::MakeActiveView);
            }
        }

        void CameraListModel::OnCameraRemoved(const AZ::EntityId& cameraId)
        {
            if (auto cameraIt = AZStd::find_if(
                    m_cameraItems.begin(),
                    m_cameraItems.end(),
                    [cameraId](const CameraListItem& entry)
                    {
                        return entry.m_cameraId == cameraId;
                    });
                cameraIt != m_cameraItems.end())
            {
                int listIndex = static_cast<int>(cameraIt - m_cameraItems.begin());
                beginRemoveRows(QModelIndex(), listIndex, listIndex);
                m_cameraItems.erase(cameraIt);
                endRemoveRows();
            }
        }

        void CameraListModel::ConnectCameraNotificationBus()
        {
            CameraNotificationBus::Handler::BusConnect();
        }

        void CameraListModel::DisconnecCameraNotificationBus()
        {
            CameraNotificationBus::Handler::BusDisconnect();
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

        void CameraListModel::Reset()
        {
            m_lastActiveCamera.SetInvalid();
            // add a single invalid entity id to indicate the default Editor Camera (not tied to an entity or component)
            m_cameraItems = AZStd::vector<CameraListItem>(1, CameraListItem(AZ::EntityId()));
        }

        ViewportCameraSelectorWindow::ViewportCameraSelectorWindow(QWidget* parent)
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
                m_cameraList,
                &CameraListModel::rowsInserted,
                this,
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
                // Make sure the selected item is always visible (e.g. when using the arrow keys to change selection)
                if (current.isValid())
                {
                    scrollTo(current);
                }

                QScopedValueRollback<bool> rb(m_ignoreViewportViewEntityChanged, true);

                const AZ::EntityId entityId = selectionModel()->currentIndex().data(Qt::CameraIdRole).value<AZ::EntityId>();
                // only check entity validity if entity is valid, otherwise the change event will be for the editor camera
                if (entityId.IsValid())
                {
                    // if the entity is not in an active state we are most likely in a transition event (going to game mode
                    // or changing level) and we do not want so signal any changes to the camera request bus
                    if (const AZ::Entity* entity = AzToolsFramework::GetEntityById(entityId);
                        !entity || entity->GetState() != AZ::Entity::State::Active)
                    {
                        return;
                    }
                }

                EditorCameraRequests::Bus::Broadcast(&EditorCameraRequests::SetViewFromEntityPerspective, entityId);
            }
        }

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

        // make sure we can only use this window while in Edit mode
        void ViewportCameraSelectorWindow::OnStartPlayInEditorBegin()
        {
            m_cameraList->DisconnecCameraNotificationBus();
            setDisabled(true);
        }

        void ViewportCameraSelectorWindow::OnStopPlayInEditor()
        {
            setDisabled(false);
            m_cameraList->ConnectCameraNotificationBus();
        }

        void ViewportCameraSelectorWindow::OnPrepareForContextReset()
        {
            EditorCameraRequests::Bus::Broadcast(&EditorCameraRequests::SetViewFromEntityPerspective, AZ::EntityId());
        }

        void ViewportCameraSelectorWindow::OnContextReset()
        {
            m_cameraList->Reset();
        }

        // double click selects the entity
        void ViewportCameraSelectorWindow::mouseDoubleClickEvent([[maybe_unused]] QMouseEvent* event)
        {
            if (const AZ::EntityId entityId = selectionModel()->currentIndex().data(Qt::CameraIdRole).value<AZ::EntityId>();
                entityId.IsValid())
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
    } // namespace Internal

    void RegisterViewportCameraSelectorWindow()
    {
        AzToolsFramework::ViewPaneOptions viewOptions;
        viewOptions.isPreview = true;
        viewOptions.showInMenu = true;
        viewOptions.preferedDockingArea = Qt::DockWidgetArea::LeftDockWidgetArea;
        AzToolsFramework::EditorRequestBus::Broadcast(
            &AzToolsFramework::EditorRequestBus::Events::RegisterViewPane,
            s_viewportCameraSelectorName,
            "Viewport",
            viewOptions,
            &Internal::CreateNewSelectionWindow);
    }
} // namespace Camera
