/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/SerializeContext.h>

namespace LmbrCentral
{
    namespace ClassConverters
    {
        /// Common convert function to move shape config to shape type.
        template <typename Shape, typename ShapeConfig>
        bool UpgradeShapeComponentConfigToShape(
            AZ::u32 version, const char* shapeName, AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

        /// ShapeComponent converters
        bool UpgradeBoxShapeComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        bool UpgradeSphereShapeComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        bool UpgradeCapsuleShapeComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        bool UpgradeCylinderShapeComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

    } // namespace ClassConverters
} // namespace LmbrCentral

#include "ShapeComponentConverters.inl"
