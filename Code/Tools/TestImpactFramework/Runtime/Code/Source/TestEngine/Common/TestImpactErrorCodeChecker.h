/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactClientTestRun.h>

#include <Process/TestImpactProcessInfo.h>

#include <AzCore/std/optional.h>
#include <AzCore/std/functional.h>

namespace TestImpact
{
    //!
    using ErrorCodeHandler = AZStd::function<AZStd::optional<Client::TestRunResult>(ReturnCode returnCode)>;

    //!
    class ErrorCodeChecker
    {
    public:
        ErrorCodeChecker(AZStd::vector<ErrorCodeHandler>&& handlers);
        ErrorCodeChecker(const AZStd::vector<ErrorCodeHandler>& handlers);

        AZStd::optional<Client::TestRunResult> CheckErrorCode(ReturnCode returnCode) const;
    private:
        AZStd::vector<ErrorCodeHandler> m_handlers;
    };
} // namespace TestImpact
