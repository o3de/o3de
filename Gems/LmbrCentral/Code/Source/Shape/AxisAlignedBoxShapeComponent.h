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
#include "AxisAlignedBoxShape.h"

namespace LmbrCentral
{
    /// Provide a Component interface for AxisAlignedBoxShape functionality.
    class AxisAlignedBoxShapeComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(AxisAlignedBoxShapeComponent, AxisAlignedBoxShapeComponentTypeId)
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
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        AxisAlignedBoxShape m_aaboxShape; ///< Stores underlying box type for this component.
    };

    /// Concrete EntityDebugDisplay implementation for BoxShape.
    class AxisAlignedBoxShapeDebugDisplayComponent
        : public EntityDebugDisplayComponent
        , public ShapeComponentNotificationsBus::Handler
    {
    public:
        AZ_COMPONENT(AxisAlignedBoxShapeDebugDisplayComponent, "{BA93F933-1DC9-4E0E-B930-A7E3968D5DD1}", EntityDebugDisplayComponent)
        static void Reflect(AZ::ReflectContext* context);

        AxisAlignedBoxShapeDebugDisplayComponent() = default;

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        // EntityDebugDisplayComponent
        void Draw(AzFramework::DebugDisplayRequests& debugDisplay) override;

    private:
        AZ_DISABLE_COPY_MOVE(AxisAlignedBoxShapeDebugDisplayComponent)

        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeChangeReasons changeReason) override;

        BoxShapeConfig m_boxShapeConfig; ///< Stores configuration data for box shape.
        AZ::Vector3 m_nonUniformScale = AZ::Vector3::CreateOne(); ///< Caches non-uniform scale for this entity.
    };
} // namespace LmbrCentral
