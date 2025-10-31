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

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace DebugDraw
{

    class DebugDrawTextElement
    {
    public:

        AZ_CLASS_ALLOCATOR(DebugDrawTextElement, AZ::SystemAllocator);
        AZ_TYPE_INFO(DebugDrawTextElement, "{A49413DB-0AFC-4D38-BD4B-EDC8FA83B640}");
        static void Reflect(AZ::ReflectContext* context);

        enum class DrawMode
        {
            OnScreen,
            InWorld,
        };

        DrawMode m_drawMode = DrawMode::OnScreen;
        float m_duration = 0.0f;
        float m_size = 1.4f;
        AZStd::string m_text = "";
        bool m_centered = false;
        AZ::ScriptTimePoint m_activateTime;
        AZ::Color m_color = AZ::Color(1.0f, 1.0f, 1.0f, 1.0f);
        AZ::EntityId m_targetEntityId;
        AZ::Vector3 m_worldLocation = AZ::Vector3::CreateZero();
        AZ::ComponentId m_owningEditorComponent = AZ::InvalidComponentId;
        float m_fontScale = 1.0f; // Scale factor to default render font
        bool m_useOnScreenCoordinates = false; // Whether to use m_worldLocation.X and m_worldLocation.Y as OnScreen 2d Coordinates
        bool m_bCenter = false; // If true, centers drawn text relative to x coordinate
    };


    class DebugDrawTextComponent
        : public AZ::Component
    {
        friend class DebugDrawSystemComponent;
    public:
        AZ_COMPONENT(DebugDrawTextComponent, "{A705A8DF-31F1-49FF-8727-CC7783E09EF9}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        DebugDrawTextComponent() = default;
        explicit DebugDrawTextComponent(const DebugDrawTextElement& textElement);
        ~DebugDrawTextComponent() override = default;

        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;

    protected:
        DebugDrawTextElement m_element;
    };
}
