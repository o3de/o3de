/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/CommandSystem/Source/AnimGraphParameterCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConnectionCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>

#include <AzCore/std/sort.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphParameterCondition.h>
#include <EMotionFX/Source/AnimGraphTagCondition.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/Parameter/Parameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <EMotionFX/Source/Parameter/ValueParameter.h>
#include <MCore/Source/AttributeFactory.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/ReflectionSerializer.h>

 // The implementation is included directly into EMotionFX Test cpp files
  // so it has move to an .inl file with no #include directives
#include "AnimGraphParameterCommands_Impl.inl"
