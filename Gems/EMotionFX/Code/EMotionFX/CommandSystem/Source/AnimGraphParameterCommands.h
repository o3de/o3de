/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionFX/CommandSystem/Source/CommandSystemConfig.h>
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/StringIdPool.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>


 // Due to the AnimGraphParameterCommands implementation being directly included into EMotionFX AnimGraphParameterCommandsTests.cpp
 // the interface has been separated from the #include directives to allow it to be directly
 // included without polluting the test namespace with non-EMotionFX names
#include "AnimGraphParameterCommands_Interface.inl"
