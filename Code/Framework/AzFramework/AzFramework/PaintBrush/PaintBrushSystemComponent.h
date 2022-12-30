/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/PaintBrush/PaintBrush.h>
#include <AzFramework/PaintBrush/PaintBrushSessionBus.h>

namespace AzFramework
{
    //! PaintBrushSystemComponent generically manages runtime paint brush sessions for any components that support runtime painting.
    class PaintBrushSystemComponent
        : public AZ::Component
        , protected AzFramework::PaintBrushSessionBus::Handler
    {
    public:
        AZ_COMPONENT(PaintBrushSystemComponent, "{EB1CDA1E-A2FD-4AEF-AFBF-B39ED18463AE}");

        PaintBrushSystemComponent() = default;
        ~PaintBrushSystemComponent() override = default;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        void Activate() override;
        void Deactivate() override;

        // PaintBrushSessionBus overrides...
        AZ::Uuid StartPaintSession(const AZ::EntityId& paintableEntityId) override;
        void EndPaintSession(const AZ::Uuid& sessionId) override;
        void BeginBrushStroke(const AZ::Uuid& sessionId, const AzFramework::PaintBrushSettings& brushSettings) override;
        void EndBrushStroke(const AZ::Uuid& sessionId) override;
        bool IsInBrushStroke(const AZ::Uuid& sessionId) const override;
        void ResetBrushStrokeTracking(const AZ::Uuid& sessionId) override;
        void PaintToLocation(
            const AZ::Uuid& sessionId, const AZ::Vector3& brushCenter, const AzFramework::PaintBrushSettings& brushSettings) override;
        void SmoothToLocation(
            const AZ::Uuid& sessionId, const AZ::Vector3& brushCenter, const AzFramework::PaintBrushSettings& brushSettings) override;

        //! Instances of a PaintBrush for image modifications that exist for use with the high-level paint APIs exposed
        //! on the PaintBrushSession. This is *only* used by the paint APIs on the bus - the Editor has its own instance
        //! of a PaintBrush that it uses in conjunction with mouse tracking, manipulator drawing, etc.
        //! These are only created and active between StartPaintSession / EndPaintSession calls.
        AZStd::unordered_map<AZ::Uuid, AZStd::shared_ptr<AzFramework::PaintBrush>> m_brushSessions;
    };
}; // namespace AzFramework
