/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/SimulatedObjectCommands.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/SimulatedObjectBus.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/ReflectionSerializer.h>

// The implementation is included directly into EMotionFX Test cpp files
// so it has move to an .inl file with no #include directives
#include "SimulatedObjectCommands_Impl.inl"
