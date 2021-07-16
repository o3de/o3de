/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestEngine/Enumeration/TestImpactTestEnumeration.h>

#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //! Serializes the specified test enumeration to JSON format.
    AZStd::string SerializeTestEnumeration(const TestEnumeration& testEnumeration);

    //! Deserializes a test enumeration from the specified test enumeration data in JSON format.
    TestEnumeration DeserializeTestEnumeration(const AZStd::string& testEnumerationString);
} // namespace TestImpact
