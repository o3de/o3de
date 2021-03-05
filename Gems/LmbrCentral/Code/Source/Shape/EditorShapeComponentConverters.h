/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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