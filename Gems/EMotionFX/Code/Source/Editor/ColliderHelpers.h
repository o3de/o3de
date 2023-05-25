/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/PhysicsSetup.h>
#include <QModelIndexList>

QT_FORWARD_DECLARE_CLASS(QLayout)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QObject)

namespace EMotionFX
{
    class ColliderHelpers
    {
    public:
        static QString GetMimeTypeForColliderShape()
        {
            return QString("com.amazon.lumberyard/%1").arg(azrtti_typeid<AzPhysics::ShapeColliderPair>().ToFixedString().c_str());
        }

        static void AddCopyColliderCommandToGroup(const Actor* actor, const Node* joint, PhysicsSetup::ColliderConfigType copyFrom, PhysicsSetup::ColliderConfigType copyTo, MCore::CommandGroup& commandGroup);
        static void CopyColliders(const QModelIndexList& modelIndices, PhysicsSetup::ColliderConfigType copyFrom, PhysicsSetup::ColliderConfigType copyTo, bool removeExistingColliders = true);
        static void AddCollider(const QModelIndexList& modelIndices, PhysicsSetup::ColliderConfigType addTo, const AZ::TypeId& colliderType);
        static void AddToRagdoll(const QModelIndexList& modelIndices);
        static void RemoveFromRagdoll(const QModelIndexList &modelIndices);
        static void ClearColliders(const QModelIndexList& modelIndices, PhysicsSetup::ColliderConfigType removeFrom);
        static bool AreCollidersReflected();
        static bool CanCopyFrom(const QModelIndexList& modelIndices, PhysicsSetup::ColliderConfigType copyFrom);

        static void AddCopyFromMenu(QObject* parent, QMenu* parentMenu, PhysicsSetup::ColliderConfigType createForType, const QModelIndexList& modelIndices);
        static void AddCopyFromMenu(QObject* parent, QMenu* parentMenu, PhysicsSetup::ColliderConfigType createForType, const QModelIndexList& modelIndices,
            const AZStd::function<void(PhysicsSetup::ColliderConfigType copyFrom, PhysicsSetup::ColliderConfigType copyTo)>& copyFunc);

        static void CopyColliderToClipboard(const QModelIndex& modelIndex, size_t shapeIndex, PhysicsSetup::ColliderConfigType type);
        static void PasteColliderFromClipboard(const QModelIndex& modelIndex, size_t shapeIndex, PhysicsSetup::ColliderConfigType type, bool replace);

        static bool NodeHasRagdoll(const QModelIndex& modelIndex);
        static bool NodeHasClothCollider(const QModelIndex& modelIndex);
        static bool NodeHasHitDetection(const QModelIndex& modelIndex);
    };
} // namespace EMotionFX
