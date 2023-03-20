/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/optional.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/string/string.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/PhysicsSetup.h>
#include <EMotionFX/CommandSystem/Source/ParameterMixins.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>
#include <EMotionFX/Source/EMotionFXConfig.h>


namespace AZ
{
    class ReflectContext;
}

// Due to the SimulatedObjectCommands implementation being directly included into EMotionFX simulated object test CommandAdjustSimulatedObjectTests.cpp
// the interface has been separated from the #include directives to allow it to be directly
// included without polluting the test namespace with non-EMotionFX names
#include "SimulatedObjectCommands_Interface.inl"
