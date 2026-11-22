/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>

namespace DebugDraw
{
    class DebugDrawOctreeComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(DebugDrawOctreeComponent, "{D917C18B-B830-11F1-AA5A-F02F74FA1AF1}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        DebugDrawOctreeComponent() = default;
        ~DebugDrawOctreeComponent() override = default;

        ////////////////////////////////////////////////////////////////////////
        // AZ::TickBus::Handler interface implementation
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        ////////////////////////////////////////////////////////////////////////

        //! Enumerates the default visibility scene and draws its node bounds (orange) and entry volumes (cyan)
        //! via the DebugDraw system. Shared by the runtime component (per game tick) and the editor component
        //! (edit-mode preview).
        static void DrawVisibilityOctree();

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };
}