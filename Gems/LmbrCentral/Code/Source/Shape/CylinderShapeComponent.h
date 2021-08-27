/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

#include "Rendering/EntityDebugDisplayComponent.h"
#include "CylinderShape.h"

namespace LmbrCentral
{
    /// Provide a Component interface for CylinderShape functionality.
    class CylinderShapeComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(CylinderShapeComponent, CylinderShapeComponentTypeId);
        static void Reflect(AZ::ReflectContext* context);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        CylinderShape m_cylinderShape; ///< Stores underlying cylinder type for this component.
    };

    /// Concrete EntityDebugDisplay implementation for CylinderShape.
    class CylinderShapeDebugDisplayComponent
        : public EntityDebugDisplayComponent
        , public ShapeComponentNotificationsBus::Handler
    {
    public:
        AZ_COMPONENT(CylinderShapeDebugDisplayComponent, "{13F00855-7BB6-447A-9E8D-61F37275BC95}", EntityDebugDisplayComponent)
        static void Reflect(AZ::ReflectContext* context);

        CylinderShapeDebugDisplayComponent() = default;

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        // EntityDebugDisplayComponent
        void Draw(AzFramework::DebugDisplayRequests& debugDisplay) override;

    private:
        AZ_DISABLE_COPY_MOVE(CylinderShapeDebugDisplayComponent)

        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeChangeReasons changeReason) override;

        CylinderShapeConfig m_cylinderShapeConfig; ///< Stores configuration data for cylinder shape.
    };
} // namespace LmbrCentral
