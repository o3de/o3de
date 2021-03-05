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

#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace Physics
{
    namespace ClassConverters
    {
        bool RagdollNodeConfigConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        bool RagdollConfigConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        bool MaterialLibraryAssetConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        bool ColliderConfigurationConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        bool MaterialSelectionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        bool RigidBodyVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    } // namespace ClassConverters
} // namespace Physics