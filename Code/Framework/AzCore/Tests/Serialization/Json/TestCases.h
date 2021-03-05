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

#pragma once

#include <AzCore/UnitTest/TestTypes.h>
#include <Tests/Serialization/Json/TestCases_Base.h>
#include <Tests/Serialization/Json/TestCases_Classes.h>
#include <Tests/Serialization/Json/TestCases_Pointers.h>

namespace JsonSerializationTests
{
    using JsonSerializationTestCases = ::testing::Types<
        // Structures
        SimpleClass, SimpleInheritence, MultipleInheritence, SimpleNested, SimpleEnumWrapper,
        // Pointers
        SimpleNullPointer, SimpleAssignedPointer, ComplexAssignedPointer, ComplexNullInheritedPointer,
        ComplexAssignedDifferentInheritedPointer, ComplexAssignedSameInheritedPointer,
        PrimitivePointerInContainer, SimplePointerInContainer, InheritedPointerInContainer>;
} // namespace JsonSerializationTests
