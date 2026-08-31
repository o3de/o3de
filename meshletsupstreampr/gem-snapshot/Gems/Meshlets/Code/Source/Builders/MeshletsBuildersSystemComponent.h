/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Builders/JsonSidecarBuilder.h>

namespace AZ::Meshlets::Builders
{
    class MeshletsBuildersSystemComponent : public AZ::Component
    {
    public:
        AZ_COMPONENT(MeshletsBuildersSystemComponent,
                     "{4A8E7C29-2D5F-41B6-8C9A-3E1F7B5D2A60}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType&);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType&);

        void Init() override {}
        void Activate() override;
        void Deactivate() override;

    private:
        JsonSidecarBuilder m_jsonBuilder;
    };

} // namespace AZ::Meshlets::Builders
