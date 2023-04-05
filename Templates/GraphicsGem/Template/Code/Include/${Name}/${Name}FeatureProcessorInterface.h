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

namespace ${Name}
{
    class ${Name};

    using ${Name}Handle = AZStd::shared_ptr<${Name}>;

    // ${Name}FeatureProcessorInterface provides an interface to the feature processor for code outside of Atom
    class ${Name}FeatureProcessorInterface
        : public AZ::RPI::FeatureProcessor
    {
    public:
        AZ_RTTI(${Name}FeatureProcessorInterface, "{${Random_Uuid}}", AZ::RPI::FeatureProcessor);

    };
}
