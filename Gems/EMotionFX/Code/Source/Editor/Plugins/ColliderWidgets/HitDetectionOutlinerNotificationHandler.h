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

    class HitDetectionOutlinerNotificationHandler
        : public QObject
        , private EMotionFX::SkeletonOutlinerNotificationBus::Handler
    {
        Q_OBJECT //AUTOMOC

    public:
        HitDetectionOutlinerNotificationHandler(HitDetectionJointWidget* jointWidget);
        ~HitDetectionOutlinerNotificationHandler();

        // SkeletonOutlinerNotificationBus overrides
        void OnContextMenu(QMenu* menu, const QModelIndexList& selectedRowIndices) override;

    public slots:
        void OnAddCollider();
        void OnClearColliders();

    private:
        HitDetectionJointWidget* m_nodeWidget;
    };
} // namespace EMotionFX
