/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/AzFrameworkAPI.h>

namespace Physics
{
    namespace ClassConverters
    {
        AZF_API bool RagdollNodeConfigConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        AZF_API bool RagdollConfigConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        AZF_API bool ColliderConfigurationConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        
    } // namespace ClassConverters
} // namespace Physics
