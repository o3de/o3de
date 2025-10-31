/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerBus.h>

#endif


QT_FORWARD_DECLARE_CLASS(QVBoxLayout)

namespace EMotionFX
{
    class Node;
    class RagdollNodeWidget;

    class RagdollOutlinerNotificationHandler
            : public QObject
            , private EMotionFX::SkeletonOutlinerNotificationBus::Handler
    {
        Q_OBJECT //AUTOMOC

    public:

        RagdollOutlinerNotificationHandler(RagdollNodeWidget* nodeWidget);
        ~RagdollOutlinerNotificationHandler() override;

        // SkeletonOutlinerNotificationBus overrides
        void OnContextMenu(QMenu* menu, const QModelIndexList& selectedRowIndices) override;

        static void AddToRagdoll(const QModelIndexList& modelIndices);
        static void RemoveFromRagdoll(const QModelIndexList& modelIndices);
        static bool IsNodeInRagdoll(const QModelIndex& index);
        static void AddCollider(const QModelIndexList& modelIndices, const AZ::TypeId& colliderType);
        static void CopyColliders(const QModelIndexList& modelIndices, PhysicsSetup::ColliderConfigType copyFrom);


    public slots:
        void OnAddToRagdoll();
        void OnAddCollider();
        void OnRemoveFromRagdoll();
        void OnClearColliders();
        void OnPasteJointLimits();

    private:
        bool IsPhysXGemAvailable() const;

        RagdollNodeWidget* m_nodeWidget = nullptr;
    };
} // namespace EMotionFX
