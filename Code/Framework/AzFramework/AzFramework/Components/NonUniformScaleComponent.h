/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Component/Component.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Math/Vector3.h>

namespace AzFramework
{
    class NonUniformScaleComponent
        : public AZ::Component
        , public AZ::NonUniformScaleRequestBus::Handler
    {
    public:
        AZ_COMPONENT(NonUniformScaleComponent, "{077A7A44-BC44-4357-840D-E8E193ADE991}");

        static void Reflect(AZ::ReflectContext* context);

        NonUniformScaleComponent() = default;
        ~NonUniformScaleComponent() = default;

        // AZ::NonUniformScaleRequests::Handler ...
        AZ::Vector3 GetScale() const override;
        void SetScale(const AZ::Vector3& scale) override;
        void RegisterScaleChangedEvent(AZ::NonUniformScaleChangedEvent::Handler& handler) override;

    protected:
        // AZ::Component ...
        void Activate() override;
        void Deactivate() override;

    private:
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        AZ::Vector3 m_scale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleChangedEvent m_scaleChangedEvent;
    };
} // namespace AzFramework
