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
