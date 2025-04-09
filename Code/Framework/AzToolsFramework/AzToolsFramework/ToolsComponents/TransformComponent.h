/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Slice/SliceBus.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Commands/SelectionCommand.h>
#include <AzToolsFramework/ToolsComponents/EditorNonUniformScaleComponent.h>
#include <AzToolsFramework/FocusMode/FocusModeInterface.h>

#include "EditorComponentBase.h"
#include "TransformComponentBus.h"

namespace AzToolsFramework
{
    namespace Components
    {
        /// Manages transform data as separate vector fields for editing purposes.
        /// The TransformComponent is referenced by other components in the same entity, it is not an asset.
        class TransformComponent
            : public EditorComponentBase
            , public AZ::TransformBus::Handler
            , public AZ::SliceEntityHierarchyRequestBus::Handler
            , private TransformComponentMessages::Bus::Handler
            , private AZ::EntityBus::Handler
            , private AZ::TransformNotificationBus::MultiHandler
            , private AZ::TransformHierarchyInformationBus::Handler
        {
            friend class JsonTransformComponentSerializer;

        public:
            friend class TransformComponentFactory;

            AZ_EDITOR_COMPONENT(TransformComponent, AZ::EditorTransformComponentTypeId, EditorComponentBase, AZ::SliceEntityHierarchyInterface, AZ::TransformInterface)

            TransformComponent();
            virtual ~TransformComponent();

            // AZ::Component
            void Init() override;
            void Activate() override;
            void Deactivate() override;

            static void PasteOverComponent(const TransformComponent* sourceComponent, TransformComponent* destinationComponent);

            // AZ::EntityBus
            void OnEntityActivated(const AZ::EntityId& parentEntityId) override;
            void OnEntityDeactivated(const AZ::EntityId& parentEntityId) override;

            // AZ::TransformBus
            void BindTransformChangedEventHandler(AZ::TransformChangedEvent::Handler& handler) override;
            void BindParentChangedEventHandler(AZ::ParentChangedEvent::Handler& handler) override;
            void BindChildChangedEventHandler(AZ::ChildChangedEvent::Handler& handler) override;
            void NotifyChildChangedEvent(AZ::ChildChangeType changeType, AZ::EntityId entityId) override;
            const AZ::Transform& GetLocalTM() override;
            void SetLocalTM(const AZ::Transform& tm) override;
            const AZ::Transform& GetWorldTM() override;
            void SetWorldTM(const AZ::Transform& tm) override;
            void GetLocalAndWorld(AZ::Transform& localTM, AZ::Transform& worldTM) override;

            // Translation modifiers
            void SetWorldTranslation(const AZ::Vector3& newPosition) override;
            void SetLocalTranslation(const AZ::Vector3& newPosition) override;

            AZ::Vector3 GetWorldTranslation() override;
            AZ::Vector3 GetLocalTranslation() override;

            void MoveEntity(const AZ::Vector3& offset) override;

            void SetWorldX(float newX) override;
            void SetWorldY(float newY) override;
            void SetWorldZ(float newZ) override;

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

            AZ::EntityId  GetParentId() override;
            AZ::TransformInterface* GetParent() override;
            void SetParent(AZ::EntityId parentId) override;
            void SetParentRelative(AZ::EntityId parentId) override;
            AZStd::vector<AZ::EntityId> GetChildren() override;
            AZStd::vector<AZ::EntityId> GetAllDescendants() override;
            AZStd::vector<AZ::EntityId> GetEntityAndAllDescendants() override;
            bool IsStaticTransform() override;
            void SetIsStaticTransform(bool isStatic) override;

            // TransformComponentMessages::Bus
            void TranslateBy(const AZ::Vector3&) override;
            void RotateBy(const AZ::Vector3&) override; // euler in degrees
            const EditorTransform& GetLocalEditorTransform() override;
            void SetLocalEditorTransform(const EditorTransform& dest) override;
            bool IsTransformLocked() override;

            /// \return true if the entity is a root-level entity (has no transform parent).
            bool IsRootEntity() const { return !m_parentEntityId.IsValid(); }

            //callable is a lambda taking a AZ::EntityId and returning nothing, will be called with the id of each child
            //Also will be called for children of children, all the way down the hierarchy
            template<typename Callable>
            void ForEachChild(Callable callable)
            {
                for (auto childId : m_childrenEntityIds)
                {
                    callable(childId);
                    auto child = GetTransformComponent(childId);
                    if (child)
                    {
                        child->ForEachChild(callable);
                    }
                }
            }

            void BuildGameEntity(AZ::Entity* gameEntity) override;

            void UpdateCachedWorldTransform();
            void ClearCachedWorldTransform();

            // SliceEntityHierarchyRequestBus
            AZ::EntityId GetSliceEntityParentId() override;
            AZStd::vector<AZ::EntityId> GetSliceEntityChildren() override;

            // For tools
            // Returns false if the component did not exist and failed to be added.
            // Returns true otherwise, and always updates the non-uniform scale value.
            bool AddNonUniformScaleComponent(const AZ::Vector3& nonUniformScale);

        private:
            // AZ::TransformNotificationBus - Connected to parent's ID
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            void OnTransformChanged(); //convenience

            // TransformHierarchyInformationBus
            void GatherChildren(AZStd::vector<AZ::EntityId>& children) override;

            void AddContextMenuActions(QMenu* menu) override;

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void Reflect(AZ::ReflectContext* context);

            AZ::u32 TransformChangedInspector();
            AZ::u32 StaticChangedInspector();

            bool TransformChanged();

            AZ::Transform GetLocalTranslationTM() const;
            AZ::Transform GetLocalRotationTM() const;
            AZ::Transform GetLocalScaleTM() const;

            TransformComponent* GetParentTransformComponent() const;
            TransformComponent* GetTransformComponent(AZ::EntityId otherEntityId) const;
            bool IsEntityInHierarchy(AZ::EntityId entityId);

            void SetParentImpl(AZ::EntityId parentId, bool relative);
            const AZ::Transform& GetParentWorldTM() const;

            void ModifyEditorTransform(AZ::Vector3& vec, const AZ::Vector3& data, const AZ::Transform& parentInverse);

            void CheckApplyCachedWorldTransform(const AZ::Transform& parentWorld);

            AZ::Component* FindPresentOrPendingComponent(AZ::Uuid componentUuid);
            bool IsAddNonUniformScaleButtonReadOnly();
            AZ::Crc32 OnAddNonUniformScaleButtonPressed();

            // Drives transform behavior when parent activates. See AZ::TransformConfig::ParentActivationTransformMode for details.
            AZ::TransformConfig::ParentActivationTransformMode m_parentActivationTransformMode;

            AZ::TransformChangedEvent m_transformChangedEvent; ///< Event used to signal when a transform changes.
            AZ::ParentChangedEvent    m_parentChangedEvent;    ///< Event used to signal when a transforms parent changes.
            AZ::ChildChangedEvent     m_childChangedEvent;     ///< Event used to signal when a transform has a child entity added or removed.

            EditorTransform m_editorTransform;

            // These are only used to hold onto the references returned by GetLocalTM and GetWorldTM
            AZ::Transform m_localTransformCache;
            AZ::Transform m_worldTransformCache;

            // Keeping a world transform along with a parent Id at the time of capture.
            // This is required for dealing with external changes to parent assignment (i.e. slice propagation).
            // A local transform alone isn't enough, since we may have serialized a parent-relative local transform,
            // but detached from the parent via propagation of the parent Id field. In such a case, we need to
            // know to not erroneously apply the local-space transform we serialized in a world-space capacity.
            AZ::Transform m_cachedWorldTransform;
            AZ::EntityId m_cachedWorldTransformParent;

            AZ::EntityId m_parentEntityId;
            AZ::EntityId m_previousParentEntityId;

            EntityIdList m_childrenEntityIds;

            bool m_suppressTransformChangedEvent;

            bool m_localTransformDirty = true;
            bool m_worldTransformDirty = true;
            bool m_isStatic = false;

            // Used to check whether entity was just created vs manually reactivated. Set true after OnEntityActivated is called the first time.
            bool m_initialized = false;

            FocusModeInterface* m_focusModeInterface = nullptr;

            // Deprecated
            AZ::InterpolationMode m_interpolatePosition;
            AZ::InterpolationMode m_interpolateRotation;
            bool m_netSyncEnabled = false;
        };
    }
} // namespace AzToolsFramework
