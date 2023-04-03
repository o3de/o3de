/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GraphicsGem_AR_TestFeatureProcessor.h"

namespace GraphicsGem_AR_Test
{
    void GraphicsGem_AR_TestFeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext
                ->Class<GraphicsGem_AR_TestFeatureProcessor, FeatureProcessor>()
                ;
        }
    }

    void GraphicsGem_AR_TestFeatureProcessor::Activate()
    {

    }

    void GraphicsGem_AR_TestFeatureProcessor::Deactivate()
    {
        
    }

    void GraphicsGem_AR_TestFeatureProcessor::Simulate([[maybe_unused]] const FeatureProcessor::SimulatePacket& packet)
    {
        
    }    
}