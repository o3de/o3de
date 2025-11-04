/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace LmbrCentral
{
    //! Type ID for QuadShapeComponent
    inline constexpr AZ::TypeId QuadShapeComponentTypeId{ "{A2205305-1087-4D34-A23F-2A68D6CA333A}" };

    //! Type ID for EditorQuadShapeComponent
    inline constexpr AZ::TypeId EditorQuadShapeComponentTypeId{ "{E8E60770-40E9-426F-B134-3964BF8BDD84}" };

    //! Configuration data for QuadShapeComponent
    class QuadShapeConfig
        : public ShapeComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(QuadShapeConfig, AZ::SystemAllocator);
        AZ_RTTI(LmbrCentral::QuadShapeConfig, "{35CA7415-DB12-4630-B0D0-4A140CE1B9A7}", ShapeComponentConfig);

        static void Reflect(AZ::ReflectContext* context);

        QuadShapeConfig() = default;
        QuadShapeConfig(float width, float height) : m_width(width), m_height(height) {}

        AZStd::array<AZ::Vector3, 4> GetCorners() const
        {
            float halfWidth = m_width * 0.5f;
            float halfHeight = m_height * 0.5f;
            return
            { {
                 AZ::Vector3(-halfWidth,  halfHeight, 0.0f),
                 AZ::Vector3(halfWidth,  halfHeight, 0.0f),
                 AZ::Vector3(halfWidth, -halfHeight, 0.0f),
                 AZ::Vector3(-halfWidth, -halfHeight, 0.0f)
            } };
        };

        float m_width = 1.0f;
        float m_height = 1.0f;
    };

    //! Services provided by the Quad Shape Component.
    class QuadShapeComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual const QuadShapeConfig& GetQuadConfiguration() const = 0;

        //! @brief Returns the width for the quad shape component.
        virtual float GetQuadWidth() const = 0;

        //! @brief Sets the width for the quad shape component.
        //! @param width New width of the quad shape.
        virtual void SetQuadWidth(float width) = 0;

        //! @brief Returns the height for the quad shape component.
        virtual float GetQuadHeight() const = 0;

        //! @brief Sets the height for the quad shape component.
        //! @param width New height of the quad shape.
        virtual void SetQuadHeight(float height) = 0;

        //! @brief Convenience function that returns the orientation as a quaternion for the quad determined by the transform component.
        virtual const AZ::Quaternion& GetQuadOrientation() const = 0;

    };

    // Bus to service the Quad Shape component event group
    using QuadShapeComponentRequestBus = AZ::EBus<QuadShapeComponentRequests>;

} // namespace LmbrCentral
