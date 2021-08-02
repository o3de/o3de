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

    class DebugDrawLineComponent
        : public AZ::Component
    {
        friend class DebugDrawSystemComponent;
    public:
        AZ_COMPONENT(DebugDrawLineComponent, "{E7CBA314-CD9C-4625-AF65-B28369FE272A}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        DebugDrawLineComponent() = default;
        explicit DebugDrawLineComponent(const DebugDrawLineElement& element);
        ~DebugDrawLineComponent() override = default;

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    protected:
        DebugDrawLineElement m_element;
    };
}
