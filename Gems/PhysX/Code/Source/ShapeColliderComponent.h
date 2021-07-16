/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <BaseColliderComponent.h>

namespace LmbrCentral
{
    class CapsuleShapeConfig;
} // namespace LmbrCentral

namespace PhysX
{
    namespace Utils
    {
        Physics::CapsuleShapeConfiguration ConvertFromLmbrCentralCapsuleConfig(
            const LmbrCentral::CapsuleShapeConfig& inputCapsuleConfig);
    } // namespace Utils

    namespace ShapeConstants
    {
        static const AZ::Crc32 Box = AZ_CRC("Box", 0x08a9483a);
        static const AZ::Crc32 Capsule = AZ_CRC("Capsule", 0xc268a183);
        static const AZ::Crc32 Sphere = AZ_CRC("Sphere", 0x55f96687);
        static const AZ::Crc32 PolygonPrism = AZ_CRC("PolygonPrism", 0xd6b50036);
        static const AZ::Crc32 Cylinder = AZ_CRC("Cylinder", 0x9b045bea);
    } // namespace ShapeConstants

    /// Component that provides a collider based on geometry from a shape component.
    class ShapeColliderComponent
        : public BaseColliderComponent
    {
    public:
        AZ_COMPONENT(ShapeColliderComponent, "{30CC9E77-378C-49DF-9617-6BF191901FE0}", BaseColliderComponent);
        static void Reflect(AZ::ReflectContext* context);

        ShapeColliderComponent() = default;

        // BaseColliderComponent
        void UpdateScaleForShapeConfigs() override;
    };
} // namespace PhysX
