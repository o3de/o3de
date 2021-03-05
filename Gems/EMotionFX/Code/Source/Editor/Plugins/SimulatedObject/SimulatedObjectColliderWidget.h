/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

    public slots:
        void OnAddCollider(const AZ::TypeId& colliderType);
        void OnCopyCollider(size_t colliderIndex);
        void OnPasteCollider(size_t colliderIndex, bool replace);
        void OnRemoveCollider(size_t colliderIndex);

    private:
        // SkeletonModelJointWidget
        QWidget* CreateContentWidget(QWidget* parent) override;
        QWidget* CreateNoSelectionWidget(QWidget* parent) override;
        void InternalReinit() override;
        void UpdateOwnershipLabel();
        void UpdateColliderNotification();

        Physics::CharacterColliderNodeConfiguration* GetNodeConfig() const;

        ColliderContainerWidget* m_collidersWidget = nullptr;
        QLabel* m_ownershipLabel = nullptr;
        QWidget* m_ownershipWidget = nullptr;

        QLabel* m_collideWithLabel = nullptr;
        QWidget* m_collideWithWidget = nullptr;

        QLabel* m_instruction1 = nullptr;
        QLabel* m_instruction2 = nullptr;

        NotificationWidget* m_colliderNotif = nullptr;
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
