/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <Atom/RPI.Public/FeatureProcessor.h>

namespace GraphicsGem_AR_Test
{
    class GraphicsGem_AR_Test;

    using GraphicsGem_AR_TestHandle = AZStd::shared_ptr<GraphicsGem_AR_Test>;

    // GraphicsGem_AR_TestFeatureProcessorInterface provides an interface to the feature processor for code outside of Atom
    class GraphicsGem_AR_TestFeatureProcessorInterface
        : public AZ::RPI::FeatureProcessor
    {
    public:
        AZ_RTTI(GraphicsGem_AR_TestFeatureProcessorInterface, "{D953101A-E1AD-4A6A-BAF3-2DCBA69F340A}", AZ::RPI::FeatureProcessor);

    };
}
