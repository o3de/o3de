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

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace AZ
{
    namespace RPI
    {
        //! struct FeatureProcessorDescriptor
        //! brief A collection of data describing how a FeatureProcessor should be set up
        struct FeatureProcessorDescriptor
        {
            AZ_TYPE_INFO(FeatureProcessorDescriptor, "{3A7FFA35-9D92-4AAC-BA2B-0A90268F563C}");
            static void Reflect(AZ::ReflectContext* context);

            uint32_t m_maxRenderGraphLatency = 1;
        };
    } // namespace RPI
} // namespace AZ