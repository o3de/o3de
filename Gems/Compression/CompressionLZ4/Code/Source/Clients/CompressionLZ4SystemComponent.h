/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <CompressionLZ4/CompressionLZ4Bus.h>
#include <CompressionLZ4/CompressionLZ4TypeIds.h>

namespace CompressionLZ4
{
    class CompressionLZ4SystemComponent
        : public AZ::Component
        , protected CompressionLZ4RequestBus::Handler
    {
    public:
        AZ_COMPONENT_DECL(CompressionLZ4SystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        CompressionLZ4SystemComponent();
        ~CompressionLZ4SystemComponent();

    protected:
        // CompressionLZ4RequestBus interface implementation

        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
    };

} // namespace CompressionLZ4
