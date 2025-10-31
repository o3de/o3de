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
#include <QObject>
#endif


QT_FORWARD_DECLARE_CLASS(QVBoxLayout)

namespace EMotionFX
{
    class Node;
    class ClothJointWidget;

    class ClothOutlinerNotificationHandler
        : public QObject
        , private EMotionFX::SkeletonOutlinerNotificationBus::Handler
    {
        Q_OBJECT //AUTOMOC

    public:
        ClothOutlinerNotificationHandler(ClothJointWidget* m_colliderWidget);
        ~ClothOutlinerNotificationHandler();

        // SkeletonOutlinerNotificationBus overrides
        void OnContextMenu(QMenu* menu, const QModelIndexList& selectedRowIndices) override;

        static bool IsJointInCloth(const QModelIndex& index);

    public slots:
        void OnAddCollider();
        void OnClearColliders();

    private:
        bool IsNvClothGemAvailable() const;
        ClothJointWidget* m_colliderWidget = nullptr;
    };
} // namespace EMotionFX
