/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TransformComponent.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Visibility/EntityBoundsUnionBus.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/Entity/EditorEntityTransformBus.h>
#include <AzToolsFramework/API/EntityPropertyEditorRequestsBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/ToolsComponents/TransformComponentBus.h>
#include <AzToolsFramework/ToolsComponents/TransformComponentSerializer.h>
#include <AzToolsFramework/ToolsComponents/EditorInspectorComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionBus.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

#include <QMessageBox>

#include <QMenu>

namespace AzToolsFramework
{
    namespace Components
    {
        namespace Internal
        {
            const AZ::u32 ParentEntityCRC = AZ_CRC("Parent Entity", 0x5b1b276c);

            // Decompose a transform into euler angles in degrees, uniform scale, and translation.
            void DecomposeTransform(const AZ::Transform& transform, AZ::Vector3& translation, AZ::Vector3& rotation, float& scale)
            {
                scale = transform.GetUniformScale();
                translation = transform.GetTranslation();
                rotation = transform.GetRotation().GetEulerDegrees();
            }

            bool TransformComponentDataConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
            {
                if (classElement.GetVersion() < 6)
                {
                    // In v6, "Slice Transform" became slice-relative.
                    const int sliceRelTransformIdx = classElement.FindElement(AZ_CRC("Slice Transform", 0x4f156fd1));
                    if (sliceRelTransformIdx >= 0)
                    {
                    // Convert slice-relative transform/root to standard parent-child relationship.
                    const int sliceRootIdx = classElement.FindElement(AZ_CRC("Slice Root", 0x9f115e1f));
                    const int parentIdx = classElement.FindElement(ParentEntityCRC);
                    const int editorTransformIdx = classElement.FindElement(AZ_CRC("Transform Data", 0xf0a2bb50));
                    const int cachedTransformIdx = classElement.FindElement(AZ_CRC("Cached World Transform", 0x571fab30));

                    if (editorTransformIdx >= 0 && sliceRootIdx >= 0 && parentIdx >= 0)
                    {
                        auto& sliceTransformElement = classElement.GetSubElement(sliceRelTransformIdx);
                        auto& sliceRootElement = classElement.GetSubElement(sliceRootIdx);
                        auto& parentElement = classElement.GetSubElement(parentIdx);
                        auto& editorTransformElement = classElement.GetSubElement(editorTransformIdx);

                        AZ::Transform sliceRelTransform;
                        if (sliceTransformElement.GetData(sliceRelTransform))
                        {
                            // If the entity already has a parent assigned, we don't need to fix anything up.
                            // We only need to convert slice root to parent for non-child entities.
                            const int parentIdValueIdx = parentElement.FindElement(AZ_CRC("id", 0xbf396750));
                            AZ::u64 parentId = 0;
                            if (parentIdValueIdx >= 0)
                            {
                                parentElement.GetSubElement(parentIdValueIdx).GetData(parentId);
                            }

                            AZ::EntityId sliceRootId;
                            const int entityIdValueIdx = sliceRootElement.FindElement(AZ_CRC("id", 0xbf396750));

                            if (entityIdValueIdx < 0)
                            {
                                return false;
                            }

                            if (parentId == static_cast<AZ::u64>(AZ::EntityId()) && sliceRootElement.GetSubElement(entityIdValueIdx).GetData(sliceRootId))
                            {
                                // Upgrading the data itself is only relevant when a slice root was actually defined.
                                if (sliceRootId.IsValid())
                                {
                                    // Cached transforms weren't nullified in really old slices.
                                    if (cachedTransformIdx >= 0)
                                    {
                                        auto& cachedTransformElement = classElement.GetSubElement(cachedTransformIdx);
                                        cachedTransformElement.Convert<AZ::Transform>(context);
                                        cachedTransformElement.SetData(context, AZ::Transform::Identity());
                                    }

                                    // Our old slice root Id is now our parent Id.
                                    // Note - this could be ourself, but we can't know yet, so it gets fixed up by EditorEntityFixupComponent.
                                    parentElement.Convert<AZ::EntityId>(context);
                                    parentElement.SetData(context, sliceRootId);

                                    // Decompose the old slice-relative transform and set it as a our editor transform,
                                    // since the entity is now our parent.
                                    EditorTransform editorTransform;
                                    DecomposeTransform(sliceRelTransform, editorTransform.m_translate, editorTransform.m_rotate, editorTransform.m_uniformScale);
                                    editorTransformElement.Convert<EditorTransform>(context);
                                    editorTransformElement.SetData(context, editorTransform);
                                }
                            }

                            // Finally, remove old fields.
                            classElement.RemoveElementByName(AZ_CRC("Slice Transform", 0x4f156fd1));
                            classElement.RemoveElementByName(AZ_CRC("Slice Root", 0x9f115e1f));
                            }
                        }
                        }
                    }

                if (classElement.GetVersion() < 7)
                {
                    // "IsStatic" added at v7.
                    // Old versions of TransformComponent are assumed to be non-static.
                    classElement.AddElementWithData(context, "IsStatic", false);
                }

                if (classElement.GetVersion() < 8)
                {
                    // "InterpolatePosition" added at v8.
                    // "InterpolateRotation" added at v8.
                    // Old versions of TransformComponent are assumed to be using linear interpolation.
                    classElement.AddElementWithData(context, "InterpolatePosition", AZ::InterpolationMode::NoInterpolation);
                    classElement.AddElementWithData(context, "InterpolateRotation", AZ::InterpolationMode::NoInterpolation);
                }

                // note the == on the following line.  Do not add to this block.  If you add an "InterpolateScale" back in, then
                // consider erasing this block.  The version was bumped from 8 to 9 to ensure this code runs.
                // if you add the field back in, then increment the version number again.
                if (classElement.GetVersion() == 8)
                {
                    // a field was temporarily added to this specific version, then was removed.
                    // However, some data may have been exported with this field present, so
                    // remove it if its found, but only in this version which the change was present in, so that
                    // future re-additions of it won't remove it (as long as they bump the version number.)
                    classElement.RemoveElementByName(AZ_CRC("InterpolateScale", 0x9d00b831));
                }

                if (classElement.GetVersion() < 10)
                {
                    // The "Sync Enabled" flag is no longer needed.
                    classElement.RemoveElementByName(AZ_CRC_CE("Sync Enabled"));
                }

                return true;
            }

            bool EditorTransformDataConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
            {
                if (classElement.GetVersion() < 3)
                {
                    // version 3 replaces vector scale with uniform scale but does not yet delete the legacy scale data
                    // in order to allow for migration
                    AZ::Vector3 vectorScale;
                    if (classElement.FindSubElementAndGetData<AZ::Vector3>(AZ_CRC_CE("Scale"), vectorScale))
                    {
                        const float uniformScale = vectorScale.GetMaxElement();
                        classElement.AddElementWithData(context, "UniformScale", uniformScale);
                    }
                }

                return true;
            }
        } // namespace Internal

        TransformComponent::TransformComponent()
            : m_isStatic(false)
            , m_parentActivationTransformMode(AZ::TransformConfig::ParentActivationTransformMode::MaintainOriginalRelativeTransform)
            , m_cachedWorldTransform(AZ::Transform::Identity())
            , m_suppressTransformChangedEvent(false)
            , m_interpolatePosition(AZ::InterpolationMode::NoInterpolation)
            , m_interpolateRotation(AZ::InterpolationMode::NoInterpolation)
        {
        }

        TransformComponent::~TransformComponent()
        {
        }

        void TransformComponent::Init()
        {
            //Ensure that when we init that our previous/parent match
            m_previousParentEntityId = m_parentEntityId;
        }

        void TransformComponent::Activate()
        {
            EditorComponentBase::Activate();

            TransformComponentMessages::Bus::Handler::BusConnect(GetEntityId());
            AZ::TransformBus::Handler::BusConnect(GetEntityId());
            AZ::SliceEntityHierarchyRequestBus::Handler::BusConnect(GetEntityId());

            // for drag + drop child entity from one parent to another, undo/redo
            if (m_parentEntityId.IsValid())
            {
                AZ::EntityBus::Handler::BusConnect(m_parentEntityId);

                if (m_parentEntityId != m_previousParentEntityId)
                {
                    ToolsApplicationNotificationBus::Broadcast(
                        &ToolsApplicationEvents::EntityParentChanged, GetEntityId(),
                        m_parentEntityId, m_previousParentEntityId);
                }

                m_previousParentEntityId = m_parentEntityId;
            }
            // it includes the process of create/delete entity
            else
            {
                CheckApplyCachedWorldTransform(AZ::Transform::Identity());
                UpdateCachedWorldTransform();
            }
        }

        void TransformComponent::Deactivate()
        {
            AZ::EntityBus::Handler::BusDisconnect();
            AZ::SliceEntityHierarchyRequestBus::Handler::BusDisconnect();
            AZ::TransformBus::Handler::BusDisconnect();
            TransformComponentMessages::Bus::Handler::BusDisconnect();
            AZ::TransformHierarchyInformationBus::Handler::BusDisconnect();
            AZ::TransformNotificationBus::MultiHandler::BusDisconnect();

            EditorComponentBase::Deactivate();
        }

        void TransformComponent::BindTransformChangedEventHandler(AZ::TransformChangedEvent::Handler& handler)
        {
            handler.Connect(m_transformChangedEvent);
        }

        void TransformComponent::BindParentChangedEventHandler(AZ::ParentChangedEvent::Handler& handler)
        {
            handler.Connect(m_parentChangedEvent);
        }

        void TransformComponent::BindChildChangedEventHandler(AZ::ChildChangedEvent::Handler& handler)
        {
            handler.Connect(m_childChangedEvent);
        }

        void TransformComponent::NotifyChildChangedEvent(AZ::ChildChangeType changeType, AZ::EntityId entityId)
        {
            m_childChangedEvent.Signal(changeType, entityId);
        }

        // This is called when our transform changes directly, or our parent's has changed.
        void TransformComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
        {
            m_localTransformDirty = true;
            m_worldTransformDirty = true;

            if (const AZ::Entity* entity = GetEntity())
            {
                SetDirty();

                // Update parent-relative transform.
                const auto& localTM = GetLocalTM();
                const auto worldTM = world * localTM;

                UpdateCachedWorldTransform();

                AZ::TransformNotificationBus::Event(
                    GetEntityId(), &TransformNotification::OnTransformChanged, localTM, worldTM);
                m_transformChangedEvent.Signal(localTM, worldTM);

                AzFramework::IEntityBoundsUnion* boundsUnion = AZ::Interface<AzFramework::IEntityBoundsUnion>::Get();
                if (boundsUnion != nullptr)
                {
                    boundsUnion->OnTransformUpdated(GetEntity());
                }
                // Fire a property changed notification for this component
                if (const AZ::Component* component = entity->FindComponent<Components::TransformComponent>())
                {
                    PropertyEditorEntityChangeNotificationBus::Event(
                        GetEntityId(), &PropertyEditorEntityChangeNotifications::OnEntityComponentPropertyChanged, component->GetId());
                }

                // Refresh the property editor if we're selected
                bool selected = false;
                ToolsApplicationRequestBus::BroadcastResult(
                    selected, &AzToolsFramework::ToolsApplicationRequests::IsSelected, GetEntityId());
                if (selected)
                {
                    ToolsApplicationEvents::Bus::Broadcast(
                        &ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_Values);
                }
            }
        }

        void TransformComponent::OnTransformChanged()
        {
            if (!m_suppressTransformChangedEvent)
            {
                auto parent = GetParentTransformComponent();
                if (parent)
                {
                    OnTransformChanged(parent->GetLocalTM(), parent->GetWorldTM());
                }
                else
                {
                    OnTransformChanged(AZ::Transform::Identity(), AZ::Transform::Identity());
                }
            }
        }

        void TransformComponent::UpdateCachedWorldTransform()
        {
            const AZ::Transform& worldTransform = GetWorldTM();
            if (m_cachedWorldTransformParent != m_parentEntityId || !worldTransform.IsClose(m_cachedWorldTransform))
            {
                m_cachedWorldTransformParent = GetParentId();
                m_cachedWorldTransform = GetWorldTM();
                if (GetEntity())
                {
                    SetDirty();
                }
            }
        }

        void TransformComponent::ClearCachedWorldTransform()
        {
            m_cachedWorldTransform = AZ::Transform::Identity();
            m_cachedWorldTransformParent = AZ::EntityId();
        }

        void TransformComponent::CheckApplyCachedWorldTransform(const AZ::Transform& parentWorld)
        {
            if (m_parentEntityId != m_cachedWorldTransformParent)
            {
                if (!m_cachedWorldTransform.IsClose(AZ::Transform::Identity()))
                {
                    SetLocalTM(parentWorld.GetInverse() * m_cachedWorldTransform);
                }
            }
        }

        AZ::Transform TransformComponent::GetLocalTranslationTM() const
        {
            return AZ::Transform::CreateTranslation(m_editorTransform.m_translate);
        }

        AZ::Transform TransformComponent::GetLocalRotationTM() const
        {
            return AZ::ConvertEulerDegreesToTransform(m_editorTransform.m_rotate);
        }

        AZ::Transform TransformComponent::GetLocalScaleTM() const
        {
            return AZ::Transform::CreateUniformScale(m_editorTransform.m_uniformScale);
        }

        const AZ::Transform& TransformComponent::GetLocalTM()
        {
            if (m_localTransformDirty)
            {
                m_localTransformCache = GetLocalTranslationTM() * GetLocalRotationTM() * GetLocalScaleTM();
                m_localTransformDirty = false;
            }

            return m_localTransformCache;
        }

        // given a local transform, update local transform.
        void TransformComponent::SetLocalTM(const AZ::Transform& finalTx)
        {
            AZ::Vector3 tx, rot;
            float uniformScale;
            Internal::DecomposeTransform(finalTx, tx, rot, uniformScale);

            m_editorTransform.m_translate = tx;
            m_editorTransform.m_rotate = rot;
            m_editorTransform.m_uniformScale = uniformScale;

            TransformChanged();
        }

        const EditorTransform& TransformComponent::GetLocalEditorTransform()
        {
            return m_editorTransform;
        }

        void TransformComponent::SetLocalEditorTransform(const EditorTransform& dest)
        {
            m_editorTransform = dest;

            TransformChanged();
        }

        bool TransformComponent::IsTransformLocked()
        {
            return m_editorTransform.m_locked;
        }

        const AZ::Transform& TransformComponent::GetWorldTM()
        {
            if (m_worldTransformDirty)
            {
                m_worldTransformCache = GetParentWorldTM() * GetLocalTM();
                m_worldTransformDirty = false;
            }

            return m_worldTransformCache;
        }

        void TransformComponent::SetWorldTM(const AZ::Transform& finalTx)
        {
            AZ::Transform parentGlobalToWorldInverse = GetParentWorldTM().GetInverse();
            SetLocalTM(parentGlobalToWorldInverse * finalTx);
        }

        void TransformComponent::GetLocalAndWorld(AZ::Transform& localTM, AZ::Transform& worldTM)
        {
            localTM = GetLocalTM();
            worldTM = GetWorldTM();
        }

        void TransformComponent::SetWorldTranslation(const AZ::Vector3& newPosition)
        {
            AZ::Transform newWorldTransform = GetWorldTM();
            newWorldTransform.SetTranslation(newPosition);
            SetWorldTM(newWorldTransform);
        }

        void TransformComponent::SetLocalTranslation(const AZ::Vector3& newPosition)
        {
            AZ::Transform newLocalTransform = GetLocalTM();
            newLocalTransform.SetTranslation(newPosition);
            SetLocalTM(newLocalTransform);
        }

        AZ::Vector3 TransformComponent::GetWorldTranslation()
        {
            return GetWorldTM().GetTranslation();
        }

        AZ::Vector3 TransformComponent::GetLocalTranslation()
        {
            return GetLocalTM().GetTranslation();
        }

        void TransformComponent::MoveEntity(const AZ::Vector3& offset)
        {
            const AZ::Vector3& worldPosition = GetWorldTM().GetTranslation();
            SetWorldTranslation(worldPosition + offset);
        }

        void TransformComponent::SetWorldX(float newX)
        {
            const AZ::Vector3& worldPosition = GetWorldTM().GetTranslation();
            SetWorldTranslation(AZ::Vector3(newX, worldPosition.GetY(), worldPosition.GetZ()));
        }

        void TransformComponent::SetWorldY(float newY)
        {
            const AZ::Vector3& worldPosition = GetWorldTM().GetTranslation();
            SetWorldTranslation(AZ::Vector3(worldPosition.GetX(), newY, worldPosition.GetZ()));
        }

        void TransformComponent::SetWorldZ(float newZ)
        {
            const AZ::Vector3& worldPosition = GetWorldTM().GetTranslation();
            SetWorldTranslation(AZ::Vector3(worldPosition.GetX(), worldPosition.GetY(), newZ));
        }

        float TransformComponent::GetWorldX()
        {
            return GetWorldTranslation().GetX();
        }

        float TransformComponent::GetWorldY()
        {
            return GetWorldTranslation().GetY();
        }

        float TransformComponent::GetWorldZ()
        {
            return GetWorldTranslation().GetZ();
        }

        void TransformComponent::SetLocalX(float x)
        {
            m_editorTransform.m_translate.SetX(x);
            TransformChanged();
        }

        void TransformComponent::SetLocalY(float y)
        {
            m_editorTransform.m_translate.SetY(y);
            TransformChanged();
        }

        void TransformComponent::SetLocalZ(float z)
        {
            m_editorTransform.m_translate.SetZ(z);
            TransformChanged();
        }

        float TransformComponent::GetLocalX()
        {
            return m_editorTransform.m_translate.GetX();
        }

        float TransformComponent::GetLocalY()
        {
            return m_editorTransform.m_translate.GetY();
        }

        float TransformComponent::GetLocalZ()
        {
            return m_editorTransform.m_translate.GetZ();
        }

        void TransformComponent::SetWorldRotationQuaternion(const AZ::Quaternion& quaternion)
        {
            AZ::Transform newWorldTransform = GetWorldTM();
            newWorldTransform.SetRotation(quaternion);
            SetWorldTM(newWorldTransform);
        }

        AZ::Vector3 TransformComponent::GetWorldRotation()
        {
            return GetWorldTM().GetRotation().GetEulerRadians();
        }

        AZ::Quaternion TransformComponent::GetWorldRotationQuaternion()
        {
            return GetWorldTM().GetRotation();
        }

        void TransformComponent::SetLocalRotation(const AZ::Vector3& eulerAnglesRadian)
        {
            m_editorTransform.m_rotate = AZ::Vector3RadToDeg(eulerAnglesRadian);
            TransformChanged();
        }

        void TransformComponent::SetLocalRotationQuaternion(const AZ::Quaternion& quaternion)
        {
            m_editorTransform.m_rotate = quaternion.GetEulerDegrees();
            TransformChanged();
        }

        void TransformComponent::RotateAroundLocalX(float eulerAngleRadian)
        {
            AZ::Transform localRotate = AZ::ConvertEulerDegreesToTransform(m_editorTransform.m_rotate);
            AZ::Vector3 xAxis = localRotate.GetBasisX();
            AZ::Quaternion xRotate = AZ::Quaternion::CreateFromAxisAngle(xAxis, eulerAngleRadian);
            AZ::Quaternion currentRotate = AZ::ConvertEulerDegreesToQuaternion(m_editorTransform.m_rotate);
            AZ::Quaternion newRotate = xRotate * currentRotate;
            newRotate.Normalize();
            m_editorTransform.m_rotate = newRotate.GetEulerDegrees();
            TransformChanged();
        }

        void TransformComponent::RotateAroundLocalY(float eulerAngleRadian)
        {
            AZ::Transform localRotate = AZ::ConvertEulerDegreesToTransform(m_editorTransform.m_rotate);
            AZ::Vector3 yAxis = localRotate.GetBasisY();
            AZ::Quaternion yRotate = AZ::Quaternion::CreateFromAxisAngle(yAxis, eulerAngleRadian);
            AZ::Quaternion currentRotate = AZ::ConvertEulerDegreesToQuaternion(m_editorTransform.m_rotate);
            AZ::Quaternion newRotate = yRotate * currentRotate;
            newRotate.Normalize();
            m_editorTransform.m_rotate = newRotate.GetEulerDegrees();

            TransformChanged();
        }

        void TransformComponent::RotateAroundLocalZ(float eulerAngleRadian)
        {
            AZ::Transform localRotate = AZ::ConvertEulerDegreesToTransform(m_editorTransform.m_rotate);
            AZ::Vector3 zAxis = localRotate.GetBasisZ();
            AZ::Quaternion zRotate = AZ::Quaternion::CreateFromAxisAngle(zAxis, eulerAngleRadian);
            AZ::Quaternion currentRotate = AZ::ConvertEulerDegreesToQuaternion(m_editorTransform.m_rotate);
            AZ::Quaternion newRotate = zRotate * currentRotate;
            newRotate.Normalize();
            m_editorTransform.m_rotate = newRotate.GetEulerDegrees();

            TransformChanged();
        }

        AZ::Vector3 TransformComponent::GetLocalRotation()
        {
            AZ::Vector3 result = AZ::Vector3DegToRad(m_editorTransform.m_rotate);
            return result;
        }

        AZ::Quaternion TransformComponent::GetLocalRotationQuaternion()
        {
            AZ::Quaternion result = AZ::ConvertEulerDegreesToQuaternion(m_editorTransform.m_rotate);
            return result;
        }

        AZ::Vector3 TransformComponent::GetLocalScale()
        {
            AZ_WarningOnce("TransformComponent", false, "GetLocalScale is deprecated, please use GetLocalUniformScale instead");
            return m_editorTransform.m_legacyScale;
        }

        void TransformComponent::SetLocalUniformScale(float scale)
        {
            m_editorTransform.m_uniformScale = scale;
            TransformChanged();
        }

        float TransformComponent::GetLocalUniformScale()
        {
            return m_editorTransform.m_uniformScale;
        }

        float TransformComponent::GetWorldUniformScale()
        {
            return GetWorldTM().GetUniformScale();
        }

        const AZ::Transform& TransformComponent::GetParentWorldTM() const
        {
            auto parent = GetParentTransformComponent();
            if (parent)
            {
                return parent->GetWorldTM();
            }
            return AZ::Transform::Identity();
        }

        void TransformComponent::SetParentImpl(AZ::EntityId parentId, bool relative)
        {
            // If the parent id to be set is the same as the current parent id
            // Or if the component belongs to an entity and the entity's id is the same as the id being set as parent
            if (parentId == m_parentEntityId || (GetEntity() && (GetEntityId() == parentId)))
            {
                return;
            }

            // Entity is not associated if we're just doing data preparation (slice construction).
            if (!GetEntity() || GetEntity()->GetState() == AZ::Entity::State::Constructed)
            {
                m_previousParentEntityId = m_parentEntityId = parentId;
                return;
            }


            // Prevent this from parenting to its own child. Check if this entity is in the new parent's hierarchy.
            auto potentialParentTransformComponent = GetTransformComponent(parentId);
            if (potentialParentTransformComponent && potentialParentTransformComponent->IsEntityInHierarchy(GetEntityId()))
            {
                return;
            }

            auto oldParentId = m_parentEntityId;

            bool canChangeParent = true;
            AZ::TransformNotificationBus::Event(
                GetEntityId(),
                &AZ::TransformNotificationBus::Events::CanParentChange,
                canChangeParent,
                oldParentId,
                parentId);

            if (!canChangeParent)
            {
                return;
            }

            // SetLocalTM calls below can confuse listeners, because transforms are mathematically
            // detached before the ParentChanged events are dispatched. Suppress TransformChanged()
            // until the transaction is complete.
            m_suppressTransformChangedEvent = true;

            if (m_parentEntityId.IsValid())
            {
                AZ::TransformHierarchyInformationBus::Handler::BusDisconnect();
                AZ::TransformNotificationBus::MultiHandler::BusDisconnect(m_parentEntityId);
                AZ::EntityBus::Handler::BusDisconnect(m_parentEntityId);

                if (!relative)
                {
                    SetLocalTM(GetParentWorldTM() * GetLocalTM());
                }

                TransformComponent* parentTransform = GetParentTransformComponent();
                if (parentTransform)
                {
                    auto& parentChildIds = GetParentTransformComponent()->m_childrenEntityIds;
                    parentChildIds.erase(AZStd::remove(parentChildIds.begin(), parentChildIds.end(), GetEntityId()), parentChildIds.end());
                }

                m_parentEntityId.SetInvalid();
                m_previousParentEntityId = m_parentEntityId;
            }

            if (parentId.IsValid())
            {
                AZ::TransformNotificationBus::MultiHandler::BusConnect(parentId);
                AZ::TransformHierarchyInformationBus::Handler::BusConnect(parentId);

                m_parentEntityId = parentId;
                m_previousParentEntityId = m_parentEntityId;

                if (!relative)
                {
                    AZ::Transform parentXform = GetParentWorldTM();
                    AZ::Transform inverseXform = parentXform.GetInverse();
                    SetLocalTM(inverseXform * GetLocalTM());
                }

                // OnEntityActivated will trigger immediately if the parent is active
                AZ::EntityBus::Handler::BusConnect(m_parentEntityId);
            }

            m_suppressTransformChangedEvent = false;

            // This is for Create Entity as child / Drag+drop parent update / add component
            EBUS_EVENT(AzToolsFramework::ToolsApplicationEvents::Bus, EntityParentChanged, GetEntityId(), parentId, oldParentId);
            EBUS_EVENT_ID(GetEntityId(), AZ::TransformNotificationBus, OnParentChanged, oldParentId, parentId);
            m_parentChangedEvent.Signal(oldParentId, parentId);

            TransformChanged();
        }

        void TransformComponent::SetParent(AZ::EntityId parentId)
        {
            SetParentImpl(parentId, false);
        }

        void TransformComponent::SetParentRelative(AZ::EntityId parentId)
        {
            SetParentImpl(parentId, true);
        }

        AZ::EntityId TransformComponent::GetParentId()
        {
            return m_parentEntityId;
        }

        EntityIdList TransformComponent::GetChildren()
        {
            EntityIdList children;
            AZ::TransformHierarchyInformationBus::Event(GetEntityId(), &AZ::TransformHierarchyInformation::GatherChildren, children);
            return children;
        }

        EntityIdList TransformComponent::GetAllDescendants()
        {
            EntityIdList descendants = GetChildren();
            for (size_t i = 0; i < descendants.size(); ++i)
            {
                AZ::TransformHierarchyInformationBus::Event(descendants[i], &AZ::TransformHierarchyInformation::GatherChildren, descendants);
            }
            return descendants;
        }

        AZStd::vector<AZ::EntityId> TransformComponent::GetEntityAndAllDescendants()
        {
            AZStd::vector<AZ::EntityId> descendants = { GetEntityId() };
            for (size_t i = 0; i < descendants.size(); ++i)
            {
                AZ::TransformHierarchyInformationBus::Event(descendants[i], &AZ::TransformHierarchyInformation::GatherChildren, descendants);
            }
            return descendants;
        }

        void TransformComponent::GatherChildren(EntityIdList& children)
        {
            children.push_back(GetEntityId());
        }
        
        bool TransformComponent::IsStaticTransform()
        {
            return m_isStatic;
        }

        void TransformComponent::SetIsStaticTransform(bool isStatic)
        {
            m_isStatic = isStatic;
        }

        TransformComponent* TransformComponent::GetParentTransformComponent() const
        {
            TransformComponent* component = GetTransformComponent(m_parentEntityId);

            // if our parent was cleared but we haven't gotten our change notify yet, we need to walk the previous parent if possible
            // to get the proper component for any necessary cleanup
            if (!component && !m_parentEntityId.IsValid() && m_previousParentEntityId.IsValid())
            {
                component = GetTransformComponent(m_previousParentEntityId);
            }

            return component;
        }

        TransformComponent* TransformComponent::GetTransformComponent(AZ::EntityId otherEntityId) const
        {
            if (!otherEntityId.IsValid())
            {
                return nullptr;
            }
            
            AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(otherEntityId);
            if (!entity)
            {
                return nullptr;
            }

            return entity->FindComponent<TransformComponent>();
        }

        AZ::TransformInterface* TransformComponent::GetParent()
        {
            return GetParentTransformComponent();
        }

        void TransformComponent::OnEntityActivated(const AZ::EntityId& parentEntityId)
        {
            AZ::TransformNotificationBus::MultiHandler::BusConnect(parentEntityId);
            AZ::TransformHierarchyInformationBus::Handler::BusConnect(parentEntityId);

            // Our parent entity has just been activated.
            AZ_Assert(parentEntityId == m_parentEntityId,
                "Received Activation message for an entity other than our parent.");

            TransformComponent* parentTransform = GetParentTransformComponent();
            if (parentTransform)
            {
                // Prevent circular parent/child relationships potentially generated through slice data hierarchies.
                // This doesn't only occur through direct user assignment of parent (which is handled separately),
                // but can also occur through cascading of slicces, so we need to validate on activation as well.
                if (GetEntity() && parentTransform->IsEntityInHierarchy(GetEntityId()))
                {
                    AZ_Error("Transform Component", false,
                             "Slice data propagation for Entity %s [%llu] has resulted in circular parent/child relationships. "
                             "Parent assignment for this entity has been reset.",
                             GetEntity()->GetName().c_str(),
                             GetEntityId());

                    SetParent(AZ::EntityId());
                    return;
                }

                bool suppressTransformChangedEvent = m_suppressTransformChangedEvent;
                // temporarily disable calling OnTransformChanged, because CheckApplyCachedWorldTransform is not guaranteed
                // to call it when m_cachedWorldTransform is identity. We send it manually later.
                m_suppressTransformChangedEvent = false;
                // When parent comes online, compute local TM from world TM.
                CheckApplyCachedWorldTransform(parentTransform->GetWorldTM());
                if (!m_initialized)
                {
                    m_initialized = true;
                    // If this is the first time this entity is being activated, manually compute OnTransformChanged
                    // this can occur when either the entity first created or undo/redo command is performed
                    OnTransformChanged(AZ::Transform::Identity(), parentTransform->GetWorldTM());
                }
                m_suppressTransformChangedEvent = suppressTransformChangedEvent;

                auto& parentChildIds = GetParentTransformComponent()->m_childrenEntityIds;
                if (parentChildIds.end() == AZStd::find(parentChildIds.begin(), parentChildIds.end(), GetEntityId()))
                {
                    parentChildIds.push_back(GetEntityId());
                }
            }

            UpdateCachedWorldTransform();
        }

        void TransformComponent::OnEntityDeactivated(const AZ::EntityId& parentEntityId)
        {
            AZ_Assert(parentEntityId == m_parentEntityId,
                "Received Deactivation message for an entity other than our parent.");

            AZ::TransformNotificationBus::MultiHandler::BusDisconnect(parentEntityId);
        }

        bool TransformComponent::IsEntityInHierarchy(AZ::EntityId entityId)
        {
            /// Begin 1.7 Release hack - #TODO - LMBR-37330
            if (GetParentId() == GetEntityId())
            {
                m_parentEntityId = m_previousParentEntityId;
            }
            /// End 1.7 Release hack

            auto parentTComp = this;
            while (parentTComp)
            {
                if (parentTComp->GetEntityId() == entityId)
                {
                    return true;
                }

                parentTComp = parentTComp->GetParentTransformComponent();
            }

            return false;
        }

        AZ::Outcome<void, AZStd::string> TransformComponent::ValidatePotentialParent(void* newValue, const AZ::Uuid& valueType)
        {
            if (azrtti_typeid<AZ::EntityId>() != valueType)
            {
                AZ_Assert(false, "Unexpected value type");
                return AZ::Failure(AZStd::string("Trying to set an entity ID to something that isn't an entity ID!"));
            }

            AZ::EntityId actualValue = static_cast<AZ::EntityId>(*((AZ::EntityId*)newValue));

            // Prevent setting the parent to the entity itself.
            if (actualValue == GetEntityId())
            {
                return AZ::Failure(AZStd::string("You cannot set an entity's parent to itself!"));
            }
            else
            {
                // Don't allow the change if it will result in a cycle hierarchy
                auto potentialParentTransformComponent = GetTransformComponent(actualValue);
                if (potentialParentTransformComponent && potentialParentTransformComponent->IsEntityInHierarchy(GetEntityId()))
                {
                    return AZ::Failure(AZStd::string("You cannot set an entity to be a child of one of its own children!"));
                }
            }

            return AZ::Success();
        }

        AZ::u32 TransformComponent::ParentChangedInspector()
        {
            AZ::u32 refreshLevel = AZ::Edit::PropertyRefreshLevels::None;

            if (!m_parentEntityId.IsValid())
            {
                // If Prefabs are enabled, reroute the invalid id to the level root
                bool isPrefabSystemEnabled = false;
                AzFramework::ApplicationRequests::Bus::BroadcastResult(
                    isPrefabSystemEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);

                if (isPrefabSystemEnabled)
                {
                    auto prefabPublicInterface = AZ::Interface<Prefab::PrefabPublicInterface>::Get();

                    if (prefabPublicInterface)
                    {
                        m_parentEntityId = prefabPublicInterface->GetLevelInstanceContainerEntityId();
                        refreshLevel = AZ::Edit::PropertyRefreshLevels::ValuesOnly;
                    }
                }
            }

            auto parentId = m_parentEntityId;
            m_parentEntityId = m_previousParentEntityId;
            SetParent(parentId);

            return refreshLevel;
        }

        AZ::u32 TransformComponent::TransformChangedInspector()
        {
            if (TransformChanged())
            {
                AzToolsFramework::EditorTransformChangeNotificationBus::Broadcast(
                    &AzToolsFramework::EditorTransformChangeNotificationBus::Events::OnEntityTransformChanged,
                    EntityIdList{ GetEntityId() });
            }

            return AZ::Edit::PropertyRefreshLevels::None;
        }

        bool TransformComponent::TransformChanged()
        {
            if (!m_suppressTransformChangedEvent)
            {
                if (auto parent = GetParentTransformComponent())
                {
                    OnTransformChanged(parent->GetLocalTM(), parent->GetWorldTM());
                }
                else
                {
                    OnTransformChanged(AZ::Transform::Identity(), AZ::Transform::Identity());
                }

                return true;
            }

            return false;
        }

        // This is called when our transform changes static state.
        AZ::u32 TransformComponent::StaticChangedInspector()
        {
            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::Bus::Events::InvalidatePropertyDisplay,
                AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree);
           
            if (GetEntity())
            {
                SetDirty();
                AZ::TransformNotificationBus::Event(GetEntityId(), &AZ::TransformNotificationBus::Events::OnStaticChanged, m_isStatic);
            }

            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        void TransformComponent::ModifyEditorTransform(AZ::Vector3& vec, const AZ::Vector3& data, const AZ::Transform& parentInverse)
        {
            if (data.IsZero())
            {
                return;
            }

            auto delta = parentInverse.TransformPoint(data);

            vec += delta;

            TransformChanged();
        }

        void TransformComponent::TranslateBy(const AZ::Vector3& data)
        {
            auto parent = GetParentWorldTM();
            parent.SetTranslation(AZ::Vector3::CreateZero());
            parent.Invert();

            ModifyEditorTransform(m_editorTransform.m_translate, data, parent);
        }

        void TransformComponent::RotateBy(const AZ::Vector3& data)
        {
            auto parent = GetParentWorldTM();
            parent.SetTranslation(AZ::Vector3::CreateZero());
            parent.Invert();

            ModifyEditorTransform(m_editorTransform.m_rotate, data, parent);
        }

        AZ::EntityId TransformComponent::GetSliceEntityParentId()
        {
            return GetParentId();
        }

        EntityIdList TransformComponent::GetSliceEntityChildren()
        {
            return GetChildren();
        }

        void TransformComponent::BuildGameEntity(AZ::Entity* gameEntity)
        {
            AZ::TransformConfig configuration;
            configuration.m_parentId = m_parentEntityId;
            configuration.m_worldTransform = GetWorldTM();
            configuration.m_localTransform = GetLocalTM();
            configuration.m_parentActivationTransformMode = m_parentActivationTransformMode;
            configuration.m_isStatic = m_isStatic;
            configuration.m_interpolatePosition = m_interpolatePosition;
            configuration.m_interpolateRotation = m_interpolateRotation;

            gameEntity->CreateComponent<AzFramework::TransformComponent>()->SetConfiguration(configuration);
        }

        void TransformComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        void TransformComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        void TransformComponent::PasteOverComponent(const TransformComponent* sourceComponent, TransformComponent* destinationComponent)
        {
            // When pasting from another transform component, just grab the world transform as that's the part the user is intuitively expecting to bring over
            destinationComponent->SetWorldTM(const_cast<TransformComponent*>(sourceComponent)->GetWorldTM());
        }

        AZ::Component* TransformComponent::FindPresentOrPendingComponent(AZ::Uuid componentUuid)
        {
            // first check if the component is present and valid
            if (AZ::Component* foundComponent = GetEntity()->FindComponent(componentUuid))
            {
                return foundComponent;
            }

            // then check to see if there's a component pending because it's in an invalid state
            AZStd::vector<AZ::Component*> pendingComponents;
            AzToolsFramework::EditorPendingCompositionRequestBus::Event(GetEntityId(),
                &AzToolsFramework::EditorPendingCompositionRequests::GetPendingComponents, pendingComponents);

            for (const auto pendingComponent : pendingComponents)
            {
                if (pendingComponent->RTTI_IsTypeOf(componentUuid))
                {
                    return pendingComponent;
                }
            }

            return nullptr;
        }

        bool TransformComponent::IsAddNonUniformScaleButtonReadOnly()
        {
            return FindPresentOrPendingComponent(EditorNonUniformScaleComponent::TYPEINFO_Uuid()) != nullptr;
        }

        AZ::Crc32 TransformComponent::OnAddNonUniformScaleButtonPressed()
        {
            // if there is already a non-uniform scale component, do nothing
            if (FindPresentOrPendingComponent(EditorNonUniformScaleComponent::TYPEINFO_Uuid()))
            {
                return AZ::Edit::PropertyRefreshLevels::None;
            }

            const AZStd::vector<AZ::EntityId> entityList = { GetEntityId() };
            const AZ::ComponentTypeList componentsToAdd = { EditorNonUniformScaleComponent::TYPEINFO_Uuid() };

            AzToolsFramework::EntityCompositionRequests::AddComponentsOutcome addComponentsOutcome;
            AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(addComponentsOutcome,
                &AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities, entityList, componentsToAdd);

            const auto nonUniformScaleComponent = FindPresentOrPendingComponent(EditorNonUniformScaleComponent::RTTI_Type());
            AZ::ComponentId nonUniformScaleComponentId =
                nonUniformScaleComponent ? nonUniformScaleComponent->GetId() : AZ::InvalidComponentId;

            if (!addComponentsOutcome.IsSuccess() || !nonUniformScaleComponent)
            {
                AZ_Warning("Transform component", false, "Failed to add non-uniform scale component.");
                return AZ::Edit::PropertyRefreshLevels::None;
            }

            AzToolsFramework::EntityPropertyEditorRequestBus::Broadcast(
                &AzToolsFramework::EntityPropertyEditorRequests::SetNewComponentId, nonUniformScaleComponentId);

            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        void TransformComponent::Reflect(AZ::ReflectContext* context)
        {
            // reflect data for script, serialization, editing..
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorTransform>()->
                    Field("Translate", &EditorTransform::m_translate)->
                    Field("Rotate", &EditorTransform::m_rotate)->
                    Field("Scale", &EditorTransform::m_legacyScale)->
                    Field("Locked", &EditorTransform::m_locked)->
                    Field("UniformScale", &EditorTransform::m_uniformScale)->
                    Version(3, &Internal::EditorTransformDataConverter);

                serializeContext->Class<Components::TransformComponent, EditorComponentBase>()->
                    Field("Parent Entity", &TransformComponent::m_parentEntityId)->
                    Field("Transform Data", &TransformComponent::m_editorTransform)->
                    Field("AddNonUniformScaleButton", &TransformComponent::m_addNonUniformScaleButton)->
                    Field("Cached World Transform", &TransformComponent::m_cachedWorldTransform)->
                    Field("Cached World Transform Parent", &TransformComponent::m_cachedWorldTransformParent)->
                    Field("Parent Activation Transform Mode", &TransformComponent::m_parentActivationTransformMode)->
                    Field("IsStatic", &TransformComponent::m_isStatic)->
                    Field("InterpolatePosition", &TransformComponent::m_interpolatePosition)->
                    Field("InterpolateRotation", &TransformComponent::m_interpolateRotation)->
                    Version(10, &Internal::TransformComponentDataConverter);

                if (AZ::EditContext* ptrEdit = serializeContext->GetEditContext())
                {
                    ptrEdit->Class<TransformComponent>("Transform", "Controls the placement of the entity in the world in 3d")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                            Attribute(AZ::Edit::Attributes::FixedComponentListIndex, 0)->
                            Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Transform.svg")->
                            Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Transform.svg")->
                            Attribute(AZ::Edit::Attributes::AutoExpand, true)->
                        DataElement(AZ::Edit::UIHandlers::Default, &TransformComponent::m_parentEntityId, "Parent entity", "")->
                            Attribute(AZ::Edit::Attributes::ChangeValidate, &TransformComponent::ValidatePotentialParent)->
                            Attribute(AZ::Edit::Attributes::ChangeNotify, &TransformComponent::ParentChangedInspector)->
                            Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::DontGatherReference | AZ::Edit::SliceFlags::NotPushableOnSliceRoot)->
                        DataElement(AZ::Edit::UIHandlers::Default, &TransformComponent::m_editorTransform, "Values", "")->
                            Attribute(AZ::Edit::Attributes::ChangeNotify, &TransformComponent::TransformChangedInspector)->
                            Attribute(AZ::Edit::Attributes::AutoExpand, true)->
                        DataElement(AZ::Edit::UIHandlers::Button, &TransformComponent::m_addNonUniformScaleButton, "", "")->
                            Attribute(AZ::Edit::Attributes::ButtonText, "Add non-uniform scale")->
                            Attribute(AZ::Edit::Attributes::ReadOnly, &TransformComponent::IsAddNonUniformScaleButtonReadOnly)->
                            Attribute(AZ::Edit::Attributes::ChangeNotify, &TransformComponent::OnAddNonUniformScaleButtonPressed)->
                        DataElement(AZ::Edit::UIHandlers::ComboBox, &TransformComponent::m_parentActivationTransformMode,
                            "Parent activation", "Configures relative transform behavior when parent activates.")->
                            EnumAttribute(AZ::TransformConfig::ParentActivationTransformMode::MaintainOriginalRelativeTransform, "Original relative transform")->
                            EnumAttribute(AZ::TransformConfig::ParentActivationTransformMode::MaintainCurrentWorldTransform, "Current world transform")->
                        DataElement(AZ::Edit::UIHandlers::Default, &TransformComponent::m_isStatic ,"Static", "Static entities are highly optimized and cannot be moved during runtime.")->
                            Attribute(AZ::Edit::Attributes::ChangeNotify, &TransformComponent::StaticChangedInspector)->
                        DataElement(AZ::Edit::UIHandlers::Default, &TransformComponent::m_cachedWorldTransformParent, "Cached Parent Entity", "")->
                            Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::DontGatherReference | AZ::Edit::SliceFlags::NotPushable)->
                            Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)->
                        DataElement(AZ::Edit::UIHandlers::Default, &TransformComponent::m_cachedWorldTransform, "Cached World Transform", "")->
                            Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::NotPushable)->
                            Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide);

                    ptrEdit->Class<EditorTransform>("Values", "XYZ PYR")->
                        DataElement(AZ::Edit::UIHandlers::Default, &EditorTransform::m_translate, "Translate", "Local Position (Relative to parent) in meters.")->
                            Attribute(AZ::Edit::Attributes::Step, 0.1f)->
                            Attribute(AZ::Edit::Attributes::Suffix, " m")->
                            Attribute(AZ::Edit::Attributes::Min, -AZ::Constants::MaxFloatBeforePrecisionLoss)->
                            Attribute(AZ::Edit::Attributes::Max, AZ::Constants::MaxFloatBeforePrecisionLoss)->
                            Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::NotPushableOnSliceRoot)->
                            Attribute(AZ::Edit::Attributes::ReadOnly, &EditorTransform::m_locked)->
                        DataElement(AZ::Edit::UIHandlers::Default, &EditorTransform::m_rotate, "Rotate", "Local Rotation (Relative to parent) in degrees.")->
                            Attribute(AZ::Edit::Attributes::Step, 1.0f)->
                            Attribute(AZ::Edit::Attributes::Suffix, " deg")->
                            Attribute(AZ::Edit::Attributes::ReadOnly, &EditorTransform::m_locked)->
                            Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::NotPushableOnSliceRoot)->
                        DataElement(AZ::Edit::UIHandlers::Default, &EditorTransform::m_uniformScale, "Uniform Scale", "Local Uniform Scale")->
                            Attribute(AZ::Edit::Attributes::Step, 0.1f)->
                            Attribute(AZ::Edit::Attributes::ReadOnly, &EditorTransform::m_locked)
                        ;
                }
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                // string-name differs from class-name to avoid collisions with the other "TransformComponent" (AzFramework::TransformComponent).
                behaviorContext->Class<TransformComponent>("EditorTransformBus")->RequestBus("TransformBus");
            }

            AZ::JsonRegistrationContext* jsonRegistration = azrtti_cast<AZ::JsonRegistrationContext*>(context);
            if (jsonRegistration)
            {
                jsonRegistration->Serializer<JsonTransformComponentSerializer>()->HandlesType<TransformComponent>();
            }
        }

        void TransformComponent::AddContextMenuActions(QMenu* menu)
        {
            if (menu)
            {
                if (!menu->actions().empty())
                {
                    menu->addSeparator();
                }

                QAction* resetAction = menu->addAction(QObject::tr("Reset transform values"), [this]()
                {
                    {
                        AzToolsFramework::ScopedUndoBatch undo("Reset transform values");
                        m_editorTransform.m_translate = AZ::Vector3::CreateZero();
                        m_editorTransform.m_legacyScale = AZ::Vector3::CreateOne();
                        m_editorTransform.m_uniformScale = 1.0f;
                        m_editorTransform.m_rotate = AZ::Vector3::CreateZero();
                        OnTransformChanged();
                        SetDirty();
                    }

                    AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_Values);
                });
                resetAction->setEnabled(!m_editorTransform.m_locked);

                QString lockString = m_editorTransform.m_locked ? "Unlock transform values" : "Lock transform values";
                menu->addAction(lockString, [this, lockString]()
                {
                    {
                        AzToolsFramework::ScopedUndoBatch undo(lockString.toUtf8().data());
                        m_editorTransform.m_locked = !m_editorTransform.m_locked;
                        SetDirty();
                    }
                    AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
                });
            }
        }
    }
} // namespace AzToolsFramework
