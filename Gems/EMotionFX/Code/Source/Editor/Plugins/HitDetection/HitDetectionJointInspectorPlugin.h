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
    class HitDetectionJointWidget;

    class HitDetectionJointInspectorPlugin
        : public EMStudio::DockWidgetPlugin
        , private EMotionFX::SkeletonOutlinerNotificationBus::Handler
    {
        Q_OBJECT //AUTOMOC

    public:
        HitDetectionJointInspectorPlugin();
        ~HitDetectionJointInspectorPlugin();

        // EMStudioPlugin overrides
        const char* GetName() const override                { return "Hit Detection"; }
        uint32 GetClassID() const override                  { return 0x00047155; }
        bool GetIsClosable() const override                 { return true;  }
        bool GetIsFloatable() const override                { return true;  }
        bool GetIsVertical() const override                 { return false; }
        bool Init() override;
        EMStudioPlugin* Clone() const override              { return new HitDetectionJointInspectorPlugin(); }

        // SkeletonOutlinerNotificationBus overrides
        void OnContextMenu(QMenu* menu, const QModelIndexList& selectedRowIndices) override;

        void LegacyRender(EMStudio::RenderPlugin* renderPlugin, RenderInfo* renderInfo) override;

    public slots:
        void OnAddCollider();
        void OnClearColliders();

    private:
        HitDetectionJointWidget* m_nodeWidget;
    };
} // namespace EMotionFX
