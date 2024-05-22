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
    class DebugDrawObbElement
    {
    public:
        AZ_CLASS_ALLOCATOR(DebugDrawObbElement, AZ::SystemAllocator);
        AZ_TYPE_INFO(DebugDrawObbElement, "{C0B12E93-287A-4170-B1B6-3BF70D1D9433}");
        static void Reflect(AZ::ReflectContext* context);

        AZ::EntityId            m_targetEntityId;
        AZ::Obb                 m_obb;
        float                   m_duration;
        AZ::ScriptTimePoint     m_activateTime;
        AZ::Color               m_color;
        AZ::Vector3             m_worldLocation;
        AZ::ComponentId         m_owningEditorComponent;
        AZ::Vector3             m_scale;

        bool                    m_isRayTracingEnabled = false;

        DebugDrawObbElement()
            : m_duration(0.1f)
            , m_color(1.0f, 1.0f, 1.0f, 1.0f) // AZ::Color::White
            , m_worldLocation(AZ::Vector3::CreateZero())
            , m_owningEditorComponent(AZ::InvalidComponentId)
            , m_scale(AZ::Vector3(1.0f, 1.0f, 1.0f))
            , m_obb(AZ::Obb::CreateFromPositionRotationAndHalfLengths(m_worldLocation, AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne()))
        {}
    };

    class DebugDrawObbComponent
        : public AZ::Component
    {
        friend class DebugDrawSystemComponent;
    public:
        AZ_COMPONENT(DebugDrawObbComponent, "{B1574E9A-C9A1-4A9C-9866-23735ED6FD69}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        DebugDrawObbComponent() = default;
        explicit DebugDrawObbComponent(const DebugDrawObbElement& element);
        ~DebugDrawObbComponent() override = default;

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    protected:
        DebugDrawObbElement m_element;
    };
}
