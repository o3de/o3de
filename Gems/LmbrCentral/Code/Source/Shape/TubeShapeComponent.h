/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.EditorTubeShapeComponent
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
#include "Rendering/EntityDebugDisplayComponent.h"
#include "TubeShape.h"

namespace LmbrCentral
{
    /// Provide a Component interface for TubeShape functionality.
    class TubeShapeComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(TubeShapeComponent, TubeShapeComponentTypeId);
        static void Reflect(AZ::ReflectContext* context);

        TubeShapeComponent() = default;
        explicit TubeShapeComponent(const TubeShape& tubeShape);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        TubeShape m_tubeShape;
    };

    /// Concrete EntityDebugDisplay implementation for TubeShape.
    class TubeShapeDebugDisplayComponent
        : public EntityDebugDisplayComponent
        , public ShapeComponentNotificationsBus::Handler
    {
    public:
        AZ_COMPONENT(TubeShapeDebugDisplayComponent, "{FC8D0C5A-FEED-4C79-A4C6-E18A966EE8CE}", EntityDebugDisplayComponent)
        static void Reflect(AZ::ReflectContext* context);

        TubeShapeDebugDisplayComponent() = default;
        explicit TubeShapeDebugDisplayComponent(const TubeShapeMeshConfig& tubeShapeMeshConfig)
            : m_tubeShapeMeshConfig(tubeShapeMeshConfig) {}

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // EntityDebugDisplayComponent
        void Draw(AzFramework::DebugDisplayRequests& debugDisplay) override;

    private:
        AZ_DISABLE_COPY_MOVE(TubeShapeDebugDisplayComponent)

        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeChangeReasons changeReason) override;

        void GenerateVertices();

        ShapeMesh m_tubeShapeMesh; ///< Buffer to hold index and vertex data for the TubeShape when drawing.
        TubeShapeMeshConfig m_tubeShapeMeshConfig; ///< Configuration to control how the TubeShape should look.
        AZ::SplinePtr m_spline = nullptr; ///< Reference to the Spline.
        SplineAttribute<float> m_radiusAttribute; ///< Radius Attribute.
        float m_radius = 0.0f; ///< Global radius for the Tube.
    };
} // namespace LmbrCentral