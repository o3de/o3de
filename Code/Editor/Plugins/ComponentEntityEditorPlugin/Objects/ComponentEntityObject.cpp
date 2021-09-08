/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ComponentEntityObject.h"


#include "Settings.h"
#include "IIconManager.h"
#include "Include/IObjectManager.h"
#include "Objects/SelectionGroup.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Math/Transform.h>

#include <AzFramework/Viewport/DisplayContextRequestBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/Commands/PreemptiveUndoCache.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/EditorLayerComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityComponent.h>
#include <AzToolsFramework/ToolsComponents/SelectionComponent.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/ToolsComponents/EditorSelectionAccentSystemComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIconComponentBus.h>
#include <AzToolsFramework/Undo/UndoCacheInterface.h>
#include <LmbrCentral/Rendering/RenderNodeBus.h>

#include <IDisplayViewport.h>
#include <CryCommon/Cry_GeoIntersect.h>
#include <MathConversion.h>
#include <TrackView/TrackViewAnimNode.h>
#include <ViewManager.h>
#include <Viewport.h>

/**
 * Scalars for icon drawing behavior.
 */
static const int s_kIconSize              = 36;       /// Icon display size (in pixels)

CComponentEntityObject::CComponentEntityObject()
    : m_hasIcon(false)
    , m_entityIconVisible(false)
    , m_iconOnlyHitTest(false)
    , m_drawAccents(true)
    , m_accentType(AzToolsFramework::EntityAccentType::None)
    , m_isIsolated(false)
    , m_iconTexture(nullptr)
{
}

CComponentEntityObject::~CComponentEntityObject()
{
    DeleteEntity();
}

bool CComponentEntityObject::Init(IEditor* ie, CBaseObject* copyFrom, const QString& file)
{
    SetColor(RGB(0, 255, 0));
    SetTextureIcon(GetClassDesc()->GetTextureIconId());

    // Sandbox does not serialize this object with others in the layer.
    SetFlags(OBJFLAG_DONT_SAVE);

    const bool result = CEntityObject::Init(ie, copyFrom, file);

    return result;
}

void CComponentEntityObject::UpdatePreemptiveUndoCache()
{
    using namespace AzToolsFramework;

    auto undoCacheInterface = AZ::Interface<UndoSystem::UndoCacheInterface>::Get();
    if (undoCacheInterface)
    {
        undoCacheInterface->UpdateCache(m_entityId);
    }
}

void CComponentEntityObject::AssignEntity(AZ::Entity* entity, bool destroyOld)
{
    const AZ::EntityId newEntityId = entity ? entity->GetId() : AZ::EntityId();

    if (m_entityId.IsValid())
    {
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        LmbrCentral::RenderBoundsNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::ComponentEntityEditorRequestBus::Handler::BusDisconnect();
        AZ::EntityBus::Handler::BusDisconnect();
        AzToolsFramework::ComponentEntityObjectRequestBus::Handler::BusDisconnect();
        AzToolsFramework::EditorLockComponentNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEntityIconComponentNotificationBus::Handler::BusDisconnect();

        if (destroyOld && m_entityId != newEntityId)
        {
            EBUS_EVENT(AzToolsFramework::EditorEntityContextRequestBus, DestroyEditorEntity, m_entityId);
        }

        m_entityId.SetInvalid();
    }

    if (entity)
    {
        m_entityId = entity->GetId();

        // note: GetObjectManager() will always be valid during normal operation but
        // will not exist when running unit tests
        if (IObjectManager* objectManager = GetObjectManager())
        {
            // Use the entity id to generate a GUID for this CEO because we need it to stay consistent for systems
            // that register by GUID such as undo/redo since our own undo/redo system constantly recreates CEOs
            GUID entityBasedGUID;
            entityBasedGUID.Data1 = 0;
            entityBasedGUID.Data2 = 0;
            entityBasedGUID.Data3 = 0;
            static_assert(sizeof(m_entityId) >= sizeof(entityBasedGUID.Data4), "The data contained in entity Id should fit inside Data4, if not switch to some other method of conversion to GUID");
            memcpy(&entityBasedGUID.Data4, &m_entityId, sizeof(entityBasedGUID.Data4));

            objectManager->ChangeObjectId(GetId(), entityBasedGUID);

            // Synchronize sandbox name to new entity's name.
            {
                EditorActionScope nameChange(m_nameReentryGuard);
                SetName(QString(entity->GetName().c_str()));
            }
        }

        EBUS_EVENT(AzToolsFramework::EditorEntityContextRequestBus, AddRequiredComponents, *entity);

        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        LmbrCentral::RenderBoundsNotificationBus::Handler::BusConnect(m_entityId);
        AzToolsFramework::ComponentEntityEditorRequestBus::Handler::BusConnect(m_entityId);
        AZ::EntityBus::Handler::BusConnect(m_entityId);
        AzToolsFramework::ComponentEntityObjectRequestBus::Handler::BusConnect(this);
        AzToolsFramework::EditorLockComponentNotificationBus::Handler::BusConnect(m_entityId);
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusConnect(m_entityId);
        AzToolsFramework::EditorEntityIconComponentNotificationBus::Handler::BusConnect(m_entityId);

        // Synchronize transform to Sandbox.
        AzToolsFramework::Components::TransformComponent* transformComponent =
            entity->FindComponent<AzToolsFramework::Components::TransformComponent>();
        if (transformComponent)
        {
            OnTransformChanged(transformComponent->GetLocalTM(), transformComponent->GetWorldTM());
        }
    }

    RefreshVisibilityAndLock();
}

void CComponentEntityObject::RefreshVisibilityAndLock()
{
    using AzToolsFramework::EditorEntityInfoRequestBus;
    using AzToolsFramework::EditorEntityLockComponentNotificationBus;
    using AzToolsFramework::EditorEntityVisibilityNotificationBus;

    // Lock state is tracked in 3 places:
    // EditorLockComponent, EditorEntityModel, and ComponentEntityObject.
    // Entities in layers have additional behavior in relation to lock state, if the layer is locked it supersedes the entity's lock state.
    // The viewport controls for manipulating entities are disabled during lock state here in ComponentEntityObject using the OBJFLAG_FROZEN.
    // In this case, the lock behavior should include the layer hierarchy as well, if the layer is locked this entity can't move.
    // EditorEntityModel can report this information.
    bool locked = false;
    EditorEntityInfoRequestBus::EventResult(
        locked, m_entityId, &EditorEntityInfoRequestBus::Events::IsLocked);

    EditorEntityLockComponentNotificationBus::Event(
        m_entityId, &EditorEntityLockComponentNotificationBus::Events::OnEntityLockChanged, locked);

    // OBJFLAG_HIDDEN should match EditorVisibilityComponent's VisibilityFlag.
    // Visibility state is similar to lock state in the number of areas it can be set / tracked. See the comment about the lock state above.
    bool visibility = true;
    EditorEntityInfoRequestBus::EventResult(
        visibility, m_entityId, &EditorEntityInfoRequestBus::Events::IsVisible);

    EditorEntityVisibilityNotificationBus::Event(
        m_entityId, &EditorEntityVisibilityNotificationBus::Events::OnEntityVisibilityChanged, visibility);
}

void CComponentEntityObject::SetName(const QString& name)
{
    if (m_nameReentryGuard)
    {
        EditorActionScope nameChange(m_nameReentryGuard);

        AZ::Entity* entity = nullptr;
        EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, m_entityId);

        if (entity)
        {
            entity->SetName(name.toUtf8().data());
        }
    }

    CEntityObject::SetName(name);
}

void CComponentEntityObject::DeleteEntity()
{
    AssignEntity(nullptr);

    CEntityObject::DeleteEntity();
}

float CComponentEntityObject::GetRadius()
{
    static const float s_defaultRadius = 0.5f;
    return s_defaultRadius;
}

void CComponentEntityObject::SetSelected(bool bSelect)
{
    CEntityObject::SetSelected(bSelect);

    if (m_selectionReentryGuard)
    {
        // Ignore event when received from the tools app, since the action is originating in Sandbox.
        EditorActionScope selectionChange(m_selectionReentryGuard);

        // Pass the action to the tools application.
        if (bSelect)
        {
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::MarkEntitySelected, m_entityId);
        }
        else
        {
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::MarkEntityDeselected, m_entityId);
        }
    }

    bool anySelected = false;
    
    AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(anySelected, &AzToolsFramework::ToolsApplicationRequests::AreAnyEntitiesSelected);

    if (!anySelected)
    {
        GetIEditor()->Notify(eNotify_OnEntitiesDeselected);
    }
    else
    {
        GetIEditor()->Notify(eNotify_OnEntitiesSelected);
    }
}

void CComponentEntityObject::SetHighlight(bool bHighlight)
{
    CEntityObject::SetHighlight(bHighlight);

    if (m_entityId.IsValid())
    {
        EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, SetEntityHighlighted, m_entityId, bHighlight);
    }
}

IRenderNode* CComponentEntityObject::GetEngineNode() const
{
    // It's possible for AZ::Entities to have multiple IRenderNodes.
    // However, the editor currently expects a single IRenderNode per "editor object".
    // Therefore, return the highest priority handler.
    if (auto* renderNodeHandler = LmbrCentral::RenderNodeRequestBus::FindFirstHandler(m_entityId))
    {
        return renderNodeHandler->GetRenderNode();
    }
    return nullptr;
}

void CComponentEntityObject::OnEntityNameChanged(const AZStd::string& name)
{
    if (m_nameReentryGuard)
    {
        EditorActionScope selectionChange(m_nameReentryGuard);

        SetName(QString(name.c_str()));
    }
}

void CComponentEntityObject::AttachChild(CBaseObject* child, bool /*bKeepPos*/)
{
    if (child->GetType() == OBJTYPE_AZENTITY)
    {
        CComponentEntityObject* childComponentEntity = static_cast<CComponentEntityObject*>(child);
        AZ::EntityId childEntityId = childComponentEntity->GetAssociatedEntityId();
        if (childEntityId.IsValid())
        {
            // The action is originating from Sandbox, so ignore the return event.
            EditorActionScope parentChange(childComponentEntity->m_parentingReentryGuard);

            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Editor Parent");
                EBUS_EVENT_ID(childEntityId, AZ::TransformBus, SetParent, m_entityId);
                undoBatch.MarkEntityDirty(childEntityId);
            }

            EBUS_EVENT(AzToolsFramework::ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, AzToolsFramework::Refresh_Values);
        }
    }
}

void CComponentEntityObject::DetachAll(bool /*bKeepPos*/)
{
}

void CComponentEntityObject::DetachThis(bool /*bKeepPos*/)
{
    if (m_parentingReentryGuard)
    {
        EditorActionScope parentChange(m_parentingReentryGuard);

        if (m_entityId.IsValid())
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("Editor Unparent");
            EBUS_EVENT_ID(m_entityId, AZ::TransformBus, SetParent, AZ::EntityId());
            undoBatch.MarkEntityDirty(m_entityId);
        }

        EBUS_EVENT(AzToolsFramework::ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, AzToolsFramework::Refresh_Values);
    }
}

CBaseObject* CComponentEntityObject::GetLinkParent() const
{
    AZ::EntityId parentId;
    EBUS_EVENT_ID_RESULT(parentId, m_entityId, AZ::TransformBus, GetParentId);

    return CComponentEntityObject::FindObjectForEntity(parentId);
}

bool CComponentEntityObject::IsFrozen() const
{
    return CheckFlags(OBJFLAG_FROZEN);
}

void CComponentEntityObject::SetFrozen(bool frozen)
{
    if (m_lockedReentryGuard)
    {
        EditorActionScope flagChange(m_lockedReentryGuard);
        AzToolsFramework::SetEntityLockState(m_entityId, frozen);
    }
}

void CComponentEntityObject::OnEntityLockChanged(bool locked)
{
    CEntityObject::SetFrozen(locked);
}

void CComponentEntityObject::SetHidden(
    bool bHidden, [[maybe_unused]] uint64 hiddenId /*=CBaseObject::s_invalidHiddenID*/, [[maybe_unused]] bool bAnimated /*=false*/)
{
    if (m_visibilityFlagReentryGuard)
    {
        EditorActionScope flagChange(m_visibilityFlagReentryGuard);
        AzToolsFramework::SetEntityVisibility(m_entityId, !bHidden);
    }
}

void CComponentEntityObject::OnEntityVisibilityChanged(bool visible)
{
    CEntityObject::SetHidden(!visible);
}

void CComponentEntityObject::OnEntityIconChanged(const AZ::Data::AssetId& entityIconAssetId)
{
    (void)entityIconAssetId;
    SetupEntityIcon();
}

void CComponentEntityObject::OnParentChanged([[maybe_unused]] AZ::EntityId oldParent, [[maybe_unused]] AZ::EntityId newParent)
{
}

void CComponentEntityObject::OnRenderBoundsReset()
{
    CEntityObject::InvalidateTM(0);
}

void CComponentEntityObject::SetSandboxObjectAccent(AzToolsFramework::EntityAccentType accent)
{
    m_accentType = accent;
    AzToolsFramework::EditorComponentSelectionNotificationsBus::Event(m_entityId,
        &AzToolsFramework::EditorComponentSelectionNotificationsBus::Events::OnAccentTypeChanged, m_accentType);
}

void CComponentEntityObject::SetSandBoxObjectIsolated(bool isIsolated)
{
    m_isIsolated = isIsolated;
    GetIEditor()->GetObjectManager()->InvalidateVisibleList();
}

bool CComponentEntityObject::IsSandBoxObjectIsolated()
{
    return m_isIsolated;
}

bool CComponentEntityObject::SetPos(const Vec3& pos, int flags)
{
    bool isAzEditorTransformLocked = false;
    AzToolsFramework::Components::TransformComponentMessages::Bus::EventResult(isAzEditorTransformLocked, m_entityId,
        &AzToolsFramework::Components::TransformComponentMessages::IsTransformLocked);

    bool lockTransformOnUserInput = isAzEditorTransformLocked && (flags & eObjectUpdateFlags_UserInput);

    if (IsLayer() || lockTransformOnUserInput)
    {
        return false;
    }
    if ((flags & eObjectUpdateFlags_MoveTool) || (flags & eObjectUpdateFlags_UserInput))
    {
        // If we have a parent also in the selection set, don't allow the move tool to manipulate our position.
        if (IsNonLayerAncestorSelected())
        {
            return false;
        }
    }

    return CEntityObject::SetPos(pos, flags);
}

bool CComponentEntityObject::SetRotation(const Quat& rotate, int flags)
{
    bool isAzEditorTransformLocked = false;
    AzToolsFramework::Components::TransformComponentMessages::Bus::EventResult(isAzEditorTransformLocked, m_entityId,
        &AzToolsFramework::Components::TransformComponentMessages::IsTransformLocked);

    bool lockTransformOnUserInput = isAzEditorTransformLocked && (flags & eObjectUpdateFlags_UserInput);

    if (IsLayer() || lockTransformOnUserInput)
    {
        return false;
    }
    if (flags & eObjectUpdateFlags_UserInput)
    {
        // If we have a parent also in the selection set, don't allow the rotate tool to manipulate our position.
        if (IsNonLayerAncestorSelected())
        {
            return false;
        }
    }

    return CEntityObject::SetRotation(rotate, flags);
}

bool CComponentEntityObject::SetScale(const Vec3& scale, int flags)
{
    bool isAzEditorTransformLocked = false;
    AzToolsFramework::Components::TransformComponentMessages::Bus::EventResult(isAzEditorTransformLocked, m_entityId,
        &AzToolsFramework::Components::TransformComponentMessages::IsTransformLocked);

    bool lockTransformOnUserInput = isAzEditorTransformLocked && (flags & eObjectUpdateFlags_UserInput);

    if (IsLayer() || lockTransformOnUserInput)
    {
        return false;
    }
    if ((flags & eObjectUpdateFlags_ScaleTool) || (flags & eObjectUpdateFlags_UserInput))
    {
        // If we have a parent also in the selection set, don't allow the scale tool to manipulate our position.
        if (IsNonLayerAncestorSelected())
        {
            return false;
        }
    }

    return CEntityObject::SetScale(scale, flags);
}

bool CComponentEntityObject::IsNonLayerAncestorSelected() const
{
    AZ::EntityId parentId;
    AZ::TransformBus::EventResult(parentId, m_entityId, &AZ::TransformBus::Events::GetParentId);
    while (parentId.IsValid())
    {
        CComponentEntityObject* parentObject = CComponentEntityObject::FindObjectForEntity(parentId);
        if (parentObject && parentObject->IsSelected())
        {
            bool isLayerEntity = false;
            AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
                isLayerEntity,
                parentObject->GetAssociatedEntityId(),
                &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
            if (!isLayerEntity)
            {
                return true;
            }
        }

        AZ::EntityId currentParentId = parentId;
        parentId.SetInvalid();
        AZ::TransformBus::EventResult(parentId, currentParentId, &AZ::TransformBus::Events::GetParentId);
    }

    return false;
}

bool CComponentEntityObject::IsLayer() const
{
    bool isLayerEntity = false;
    AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
        isLayerEntity,
        m_entityId,
        &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
    return isLayerEntity;
}

bool CComponentEntityObject::IsAncestorIconDrawingAtSameLocation() const
{
    if (m_entityId.IsValid())
    {
        AZ::EntityId parentId;
        AZ::TransformBus::EventResult(parentId, m_entityId, &AZ::TransformBus::Events::GetParentId);
        if (!parentId.IsValid())
        {
            return false;
        }

        AZ::Vector3 worldTranslation;
        AZ::TransformBus::EventResult(worldTranslation, m_entityId, &AZ::TransformBus::Events::GetWorldTranslation);

        while (parentId.IsValid())
        {
            AZ::Vector3 parentTranslation;
            AZ::TransformBus::EventResult(parentTranslation, parentId, &AZ::TransformBus::Events::GetWorldTranslation);

            if (parentTranslation.GetDistanceSq(worldTranslation) < 0.01f)
            {
                CComponentEntityObject* parentObject = CComponentEntityObject::FindObjectForEntity(parentId);
                if (parentObject && !parentObject->IsSelected() && parentObject->IsEntityIconVisible())
                {
                    // An ancestor in the same location that's not selected and has icon visible has been found
                    return true;
                }
            }

            AZ::EntityId currentParentId = parentId;
            parentId.SetInvalid();
            AZ::TransformBus::EventResult(parentId, currentParentId, &AZ::TransformBus::Events::GetParentId);
        }
    }
    return false;
}

bool CComponentEntityObject::IsDescendantSelectedAtSameLocation() const
{
    if (m_entityId.IsValid())
    {
        if (GetObjectManager()->GetSelection() && GetObjectManager()->GetSelection()->GetCount() == 0)
        {
            return false;
        }

        AZ::Vector3 worldTranslation;
        AZ::TransformBus::EventResult(worldTranslation, m_entityId, &AZ::TransformBus::Events::GetWorldTranslation);

        // For each descendant, check if it's selected and if so if it's located at the same location as we are
        AzToolsFramework::EntityIdList descendantIds;
        AZ::TransformBus::EventResult(descendantIds, m_entityId, &AZ::TransformBus::Events::GetAllDescendants);
        for (AZ::EntityId entityId : descendantIds)
        {
            CComponentEntityObject* descendantObject = CComponentEntityObject::FindObjectForEntity(entityId);
            if (descendantObject && descendantObject->IsSelected())
            {
                // Check if this entity is at the exact location of us
                AZ::Vector3 entityTranslation;
                AZ::TransformBus::EventResult(entityTranslation, entityId, &AZ::TransformBus::Events::GetWorldTranslation);
                if (entityTranslation.GetDistanceSq(worldTranslation) < 0.01f)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

void CComponentEntityObject::InvalidateTM(int nWhyFlags)
{
    CEntityObject::InvalidateTM(nWhyFlags);

    if (m_transformReentryGuard)
    {
        EditorActionScope transformChange(m_transformReentryGuard);

        if (m_entityId.IsValid())
        {
            Matrix34 worldTransform = GetWorldTM();
            EBUS_EVENT_ID(m_entityId, AZ::TransformBus, SetWorldTM, LYTransformToAZTransform(worldTransform));
        }
    }
}

void CComponentEntityObject::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
{
    if (m_transformReentryGuard) // Ignore if action originated from Sandbox.
    {
        EditorActionScope transformChange(m_transformReentryGuard);
        Matrix34 worlTM = AZTransformToLYTransform(world);
        SetLocalTM(worlTM, eObjectUpdateFlags_Animated);
    }
}

int CComponentEntityObject::MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    if (event == eMouseMove || event == eMouseLDown)
    {
        Vec3 pos;
        if (GetIEditor()->GetAxisConstrains() != AXIS_TERRAIN)
        {
            pos = view->MapViewToCP(point);
        }
        else
        {
            // Snap to terrain.
            bool hitTerrain;
            pos = view->ViewToWorld(point, &hitTerrain);
            if (hitTerrain)
            {
                pos.z = GetIEditor()->GetTerrainElevation(pos.x, pos.y);
            }
            pos = view->SnapToGrid(pos);
        }

        pos = view->SnapToGrid(pos);
        SetPos(pos);

        if (event == eMouseLDown)
        {
            return MOUSECREATE_OK;
        }

        return MOUSECREATE_CONTINUE;
    }

    return CBaseObject::MouseCreateCallback(view, event, point, flags);
}

bool CComponentEntityObject::HitHelperTest(HitContext& hc)
{
    bool hit = CEntityObject::HitHelperTest(hc);
    if (!hit && m_entityId.IsValid())
    {
        // Pick against icon in screen space.
        if (IsEntityIconVisible())
        {
            const QPoint entityScreenPos = hc.view->WorldToView(GetWorldPos());
            const float screenPosX = static_cast<float>(entityScreenPos.x());
            const float screenPosY = static_cast<float>(entityScreenPos.y());
            const float iconRange = static_cast<float>(s_kIconSize / 2);

            if ((hc.point2d.x() >= screenPosX - iconRange && hc.point2d.x() <= screenPosX + iconRange)
                && (hc.point2d.y() >= screenPosY - iconRange && hc.point2d.y() <= screenPosY + iconRange))
            {
                hc.dist = hc.raySrc.GetDistance(GetWorldPos());
                hc.iconHit = true;
                return true;
            }
        }
    }
    return hit;
}

bool CComponentEntityObject::HitTest(HitContext& hc)
{
    AZ_PROFILE_FUNCTION(Entity);

    if (m_iconOnlyHitTest)
    {
        return false;
    }

    if (m_entityId.IsValid())
    {
        // Pick against bounding box/geometry
        AABB bounds(AABB::RESET);
        GetBoundBox(bounds);
        if (!bounds.IsReset())
        {
            Vec3 hitPos;
            if (Intersect::Ray_AABB(Ray(hc.raySrc, hc.rayDir), bounds, hitPos))
            {
                bool rayIntersection = false;
                bool preciseSelectionRequired = false;
                float closestDistance = std::numeric_limits<float>::max();

                const int viewportId = GetIEditor()->GetViewManager()->GetGameViewport()->GetViewportId();
                AzToolsFramework::EditorComponentSelectionRequestsBus::EnumerateHandlersId(m_entityId,
                    [&hc, &closestDistance, &rayIntersection, &preciseSelectionRequired, viewportId](
                        AzToolsFramework::EditorComponentSelectionRequests* handler) -> bool
                {
                    AZ_PROFILE_FUNCTION(Entity);

                    if (handler->SupportsEditorRayIntersect())
                    {
                        float distance = std::numeric_limits<float>::max();
                        preciseSelectionRequired = true;
                        const bool intersection = handler->EditorSelectionIntersectRayViewport(
                            { viewportId }, LYVec3ToAZVec3(hc.raySrc), LYVec3ToAZVec3(hc.rayDir), distance);

                        rayIntersection = rayIntersection || intersection;

                        if (intersection && distance < closestDistance)
                        {
                            closestDistance = distance;
                        }
                    }

                    return true; // iterate over all handlers
                });

                hc.object = this;

                if (preciseSelectionRequired)
                {
                    hc.dist = closestDistance;
                    return rayIntersection;
                }

                hc.dist = (hitPos - hc.raySrc).GetLength();
                return true;

            }

            return false;
        }
    }

    const Vec3 origin = GetWorldPos();
    const float radius = GetRadius();

    Vec3 w = origin - hc.raySrc;
    Vec3 wcross = hc.rayDir.Cross(w);
    const float d = wcross.GetLengthSquared();

    if (d < square(radius) + hc.distanceTolerance &&
        w.GetLengthSquared() > square(radius))
    {
        Vec3 i0;
        hc.object = this;
        if (Intersect::Ray_SphereFirst(Ray(hc.raySrc, hc.rayDir), ::Sphere(origin, radius), i0))
        {
            hc.dist = hc.raySrc.GetDistance(i0);
            return true;
        }
        hc.dist = hc.raySrc.GetDistance(origin);
        return true;
    }

    return false;
}

void CComponentEntityObject::GetBoundBox(AABB& box)
{
    AZ_PROFILE_FUNCTION(Entity);

    box.Reset();

    const AZ::EntityId entityId = m_entityId;
    if (entityId.IsValid())
    {
        CViewport* gameViewport = GetIEditor()->GetViewManager()->GetGameViewport();
        const int viewportId =  gameViewport ? gameViewport->GetViewportId() : -1;

        if (const AZ::Aabb bound =
                AzToolsFramework::CalculateEditorEntitySelectionBounds(entityId, AzFramework::ViewportInfo{viewportId});
            bound.IsValid())
        {
            box.Add(AZVec3ToLYVec3(bound.GetMin()));
            box.Add(AZVec3ToLYVec3(bound.GetMax()));
            return;
        }
    }

    CBaseObject::GetBoundBox(box);
}

void CComponentEntityObject::GetLocalBounds(AABB& box)
{
    box.Reset();

    float r = GetRadius();
    box.min = -Vec3(r, r, r);
    box.max = Vec3(r, r, r);
}

XmlNodeRef CComponentEntityObject::Export([[maybe_unused]] const QString& levelPath, [[maybe_unused]] XmlNodeRef& xmlNode)
{
    // All component entities are serialized out in a separate pass, so they can be
    // loaded en-masse rather than individually. As such, we don't export them
    // alongside Cry Entities.
    return XmlNodeRef();
}

CComponentEntityObject* CComponentEntityObject::FindObjectForEntity(AZ::EntityId id)
{
    CEntityObject* object = nullptr;
    EBUS_EVENT_ID_RESULT(object, id, AzToolsFramework::ComponentEntityEditorRequestBus, GetSandboxObject);

    if (object && (object->GetType() == OBJTYPE_AZENTITY))
    {
        return static_cast<CComponentEntityObject*>(object);
    }

    return nullptr;
}

void CComponentEntityObject::Display(DisplayContext& dc)
{
    if (!(dc.flags & DISPLAY_2D))
    {
        m_entityIconVisible = false;
    }

    bool displaySelectionHelper = false;
    if (!CanBeDrawn(dc, displaySelectionHelper))
    {
        return;
    }

    DrawDefault(dc);

    bool showIcons = m_hasIcon;
    if (showIcons)
    {
        SEditorSettings* editorSettings = GetIEditor()->GetEditorSettings();
        if (!editorSettings->viewports.bShowIcons && !editorSettings->viewports.bShowSizeBasedIcons)
        {
            showIcons = false;
        }
    }

    if (m_entityId.IsValid())
    {
        // Draw link to parent if this or the parent object are selected.
        {
            AZ::EntityId parentId;
            EBUS_EVENT_ID_RESULT(parentId, m_entityId, AZ::TransformBus, GetParentId);
            if (parentId.IsValid())
            {
                bool isParentVisible = false;
                AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isParentVisible, parentId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsVisible);

                CComponentEntityObject* parentObject = CComponentEntityObject::FindObjectForEntity(parentId);
                if (isParentVisible && (IsSelected() || (parentObject && parentObject->IsSelected())))
                {
                    const QColor kLinkColorParent(0, 255, 255);
                    const QColor kLinkColorChild(0, 0, 255);

                    AZ::Vector3 parentTranslation;
                    EBUS_EVENT_ID_RESULT(parentTranslation, parentId, AZ::TransformBus, GetWorldTranslation);
                    dc.DrawLine(AZVec3ToLYVec3(parentTranslation), GetWorldTM().GetTranslation(), kLinkColorParent, kLinkColorChild);
                }
            }
        }

        // Don't draw icons if we have an ancestor in the same location that has an icon - makes sure
        // ancestor icons draw on top and are able to be selected over children. Also check if a descendant
        // is selected at the same location. In cases of entity hierarchies where numerous ancestors have
        // no position offset, we need this so the ancestors don't draw over us when we're selected
        if (showIcons)
        {
            if ((dc.flags & DISPLAY_2D) ||
                IsSelected() ||
                IsAncestorIconDrawingAtSameLocation() ||
                IsDescendantSelectedAtSameLocation())
            {
                showIcons = false;
            }
        }

        // Allow components to override in-editor visualization.
        {
            const AzFramework::DisplayContextRequestGuard displayContextGuard(dc);

            AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
            AzFramework::DebugDisplayRequestBus::Bind(
                debugDisplayBus, AzFramework::g_defaultSceneEntityDebugDisplayId);
            AZ_Assert(debugDisplayBus, "Invalid DebugDisplayRequestBus.");

            AzFramework::DebugDisplayRequests* debugDisplay =
                AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);

            AzFramework::EntityDebugDisplayEventBus::Event(
                m_entityId, &AzFramework::EntityDebugDisplayEvents::DisplayEntityViewport,
                AzFramework::ViewportInfo{ dc.GetView()->asCViewport()->GetViewportId() },
                *debugDisplay);
        }
    }
}

void CComponentEntityObject::DrawDefault(DisplayContext& dc, const QColor& labelColor)
{
    CEntityObject::DrawDefault(dc, labelColor);

    DrawAccent(dc);
}

IStatObj* CComponentEntityObject::GetIStatObj()
{
    return nullptr;
}

bool CComponentEntityObject::IsIsolated() const
{
    return m_isIsolated;
}

bool CComponentEntityObject::IsSelected() const
{
    return AzToolsFramework::IsSelected(m_entityId);
}

bool CComponentEntityObject::IsSelectable() const
{
    return AzToolsFramework::IsSelectableInViewport(m_entityId);
}

void CComponentEntityObject::SetWorldPos(const Vec3& pos, int flags)
{
    // Layers, by design, are not supposed to be moveable. Layers are intended to just be a grouping
    // mechanism to allow teams to cleanly split their level into working zones, and a moveable position
    // complicates that behavior more than it helps.
    // Unfortunately component entity objects have a position under the hood, so prevent layers from moving here.
    bool isLayerEntity = false;
    AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
        isLayerEntity,
        m_entityId,
        &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);

    bool isAzEditorTransformLocked = false;
    AzToolsFramework::Components::TransformComponentMessages::Bus::EventResult(isAzEditorTransformLocked, m_entityId,
        &AzToolsFramework::Components::TransformComponentMessages::IsTransformLocked);

    bool lockTransformOnUserInput = isAzEditorTransformLocked && (flags & eObjectUpdateFlags_UserInput);

    if (isLayerEntity || lockTransformOnUserInput)
    {
        return;
    }
    CEntityObject::SetWorldPos(pos, flags);
}

void CComponentEntityObject::OnContextMenu(QMenu* /*pMenu*/)
{
    // Deliberately bypass the base class implementation (CEntityObject::OnContextMenu()).
}

void CComponentEntityObject::SetupEntityIcon()
{
    bool hideIconInViewport = false;
    m_hasIcon = false;

    AzToolsFramework::EditorEntityIconComponentRequestBus::EventResult(hideIconInViewport, m_entityId, &AzToolsFramework::EditorEntityIconComponentRequestBus::Events::IsEntityIconHiddenInViewport);

    if (!hideIconInViewport)
    {
        AzToolsFramework::EditorEntityIconComponentRequestBus::EventResult(m_icon, m_entityId, &AzToolsFramework::EditorEntityIconComponentRequestBus::Events::GetEntityIconPath);

        if (!m_icon.empty())
        {
            m_hasIcon = true;

            // ToDo: Get from Atom?
            // int textureId = GetIEditor()->GetIconManager()->GetIconTexture(m_icon.c_str());
            m_iconTexture = nullptr;
        }
    }
}

void CComponentEntityObject::DrawAccent(DisplayContext& dc)
{
    if (!m_drawAccents)
    {
        return;
    }

    switch (m_accentType)
    {
        case AzToolsFramework::EntityAccentType::None:
        {
            if (dc.flags & DISPLAY_2D)
            {
                dc.SetColor(0.941f, 0.764f, 0.176f); // Yellow
            }
            else
            {
                return;
            }
            break;
        }
        case AzToolsFramework::EntityAccentType::Hover:
        {
            dc.SetColor(0, 1, 0); // Green
            break;
        }
        case AzToolsFramework::EntityAccentType::Selected:
        {
            dc.SetColor(1, 0, 0); // Red
            break;
        }
        case AzToolsFramework::EntityAccentType::ParentSelected:
        {
            dc.SetColor(1, 0.549f, 0); // Orange
            break;
        }
        case AzToolsFramework::EntityAccentType::SliceSelected:
        {
            dc.SetColor(0.117f, 0.565f, 1); // Blue
            break;
        }
        default:
        {
            dc.SetColor(1, 0.0784f, 0.576f); // Pink
            break;
        }
    }

    // if there are no explicit handlers, default to show the aabb when the mouse is over or the entity is selected.
    // this will be the case with newly added entities without explicit handlers attached (no components).
    if (!AzToolsFramework::EditorComponentSelectionRequestsBus::HasHandlers(m_entityId))
    {
        AABB box;
        GetBoundBox(box);
        dc.DrawWireBox(box.min, box.max);
    }
}
