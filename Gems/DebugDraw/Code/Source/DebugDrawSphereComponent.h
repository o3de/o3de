/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Script/ScriptTimePoint.h>

#include <DebugDraw/DebugDrawBus.h>

namespace DebugDraw
{
    class DebugDrawSphereElement
    {
    public:
        AZ_CLASS_ALLOCATOR(DebugDrawSphereElement, AZ::SystemAllocator);
        AZ_TYPE_INFO(DebugDrawSphereElement, "{CB6F2781-DC26-4A10-8C5F-E07032574087}");
        static void Reflect(AZ::ReflectContext* context);

        float                   m_duration;
        AZ::ScriptTimePoint     m_activateTime;
        AZ::Color               m_color;

        AZ::EntityId            m_targetEntityId;

        AZ::Vector3             m_worldLocation;
        float                   m_radius;

        bool                    m_isRayTracingEnabled = false;

        AZ::ComponentId         m_owningEditorComponent;

        DebugDrawSphereElement()
            : m_duration(0.f)
            , m_radius(1.0f)
            , m_color(1.0f, 1.0f, 1.0f, 1.0f) // AZ::Color::White
            , m_worldLocation(AZ::Vector3::CreateZero())
        {
        }
    };

    class DebugDrawSphereComponent
        : public AZ::Component
    {
        friend class DebugDrawSystemComponent;
    public:
        AZ_COMPONENT(DebugDrawSphereComponent, "{823F6C96-627E-4C98-A3B9-0168B5CB3706}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        DebugDrawSphereComponent() = default;
        explicit DebugDrawSphereComponent(const DebugDrawSphereElement& element);
        ~DebugDrawSphereComponent() override = default;

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    protected:
        DebugDrawSphereElement m_element;
    };
}
