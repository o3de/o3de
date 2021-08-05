/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace ScriptEventsBuilder
{
    class BuilderSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(BuilderSystemComponent, "{6CE4EF5D-5A18-4E25-A676-501644676B58}");

        BuilderSystemComponent();
        ~BuilderSystemComponent() override;

        static void Reflect(AZ::ReflectContext* context);

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component...
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    private:
        BuilderSystemComponent(const BuilderSystemComponent&) = delete;

    };
}
