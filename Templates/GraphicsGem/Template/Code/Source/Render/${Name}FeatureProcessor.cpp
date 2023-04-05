/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "${Name}FeatureProcessor.h"

namespace ${Name}
{
    void ${Name}FeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext
                ->Class<${Name}FeatureProcessor, FeatureProcessor>()
                ;
        }
    }

    void ${Name}FeatureProcessor::Activate()
    {

    }

    void ${Name}FeatureProcessor::Deactivate()
    {
        
    }

    void ${Name}FeatureProcessor::Simulate([[maybe_unused]] const FeatureProcessor::SimulatePacket& packet)
    {
        
    }    
}
