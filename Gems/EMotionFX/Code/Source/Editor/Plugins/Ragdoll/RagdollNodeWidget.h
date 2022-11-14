/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzFramework/Physics/Ragdoll.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <Editor/SkeletonModelJointWidget.h>
#include <QWidget>
#include <Editor/Plugins/Ragdoll/RagdollOutlinerNotificationHandler.h>
#include <Editor/Plugins/Ragdoll/PhysicsSetupViewportUiCluster.h>
#endif

QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace Physics
{
    class RagdollNodeConfiguration;
}

namespace EMotionFX
{
    class ObjectEditor;
    class AddColliderButton;
    class ColliderContainerWidget;
    class RagdollJointLimitWidget;

    class RagdollCardHeader
        : public AzQtComponents::CardHeader
    {
    public:
        RagdollCardHeader(QWidget* parent = nullptr);
    };

    class RagdollCard
        : public AzQtComponents::Card
    {
    public:
        RagdollCard(QWidget* parent = nullptr);
    };

    class RagdollNodeWidget
        : public SkeletonModelJointWidget
    {
        Q_OBJECT //AUTOMOC

    public:
        RagdollNodeWidget(QWidget* parent = nullptr);
        ~RagdollNodeWidget() = default;

        bool HasCopiedJointLimits() const { return !m_copiedJointLimit.empty(); }
        const AZStd::string& GetCopiedJointLimits() const { return m_copiedJointLimit; }

        virtual QString GetCardTitle() const override { return "Ragdoll"; }
        virtual QColor GetColor() const override { return QColor{"#f5a623"}; }
    public slots:
        void OnAddRemoveRagdollNode();

        void OnAddCollider(const AZ::TypeId& colliderType);
        void OnCopyCollider(size_t colliderIndex);
        void OnPasteCollider(size_t colliderIndex, bool replace);
        void OnRemoveCollider(size_t colliderIndex);

    protected:
        RagdollOutlinerNotificationHandler m_handler;
    private:
        // SkeletonModelJointWidget
        QWidget* CreateContentWidget(QWidget* parent) override;
        void InternalReinit() override;

        Physics::RagdollConfiguration* GetRagdollConfig() const;
        Physics::CharacterColliderNodeConfiguration* GetRagdollColliderNodeConfig() const;
        Physics::RagdollNodeConfiguration* GetRagdollNodeConfig() const;

        // Ragdoll node
        AzQtComponents::Card*         m_ragdollNodeCard;
        EMotionFX::ObjectEditor*      m_ragdollNodeEditor;
        //QPushButton*                  m_addRemoveButton;

        // Joint limit
        RagdollJointLimitWidget*      m_jointLimitWidget;

        AZStd::string                 m_copiedJointLimit;

        PhysicsSetupViewportUiCluster m_physicsSetupViewportUiCluster;
    };
} // namespace EMotionFX
