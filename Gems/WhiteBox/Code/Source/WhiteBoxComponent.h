/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Rendering/WhiteBoxRenderData.h"
#include "WhiteBox/WhiteBoxComponentBus.h"

#include <AzCore/Component/Component.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>
#include <AzFramework/Visibility/VisibleGeometryBus.h>

namespace WhiteBox
{
    class RenderMeshInterface;

    //! Runtime representation of White Box.
    class WhiteBoxComponent
        : public AZ::Component
        , public WhiteBoxComponentRequestBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , public AzFramework::VisibleGeometryRequestBus::Handler
    {
    public:
        AZ_COMPONENT(WhiteBoxComponent, "{6CFD4D82-FA68-4C18-BE67-43FC2B755B64}", AZ::Component)
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        WhiteBoxComponent();
        WhiteBoxComponent(const WhiteBoxComponent&) = delete;
        WhiteBoxComponent& operator=(const WhiteBoxComponent&) = delete;
        WhiteBoxComponent(WhiteBoxComponent&&) = default;
        WhiteBoxComponent& operator=(WhiteBoxComponent&&) = default;
        ~WhiteBoxComponent();

        void SetPhysicsGeometryData(const WhiteBoxRenderData& physicsData);
        void SetBooleanPhysicsGeometryData(const WhiteBoxRenderData& physicsData);
        // Add a getter for the collider to use:
        const WhiteBoxRenderData& GetPhysicsRenderData() const;
        // WhiteBoxComponentRequestBus ...
        bool WhiteBoxIsVisible() const override;
        void SetWhiteBoxVisible(bool visible) override;
        void SetLiveBoolean(bool enabled) override;
        bool GetLiveBoolean() const override;
        void BakeWhiteBox(bool rebakeCollider = false) override;

        //! Set the base (un-boolean) render data. This is the geometry used when the
        //! live-boolean is disabled.
        void GenerateWhiteBoxMesh(const WhiteBoxRenderData& whiteBoxRenderData);
        //! Set the pre-baked boolean-evaluated render data (used when the live-boolean is
        //! enabled). Only meaningful when a boolean source was set in the Editor.
        void SetBooleanRenderData(const WhiteBoxRenderData& booleanRenderData);
        //! Record whether a boolean variant was baked and the initial live-boolean state.
        void SetLiveBooleanState(bool hasBoolean, bool live);

        //! The active render data (base or boolean), used for the collider's runtime debug wireframe.
        const WhiteBoxRenderData& GetActiveRenderData() const;

    private:
        // AZ::Component ...
        void Activate() override;
        void Deactivate() override;

        // TransformNotificationBus ...
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // AzFramework::VisibleGeometryRequestBus::Handler overrides ...
        void BuildVisibleGeometry(const AZ::Aabb& bounds, AzFramework::VisibleGeometryContainer& geometryContainer) const override;

        //! The render data currently selected by m_liveBoolean.
        const WhiteBoxRenderData& ActiveRenderData() const;
        //! (Re)build the render mesh from the currently selected render data.
        void RebuildRenderMesh();

        WhiteBoxRenderData m_whiteBoxRenderData; //!< Base White Box render data (live-boolean disabled).
        WhiteBoxRenderData m_booleanRenderData; //!< Pre-baked boolean-evaluated render data (live-boolean enabled).
        WhiteBoxRenderData m_physicsRenderData;
        WhiteBoxRenderData m_booleanPhysicsRenderData;
        //! Physics geometry was baked (possibly empty, collision off), so never fall back to the visual geometry.
        bool m_hasPhysicsData = false;
        bool m_hasBooleanMesh = false; //!< Whether a boolean-evaluated variant was baked.
        bool m_liveBoolean = false; //!< Whether the boolean-evaluated variant is currently selected.
        AZStd::unique_ptr<RenderMeshInterface> m_renderMesh; //!< The render mesh to use for White Box rendering.
        //! Re-applies the render transform when the entity's Non-Uniform Scale component changes
        //! (the render mesh reads that scale, but a scale change raises no transform notification).
        AZ::NonUniformScaleChangedEvent::Handler m_nonUniformScaleChangedHandler;
    };
} // namespace WhiteBox
