/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/DockWidgetPlugin.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerBus.h>
#endif


QT_FORWARD_DECLARE_CLASS(QVBoxLayout)

namespace EMotionFX
{
    class Node;
    class RagdollNodeWidget;

    class RagdollNodeInspectorPlugin
        : public EMStudio::DockWidgetPlugin
        , private EMotionFX::SkeletonOutlinerNotificationBus::Handler
    {
        Q_OBJECT //AUTOMOC

    public:
        enum
        {
            CLASS_ID = 0x4c0b81e2
        };

        RagdollNodeInspectorPlugin();
        ~RagdollNodeInspectorPlugin() override;

        // EMStudioPlugin overrides
        const char* GetName() const override                { return "Ragdoll"; }
        uint32 GetClassID() const override                  { return CLASS_ID; }
        bool GetIsClosable() const override                 { return true;  }
        bool GetIsFloatable() const override                { return true;  }
        bool GetIsVertical() const override                 { return false; }
        bool Init() override;
        EMStudioPlugin* Clone() const override              { return new RagdollNodeInspectorPlugin(); }

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

        RagdollNodeWidget* m_nodeWidget;
    };
} // namespace EMotionFX
