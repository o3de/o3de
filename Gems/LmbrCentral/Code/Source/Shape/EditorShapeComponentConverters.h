/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        /// EditorSphereShapeComponent converters
        bool DeprecateEditorSphereColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        bool UpgradeEditorSphereShapeComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

        /// EditorBoxShapeComponent converters
        bool DeprecateEditorBoxColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        bool UpgradeEditorBoxShapeComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        bool UpgradeBoxShapeComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

        /// EditorCylinderShapeComponent converters
        bool DeprecateEditorCylinderColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        bool UpgradeEditorCylinderShapeComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

        /// EditorCapsuleShapeComponent converters
        bool DeprecateEditorCapsuleColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        bool UpgradeEditorCapsuleShapeComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

        /// EditorPolygonPrismShapeComponent converters
        bool UpgradeEditorPolygonPrismShapeComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    } // namespace ClassConverters
} // namespace LmbrCentral
