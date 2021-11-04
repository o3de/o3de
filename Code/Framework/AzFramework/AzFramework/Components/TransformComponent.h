/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/EBus/Event.h>

namespace AzToolsFramework
{
    namespace Components
    {
        class TransformComponent;
    }
}

namespace AzFramework
{
    class GameEntityContextComponent;

    /// @deprecated Use AZ::TransformConfig
    using TransformComponentConfiguration = AZ::TransformConfig;

    //! Fundamental component that describes the entity in 3D space.
    class TransformComponent
        : public AZ::Component
        , public AZ::EntityBus::Handler
        , public AZ::TransformBus::Handler
        , public AZ::TransformNotificationBus::Handler
        , private AZ::TransformHierarchyInformationBus::Handler
    {
    public:
        AZ_COMPONENT(TransformComponent, AZ::TransformComponentTypeId, AZ::TransformInterface);

        friend class AzToolsFramework::Components::TransformComponent;

        using ParentActivationTransformMode = AZ::TransformConfig::ParentActivationTransformMode;

        TransformComponent() = default;
        TransformComponent(const TransformComponent& copy);
        ~TransformComponent() override = default;

        // TransformBus events (publicly accessible)
        void BindTransformChangedEventHandler(AZ::TransformChangedEvent::Handler& handler) override;
        void BindParentChangedEventHandler(AZ::ParentChangedEvent::Handler& handler) override;
        void BindChildChangedEventHandler(AZ::ChildChangedEvent::Handler& handler) override;
        void NotifyChildChangedEvent(AZ::ChildChangeType changeType, AZ::EntityId entityId) override;
        //! Returns true if the tm was set to the local transform.
        const AZ::Transform& GetLocalTM() override { return m_localTM; }
        //! Returns true if the tm was set to the world transform.
        const AZ::Transform& GetWorldTM() override { return m_worldTM; }
        //! Returns both local and world transforms.
        void GetLocalAndWorld(AZ::Transform& localTM, AZ::Transform& worldTM) override { localTM = m_localTM; worldTM = m_worldTM; }
        //! Returns parent EntityId.
        AZ::EntityId GetParentId() override { return m_parentId; }
        //! Returns parent interface if available.
        AZ::TransformInterface* GetParent() override { return m_parentTM; }
        //! Sets the local transform and notifies all interested parties.
        void SetLocalTM(const AZ::Transform& tm) override;
        //! Sets the world transform and notifies all interested parties.
        void SetWorldTM(const AZ::Transform& tm) override;
        //! Set parent entity and notifies all interested parties.
        //! The object localTM will be moved into parent space so we will preserve the same worldTM.
        void SetParent(AZ::EntityId id) override;
        //! Set the parent entity and notifies all interested parties.
        //! This will use worldTM as a localTM and move the transform relative to the parent.
        void SetParentRelative(AZ::EntityId id) override;

    protected:

        // Component
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        // Translation modifiers
        void SetWorldTranslation(const AZ::Vector3& newPosition) override;
        void SetLocalTranslation(const AZ::Vector3& newPosition) override;

        AZ::Vector3 GetWorldTranslation() override;
        AZ::Vector3 GetLocalTranslation() override;

        void MoveEntity(const AZ::Vector3& offset) override;

        void SetWorldX(float x) override;
        void SetWorldY(float y) override;
        void SetWorldZ(float z) override;

        float GetWorldX() override;
        float GetWorldY() override;
        float GetWorldZ() override;

        void SetLocalX(float x) override;
        void SetLocalY(float y) override;
        void SetLocalZ(float z) override;

        float GetLocalX() override;
        float GetLocalY() override;
        float GetLocalZ() override;

        // Rotation modifiers
        void SetWorldRotation(const AZ::Vector3& eulerAnglesRadian) override;
        void SetWorldRotationQuaternion(const AZ::Quaternion& quaternion) override;

        AZ::Vector3 GetWorldRotation() override;
        AZ::Quaternion GetWorldRotationQuaternion() override;

        void SetLocalRotation(const AZ::Vector3& eulerAnglesRadian) override;
        void SetLocalRotationQuaternion(const AZ::Quaternion& quaternion) override;

        void RotateAroundLocalX(float eulerAngleRadian) override;
        void RotateAroundLocalY(float eulerAngleRadian) override;
        void RotateAroundLocalZ(float eulerAngleRadian) override;

        AZ::Vector3 GetLocalRotation() override;
        AZ::Quaternion GetLocalRotationQuaternion() override;

        // Scale Modifiers
        AZ::Vector3 GetLocalScale() override;

        void SetLocalUniformScale(float scale) override;
        float GetLocalUniformScale() override;
        float GetWorldUniformScale() override;

        // Transform hierarchy
        AZStd::vector<AZ::EntityId> GetChildren() override;
        AZStd::vector<AZ::EntityId> GetAllDescendants() override;
        AZStd::vector<AZ::EntityId> GetEntityAndAllDescendants() override;
        bool IsStaticTransform() override;

        //! Methods implementing parent support.
        //! @{
        // TransformNotificationBus - for parent entity
        void OnTransformChanged(const AZ::Transform& parentLocalTM, const AZ::Transform& parentWorldTM) override;

        // EntityBus
        //! Called when the parent entity activates.
        void OnEntityActivated(const AZ::EntityId& parentEntityId) override;
        //! Called when the parent entity deactivates.
        void OnEntityDeactivated(const AZ::EntityId& parentEntityId) override;
        //! @}

        //////////////////////////////////////////////////////////////////////////
        // Actual Implementation Functions
        // They are protected so we can gate them when network-controlled
        void SetParentImpl(AZ::EntityId parentId, bool isKeepWorldTM);
        void SetLocalTMImpl(const AZ::Transform& tm);
        void SetWorldTMImpl(const AZ::Transform& tm);
        void OnTransformChangedImpl(const AZ::Transform& parentLocalTM, const AZ::Transform& parentWorldTM);
        void ComputeLocalTM();
        void ComputeWorldTM();
        //////////////////////////////////////////////////////////////////////////

        //! Returns whether external calls are currently allowed to move the transform.
        bool AreMoveRequestsAllowed() const;

        // TransformHierarchyInformationBus
        void GatherChildren(AZStd::vector<AZ::EntityId>& children) override;

        /// \ref ComponentDescriptor::GetProvidedServices
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        /// \ref ComponentDescriptor::Reflect
        static void Reflect(AZ::ReflectContext* reflection);

        AZ::TransformChangedEvent m_transformChangedEvent; ///< Event used to signal when a transform changes.
        AZ::ParentChangedEvent    m_parentChangedEvent;    ///< Event used to signal when a transforms parent changes.
        AZ::ChildChangedEvent     m_childChangedEvent;     ///< Event used to signal when a transform has a child entity added or removed.

        AZ::Transform m_localTM = AZ::Transform::CreateIdentity(); ///< Local transform relative to parent transform (same as worldTM if no parent).
        AZ::Transform m_worldTM = AZ::Transform::CreateIdentity(); ///< World transform including parent transform (same as localTM if no parent).

        AZ::EntityId m_parentId; ///< If valid, this transform is parented to m_parentId.
        AZ::TransformInterface* m_parentTM = nullptr; ///< Cached - pointer to parent transform, to avoid extra calls. Valid only when if it's present.
        AZ::TransformNotificationBus::BusPtr m_notificationBus; ///< Cached bus pointer to the notification bus.
        ParentActivationTransformMode m_parentActivationTransformMode = ParentActivationTransformMode::MaintainOriginalRelativeTransform;
        bool m_parentActive = false; ///< Keeps track of the state of the parent entity.
        bool m_onNewParentKeepWorldTM = true; ///< If set, recompute localTM instead of worldTM when parent becomes active.
        bool m_isStatic = false; ///< If true, the transform is static and doesn't move while entity is active.
    };
}   // namespace AZ
