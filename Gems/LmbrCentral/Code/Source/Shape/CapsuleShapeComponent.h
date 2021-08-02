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
#include "CapsuleShape.h"
#include "ShapeGeometryUtil.h"

namespace LmbrCentral
{
    /// Provide a Component interface for CapsuleShape functionality.
    class CapsuleShapeComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(CapsuleShapeComponent, CapsuleShapeComponentTypeId);
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

        CapsuleShape m_capsuleShape; ///< Stores underlying capsule type for this component.
    };

    /// Concrete EntityDebugDisplay implementation for CapsuleShape.
    class CapsuleShapeDebugDisplayComponent
        : public EntityDebugDisplayComponent
        , public ShapeComponentNotificationsBus::Handler
    {
    public:
        AZ_COMPONENT(CapsuleShapeDebugDisplayComponent, "{21A6A8CD-C0AC-477D-8574-556DB46CDD3B}", EntityDebugDisplayComponent)
        static void Reflect(AZ::ReflectContext* context);

        CapsuleShapeDebugDisplayComponent() = default;

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        // EntityDebugDisplayComponent
        void Draw(AzFramework::DebugDisplayRequests& debugDisplay) override;

    private:
        AZ_DISABLE_COPY_MOVE(CapsuleShapeDebugDisplayComponent)

        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeChangeReasons changeReason) override;

        void GenerateVertices();

        ShapeMesh m_capsuleShapeMesh; ///< Buffer to hold index and vertex data for CapsuleShape when drawing.
        CapsuleShapeConfig m_capsuleShapeConfig; ///< Stores configuration data for capsule shape.
    };
} // namespace LmbrCentral
