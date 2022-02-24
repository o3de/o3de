/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TestImpactTestProcess.h"

int main(int argc, char* argv[])
{
    TestImpact::TestProcess process(argc, argv);
    return process.MainFunc();
}
