/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/optional.h>
#include <MCore/Source/Command.h>
#include <EMotionFX/Source/AnimGraphObjectIds.h>


namespace AZ
{
    class ReflectContext;
}


// Due to the ParameterMixins implementation being directly included into EMotionFX CommandAdjustSimulatedObjectTests.cpp
// the interface has been separated from the #include directives to allow it to be directly
// included without polluting the test namespace with non-EMotionFX names
#include "ParameterMixins_Interface.inl"
