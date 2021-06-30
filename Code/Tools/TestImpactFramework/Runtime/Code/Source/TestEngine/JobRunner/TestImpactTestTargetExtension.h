/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>

namespace TestImpact
{
    class TestTarget;

    //! Returns the binary file extension for the specified test target.
    AZStd::string GetTestTargetExtension(const TestTarget* testTarget);
}
