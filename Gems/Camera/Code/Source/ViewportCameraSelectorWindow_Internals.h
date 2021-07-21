/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QAbstractItemModel>
#include <QListView>
#include <Maestro/Bus/EditorSequenceBus.h>
#include <Maestro/Bus/SequenceComponentBus.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzCore/Component/EntityBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/API/EditorCameraBus.h>

namespace Camera
{
    namespace Internal
    {
        // Each item in the list holds the camera's entityId and name
        struct CameraListItem
            : public AZ::EntityBus::Handler
            , public Maestro::SequenceComponentNotificationBus::Handler
        {
        public:
            CameraListItem(const AZ::EntityId& cameraId);
            // Used for a virtual camera that is really whatever camera is being
            // used for a Track View Sequence.
            CameraListItem(const char* cameraName, const AZ::EntityId& sequenceId);
            ~CameraListItem();

            void OnEntityNameChanged(const AZStd::string& name) override { m_cameraName = name; }

            //////////////////////////////////////////////////////////////////////////
            /// Maestro::SequenceComponentNotificationBus::Handler
            void OnCameraChanged(const AZ::EntityId& oldCameraEntityId, const AZ::EntityId& newCameraEntityId) override;
            bool operator<(const CameraListItem& rhs);

            AZ::EntityId m_cameraId;
            AZStd::string m_cameraName;
            AZ::EntityId m_sequenceId;
        };

        // holds a list of camera items
        struct CameraListModel
            : public QAbstractListModel
            , public CameraNotificationBus::Handler
            , public Maestro::EditorSequenceNotificationBus::Handler
        {
        public:

            static const char* m_sequenceCameraName;

            CameraListModel(QObject* myParent);
            ~CameraListModel();

            // QAbstractItemModel interface
            int rowCount(const QModelIndex& parent = QModelIndex()) const override;
            QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

            // CameraNotificationBus interface
            void OnCameraAdded(const AZ::EntityId& cameraId) override;
            void OnCameraRemoved(const AZ::EntityId& cameraId) override;

            //////////////////////////////////////////////////////////////////////////
            /// Maestro::EditorSequenceNotificationBus::Handler
            void OnSequenceSelected(const AZ::EntityId& sequenceEntityId) override;
            QModelIndex GetIndexForEntityId(const AZ::EntityId entityId);

        private:
            AZStd::vector<CameraListItem> m_cameraItems;
            AZ::EntityId m_sequenceCameraEntityId;
            bool m_sequenceCameraSelected;
        };

        struct ViewportCameraSelectorWindow
            : public QListView
            , public EditorCameraNotificationBus::Handler
            , public AzToolsFramework::EditorEntityContextNotificationBus::Handler
            , public Maestro::EditorSequenceNotificationBus::Handler
            , public Maestro::SequenceComponentNotificationBus::Handler
        {
        public:
            ViewportCameraSelectorWindow(QWidget* parent = nullptr);
            ~ViewportCameraSelectorWindow();

            void currentChanged(const QModelIndex& current, const QModelIndex& previous) override;

            //////////////////////////////////////////////////////////////////////////
            /// EditorCameraNotificationBus::Handler
            void OnViewportViewEntityChanged(const AZ::EntityId& newViewId) override;

            //////////////////////////////////////////////////////////////////////////
            /// AzToolsFramework::EditorEntityContextRequestBus::Handler
            // make sure we can only use this window while in Edit mode
            void OnStartPlayInEditor() override;
            void OnStopPlayInEditor() override;

            //////////////////////////////////////////////////////////////////////////
            /// Maestro::EditorSequenceNotificationBus::Handler
            void OnSequenceSelected(const AZ::EntityId& sequenceEntityId) override;

            //////////////////////////////////////////////////////////////////////////
            /// Maestro::SequenceComponentNotificationBus::Handler
            void OnCameraChanged(const AZ::EntityId& oldCameraEntityId, const AZ::EntityId& newCameraEntityId) override;

            void mouseMoveEvent(QMouseEvent*) override;
            void mouseDoubleClickEvent(QMouseEvent* event) override;
            QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
            QModelIndex GetPreviousIndex() const;
            QModelIndex GetNextIndex() const;

        private:
            CameraListModel* m_cameraList;
            bool m_ignoreViewportViewEntityChanged;
        };

        // wrapper for the ViewportCameraSelectorWindow so that we can add some descriptive helpful text
        struct ViewportSelectorHolder
            : public QWidget
        {
            explicit ViewportSelectorHolder(QWidget* parent = nullptr);
        };

        // simple factory method
        ViewportSelectorHolder* CreateNewSelectionWindow(QWidget* parent = nullptr);
    } // namespace Camera::Internal
} // namespace Camera
