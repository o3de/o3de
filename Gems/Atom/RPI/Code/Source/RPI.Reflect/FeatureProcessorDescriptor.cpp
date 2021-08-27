/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Reflect/FeatureProcessorDescriptor.h>

namespace AZ
{
    namespace RPI
    {
        /**
         * @brief Reflects the FeatureProcessorDescriptor struct to the Serialization system
         * @param[in] context The reflection context
         */
        void FeatureProcessorDescriptor::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<FeatureProcessorDescriptor>()
                    ->Version(0)
                    ->Field("MaxRenderGraphLatency", &FeatureProcessorDescriptor::m_maxRenderGraphLatency)
                ;
            }
        }
    } // namespace RPI
} // namespace AZ
