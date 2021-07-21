/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/UnitTest/TestTypes.h>
#include <Tests/Serialization/Json/TestCases_Base.h>
#include <Tests/Serialization/Json/TestCases_Classes.h>
#include <Tests/Serialization/Json/TestCases_Pointers.h>

namespace JsonSerializationTests
{
    using JsonSerializationTestCases = ::testing::Types<
        // Structures
        SimpleClass, SimpleInheritence, MultipleInheritence, SimpleNested, SimpleEnumWrapper, NonReflectedEnumWrapper,
        // Pointers
        SimpleNullPointer, SimpleAssignedPointer, ComplexAssignedPointer, ComplexNullInheritedPointer,
        ComplexAssignedDifferentInheritedPointer, ComplexAssignedSameInheritedPointer,
        PrimitivePointerInContainer, SimplePointerInContainer, InheritedPointerInContainer>;
} // namespace JsonSerializationTests
