/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/string/conversions.h>

#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>
#include <EMotionFX/Source/Skeleton.h>

 // The implementation is included directly into EMotionFX Test cpp files
 // so it has move to an .inl file with no #include directives
#include "SimulatedObjectSetup_Impl.inl"
