/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <AzCore/std/string/string.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/optional.h>

#pragma once

namespace TestImpact
{
    class FailureReport
    {
    public:
        const AZStd::string& GetTargetName() const;
    };

    class ExecutionFailureReport
        : public FailureReport
    {
    public:
        const AZStd::string& GetCommand() const;
    };

    class TestFailureReport
        : FailureReport
    {};
}
