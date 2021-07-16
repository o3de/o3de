/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/PostProcessing/SMAAFeatureProcessorInterface.h>

namespace AZ
{

    namespace Render
    {
        //! A descriptor used to configure the SMAA feature
        struct SMAAConfigurationDescriptor final
        {
            AZ_TYPE_INFO(SMAAConfigurationDescriptor, "{0A546684-7E31-4C00-874C-7DFB3D12A0A6}");
            static void Reflect(AZ::ReflectContext* context);

            //! Configuration name
            AZStd::string m_name;

            uint32_t m_enable;
            uint32_t m_edgeDetectionMode;
            uint32_t m_outputMode;
            uint32_t m_quality;
        };
    } // namespace Render
} // namespace AZ
