/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Configuration.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace AZ
{
    namespace RPI
    {
        //! struct FeatureProcessorDescriptor
        //! brief A collection of data describing how a FeatureProcessor should be set up
        struct ATOM_RPI_REFLECT_API FeatureProcessorDescriptor
        {
            AZ_TYPE_INFO(FeatureProcessorDescriptor, "{3A7FFA35-9D92-4AAC-BA2B-0A90268F563C}");
            static void Reflect(AZ::ReflectContext* context);

            uint32_t m_maxRenderGraphLatency = 1;
        };
    } // namespace RPI
} // namespace AZ
