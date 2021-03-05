/**
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
