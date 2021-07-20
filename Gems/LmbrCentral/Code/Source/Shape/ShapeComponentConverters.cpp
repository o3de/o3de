/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ShapeComponentConverters.h"

#include <Shape/BoxShape.h>
#include <Shape/SphereShape.h>
#include <Shape/CapsuleShape.h>
#include <Shape/CylinderShape.h>
#include <AzCore/Math/PolygonPrism.h>

namespace LmbrCentral
{
    namespace ClassConverters
    {
        bool UpgradeBoxShapeComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // upgrade for editor and runtime components at this stage are the same
            return UpgradeShapeComponentConfigToShape<BoxShape, BoxShapeConfig>(
                classElement.GetVersion(), "BoxShape", context, classElement);
        }

        bool UpgradeSphereShapeComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // upgrade for editor and runtime components at this stage are the same
            return UpgradeShapeComponentConfigToShape<SphereShape, SphereShapeConfig>(
                classElement.GetVersion(), "SphereShape", context, classElement);
        }

        bool UpgradeCapsuleShapeComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // upgrade for editor and runtime components at this stage are the same
            return UpgradeShapeComponentConfigToShape<CapsuleShape, CapsuleShapeConfig>(
                classElement.GetVersion(), "CapsuleShape", context, classElement);
        }

        bool UpgradeCylinderShapeComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // upgrade for editor and runtime components at this stage are the same
            return UpgradeShapeComponentConfigToShape<CylinderShape, CylinderShapeConfig>(
                classElement.GetVersion(), "CylinderShape", context, classElement);
        }
    }
}
