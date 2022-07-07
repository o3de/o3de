/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <gmock/gmock.h>
#include <AzCore/std/string/string.h>

inline testing::PolymorphicMatcher<testing::internal::StrEqualityMatcher<AZStd::string>> StrEq(const AZStd::string& str)
{
    return ::testing::MakePolymorphicMatcher(testing::internal::StrEqualityMatcher<AZStd::string>(str, true, true));
}
