/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Target/Common/TestImpactTarget.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    //! Build target specialization for production targets (build targets containing production code and no test code).
    class NativeProductionTarget
        : public Target
    {
    public:
        using Target::Target;
    };
} // namespace TestImpact
