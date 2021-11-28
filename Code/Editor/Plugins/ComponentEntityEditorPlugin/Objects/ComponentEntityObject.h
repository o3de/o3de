/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef CRYINCLUDE_COMPONENTENTITYEDITORPLUGIN_COMPONENTENTITYOBJECT_H
#define CRYINCLUDE_COMPONENTENTITYEDITORPLUGIN_COMPONENTENTITYOBJECT_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/EntityBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIconComponentBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

#include <QtViewPane.h>

#include "../Editor/Objects/EntityObject.h"
#include <LmbrCentral/Rendering/RenderBoundsBus.h>
#endif

class QMenu;

/**
* Sandbox representation of component entities (AZ::Entity).
*/
class CComponentEntityObject
    : public CEntityObject
    , private AzToolsFramework::EditorLockComponentNotificationBus::Handler
    , private AzToolsFramework::EditorVisibilityNotificationBus::Handler
    , private AzToolsFramework::EditorEntityIconComponentNotificationBus::Handler
    , private AZ::TransformNotificationBus::Handler
    , private LmbrCentral::RenderBoundsNotificationBus::Handler
    , private AzToolsFramework::ComponentEntityEditorRequestBus::Handler
    , private AzToolsFramework::ComponentEntityObjectRequestBus::Handler
    , private AZ::EntityBus::Handler
{
public:
    CComponentEntityObject();
    ~CComponentEntityObject();

    //////////////////////////////////////////////////////////////////////////
    // Overrides from CEntityObject/CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    bool Init(IEditor* ie, CBaseObject* prev, const QString& file) override;
    void InitVariables() override {};
    bool SetPos(const Vec3& pos, int flags = 0) override;
    bool SetRotation(const Quat& rotate, int flags) override;
    bool SetScale(const Vec3& scale, int flags) override;
    void InvalidateTM(int nWhyFlags) override;
    void Display(DisplayContext& disp) override;
    bool HitTest(HitContext& hc) override;
    void GetLocalBounds(AABB& box) override;
    void GetBoundBox(AABB& box) override;
    void SetName(const QString& name) override;
    bool IsFrozen() const override;
    void SetFrozen(bool bFrozen) override;
    void SetSelected(bool bSelect) override;
    void SetHighlight(bool bHighlight) override;
    void AttachChild(CBaseObject* child, bool bKeepPos = true) override;
    void DetachAll(bool bKeepPos = true) override;
    void DetachThis(bool bKeepPos = true) override;
    XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode) override;
    void DeleteEntity() override;
    void DrawDefault(DisplayContext& dc, const QColor& labelColor = QColor(255, 255, 255)) override;

    bool IsIsolated() const override;
    bool IsSelected() const override;
    bool IsSelectable() const override;

    void SetWorldPos(const Vec3& pos, int flags = 0) override;

    // Always returns false as Component entity highlighting (accenting) is taken care of elsewhere
    bool IsHighlighted() { return false; }
    // Component entity highlighting (accenting) is taken care of elsewhere
    void DrawHighlight(DisplayContext& /*dc*/) override {};

    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // AZ::EntityBus::Handler
    void OnEntityNameChanged(const AZStd::string& name) override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::EditorLockComponentNotificationBus::Handler
    void OnEntityLockChanged(bool locked) override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::EditorVisibilityNotificationBus::Handler
    void OnEntityVisibilityChanged(bool flag) override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::EditorEntityIconComponentNotificationBus::Handler
    void OnEntityIconChanged(const AZ::Data::AssetId& entityIconAssetId) override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    //! AZ::TransformNotificationBus::Handler
    void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
    void OnParentChanged(AZ::EntityId oldParent, AZ::EntityId newParent) override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    //! RenderBoundsNotificationBus
    void OnRenderBoundsReset() override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    //! ComponentEntityEditorRequestBus
    CEntityObject* GetSandboxObject() override { return this; }
    bool IsSandboxObjectHighlighted() override { return IsHighlighted(); }
    void SetSandboxObjectAccent(AzToolsFramework::EntityAccentType accent) override;
    void SetSandBoxObjectIsolated(bool isIsolated) override;
    bool IsSandBoxObjectIsolated() override;
    void RefreshVisibilityAndLock() override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    //! ComponentEntityObjectRequestBus
    AZ::EntityId GetAssociatedEntityId() override { return m_entityId; }
    void UpdatePreemptiveUndoCache() override;
    //////////////////////////////////////////////////////////////////////////

    void AssignEntity(AZ::Entity* entity, bool destroyOld = true);

    bool IsEntityIconVisible() const { return m_entityIconVisible; }

    static CComponentEntityObject* FindObjectForEntity(AZ::EntityId id);

protected:
    friend class CTemplateObjectClassDesc<CComponentEntityObject>;
    friend class SandboxIntegrationManager;

    static const GUID& GetClassID()
    {
        // {70650EB8-B1BD-4DC8-AC28-7CD767D7BB30}
        static const GUID guid = {
            0x70650EB8, 0xB1BD, 0x4DC8, { 0xac, 0x28, 0x7c, 0xd7, 0x67, 0xd7, 0xbb, 0x30 }
        };
        return guid;
    }

    float GetRadius();

    void DeleteThis() override { delete this; };

    bool IsNonLayerAncestorSelected() const;
    bool IsLayer() const;

    bool IsAncestorIconDrawingAtSameLocation() const;

    bool IsDescendantSelectedAtSameLocation() const;

    void SetupEntityIcon();

    void DrawAccent(DisplayContext& dc);

    class EditorActionGuard
    {
    public:

        EditorActionGuard()
            : m_count(0) {}

        void Enter()    { ++m_count; }
        void Exit()     { --m_count; }

        //! \return true if the guard passes.
        operator bool() const {
            return m_count <= 0;
        }

    private:

        int m_count;
    };

    class EditorActionScope
    {
    public:

        EditorActionScope(EditorActionGuard& guard)
            : m_guard(guard)
        {
            m_guard.Enter();
        }

        ~EditorActionScope()
        {
            m_guard.Exit();
        }

    private:

        EditorActionGuard& m_guard;
    };

    EditorActionGuard m_lockedReentryGuard;
    EditorActionGuard m_nameReentryGuard;
    EditorActionGuard m_selectionReentryGuard;
    EditorActionGuard m_visibilityFlagReentryGuard;
    EditorActionGuard m_transformReentryGuard;
    EditorActionGuard m_parentingReentryGuard;

    AzToolsFramework::EntityAccentType m_accentType;

    //! Whether we have have a valid icon path in \ref m_icon
    bool m_hasIcon;

    //! Whether this component entity icon is visible
    bool m_entityIconVisible;

    //! Whether to only use this object's icon for hit tests. When enabled, we ignore hit tests
    //! against the geometry of the object
    bool m_iconOnlyHitTest;

    //! Whether to draw accents for this object (accents include selection wireframe bounding boxes)
    bool m_drawAccents;

    //! Indicate if an entity is isolated when the editor is in Isolation Mode.
    bool m_isIsolated;

    //! EntityId that this editor object represents/is tied to
    AZ::EntityId m_entityId;

    //! Path to component entity icon for this object
    AZStd::string m_icon;
    ITexture* m_iconTexture;

    //! Displays viewport icon for this entity.
    //! \returns whether an icon is being displayed
    bool DisplayEntityIcon(
        DisplayContext& dc, AzFramework::DebugDisplayRequests& debugDisplay);
};

#endif // CRYINCLUDE_COMPONENTENTITYEDITORPLUGIN_COMPONENTENTITYOBJECT_H
