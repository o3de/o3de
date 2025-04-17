/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <Editor/Plugins/ColliderWidgets/HitDetectionOutlinerNotificationHandler.h>
#include <Editor/SkeletonModelJointWidget.h>
#endif

namespace Physics
{
    class CharacterColliderNodeConfiguration;
};

namespace EMotionFX
{
    class AddColliderButton;
    class ColliderContainerWidget;

    class HitDetectionJointWidget : public SkeletonModelJointWidget
    {
        Q_OBJECT // AUTOMOC

    public:
        HitDetectionJointWidget(QWidget* parent = nullptr);
        ~HitDetectionJointWidget() = default;

        QString GetCardTitle() const override
        {
            return "Hit Detection";
        }
        QColor GetColor() const override
        {
            return QColor{ "#4A90E2" };
        }

    public slots:
        void OnAddCollider(const AZ::TypeId& colliderType);
        void OnCopyCollider(size_t colliderIndex);
        void OnPasteCollider(size_t colliderIndex, bool replace);
        void OnRemoveCollider(size_t colliderIndex);

    public:
        HitDetectionOutlinerNotificationHandler m_handler;

        int WidgetCount() const override;

    private:
        // SkeletonModelJointWidget
        QWidget* CreateContentWidget(QWidget* parent) override;
        void InternalReinit() override;

        Physics::CharacterColliderNodeConfiguration* GetNodeConfig() const;
        int m_widgetCount = 0;
    };
} // namespace EMotionFX
