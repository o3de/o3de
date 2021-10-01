/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestEngine/Common/TestImpactErrorCodeChecker.h>

namespace TestImpact
{
    ErrorCodeChecker::ErrorCodeChecker(AZStd::vector<ErrorCodeHandler>&& handlers)
        : m_handlers(AZStd::move(handlers))
    {
    }

    ErrorCodeChecker::ErrorCodeChecker(const AZStd::vector<ErrorCodeHandler>& handlers)
        : m_handlers(handlers)
    {
    }

    AZStd::optional<Client::TestRunResult> ErrorCodeChecker::CheckErrorCode(ReturnCode returnCode) const
    {
        for (const auto& handler : m_handlers)
        {
            if (const auto result = handler(returnCode); result != AZStd::nullopt)
            {
                return result;
            }
        }

        return AZStd::nullopt;
    }
} // namespace TestImpact
