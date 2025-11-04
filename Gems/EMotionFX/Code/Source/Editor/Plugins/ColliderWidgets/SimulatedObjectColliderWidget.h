/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <Editor/SkeletonModelJointWidget.h>
#include <Editor/Plugins/SimulatedObject/SimulatedObjectActionManager.h>
#endif

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <QPushButton>

namespace Physics
{
    class CharacterColliderNodeConfiguration;
}

namespace EMotionFX
{
    class AddColliderButton;
    class ColliderContainerWidget;
    class NotificationWidget;

    class SimulatedObjectColliderWidget
        : public SkeletonModelJointWidget
    {
        Q_OBJECT //AUTOMOC

    public:
        SimulatedObjectColliderWidget(QWidget* parent = nullptr);

        QString GetCardTitle() const override { return "Object Collider "; }
        QColor GetColor() const override { return QColor{"#1ad092"}; }

        int WidgetCount() const override;

    public slots:
        void OnAddCollider(const AZ::TypeId& colliderType);
        void OnCopyCollider(size_t colliderIndex);
        void OnPasteCollider(size_t colliderIndex, bool replace);
        void OnRemoveCollider(size_t colliderIndex);

    private:
        // SkeletonModelJointWidget
        QWidget* CreateContentWidget(QWidget* parent) override;
        void InternalReinit() override;
        void UpdateOwnershipLabel();
        void UpdateColliderNotification();

        Physics::CharacterColliderNodeConfiguration* GetNodeConfig() const;

        ColliderContainerWidget* m_collidersWidget = nullptr;
        QLabel* m_ownershipLabel = nullptr;
        QWidget* m_ownershipWidget = nullptr;

        QLabel* m_collideWithLabel = nullptr;
        QWidget* m_collideWithWidget = nullptr;

        NotificationWidget* m_colliderNotif = nullptr;

        int m_widgetCount = 0;
    };

    class AddToSimulatedObjectButton
        : public QPushButton
    {
        Q_OBJECT //AUTOMOC

    public:
        AddToSimulatedObjectButton(const QString& text, QWidget* parent = nullptr);

    signals:
        void AddToSimulatedObject();

    private slots:
        void OnCreateContextMenu();
        void OnAddJointsToObjectActionTriggered(bool checked);
        void OnCreateObjectAndAddJointsActionTriggered();

    private:
        AZStd::unique_ptr<EMStudio::SimulatedObjectActionManager> m_actionManager;
    };
} // namespace EMotionFX
