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
    class ClothJointWidget;

    class ClothJointInspectorPlugin
        : public EMStudio::DockWidgetPlugin
        , private EMotionFX::SkeletonOutlinerNotificationBus::Handler
    {
        Q_OBJECT //AUTOMOC

    public:
        enum : uint32
        {
            CLASS_ID = 0x8efd2bee
        };
        ClothJointInspectorPlugin();
        ~ClothJointInspectorPlugin();

        // EMStudioPlugin overrides
        const char* GetName() const override                { return "Cloth Colliders"; }
        uint32 GetClassID() const override                  { return CLASS_ID; }
        bool GetIsClosable() const override                 { return true;  }
        bool GetIsFloatable() const override                { return true;  }
        bool GetIsVertical() const override                 { return false; }
        bool Init() override;
        EMStudioPlugin* Clone() const override              { return new ClothJointInspectorPlugin(); }

        // SkeletonOutlinerNotificationBus overrides
        void OnContextMenu(QMenu* menu, const QModelIndexList& selectedRowIndices) override;

        void LegacyRender(EMStudio::RenderPlugin* renderPlugin, RenderInfo* renderInfo) override;
        static bool IsJointInCloth(const QModelIndex& index);

    public slots:
        void OnAddCollider();
        void OnClearColliders();

    private:
        bool IsNvClothGemAvailable() const;

        ClothJointWidget* m_jointWidget;
    };
} // namespace EMotionFX
