/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/Serialization/SerializeContext.h>

namespace PhysX
{
    namespace ClassConverters
    {
        bool DeprecateEditorCapsuleColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        bool DeprecateEditorBoxColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        bool DeprecateEditorSphereColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        bool DeprecateEditorMeshColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

        bool UpgradeEditorColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        bool EditorProxyShapeConfigVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        bool EditorRigidBodyConfigVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        bool EditorTerrainComponentConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    } // namespace ClassConverters
} // namespace PhysX
