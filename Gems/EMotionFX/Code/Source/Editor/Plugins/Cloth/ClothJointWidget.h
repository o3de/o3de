/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <Editor/SkeletonModelJointWidget.h>
#endif


namespace Physics
{
    class CharacterColliderNodeConfiguration;
}

namespace EMotionFX
{
    class AddColliderButton;
    class ColliderContainerWidget;

    class ClothJointWidget
        : public SkeletonModelJointWidget
    {
        Q_OBJECT //AUTOMOC

    public:
        ClothJointWidget(QWidget* parent = nullptr);
        ~ClothJointWidget() = default;

        static Physics::CharacterColliderNodeConfiguration* GetNodeConfig(const QModelIndex& modelIndex);

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

        Physics::CharacterColliderNodeConfiguration* GetNodeConfig() const;

        AddColliderButton*          m_addColliderButton;
        ColliderContainerWidget*    m_collidersWidget;
    };
} // namespace EMotionFX
