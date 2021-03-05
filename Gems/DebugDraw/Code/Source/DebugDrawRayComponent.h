/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Script/ScriptTimePoint.h>

#include <DebugDraw/DebugDrawBus.h>

namespace DebugDraw
{
    class DebugDrawRayElement
    {
    public:
        AZ_CLASS_ALLOCATOR(DebugDrawRayElement, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(DebugDrawRayElement, "{BFA68022-208C-4A25-8A33-CF411164F994}");
        static void Reflect(AZ::ReflectContext* context);

        AZ::EntityId            m_startEntityId;
        AZ::EntityId            m_endEntityId;
        float                   m_duration;
        AZ::ScriptTimePoint     m_activateTime;
        AZ::Color               m_color;
        AZ::Vector3             m_worldLocation;
        AZ::Vector3             m_worldDirection;
        AZ::ComponentId         m_owningEditorComponent;

        DebugDrawRayElement()
            : m_duration(0.f)
            , m_color(1.0f, 1.0f, 1.0f, 1.0f) // AZ::Color::White
            , m_worldLocation(AZ::Vector3::CreateZero())
            , m_worldDirection(AZ::Vector3::CreateZero())
            , m_owningEditorComponent(AZ::InvalidComponentId)
        {
        }
    };

    class DebugDrawRayComponent
        : public AZ::Component
    {
        friend class DebugDrawSystemComponent;
    public:
        AZ_COMPONENT(DebugDrawRayComponent, "{7D1C2FE7-541D-4C0A-B10C-D0EA4DE40BA8}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        DebugDrawRayComponent() = default;
        explicit DebugDrawRayComponent(const DebugDrawRayElement& element);
        ~DebugDrawRayComponent() override = default;

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    protected:
        DebugDrawRayElement m_element;
    };
}
