/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>

 // Due to the SimulatedObjectSetup implementation being directly included into EMotionFX SimulatedObjectSetupsTests.cpp
 // the interface has been separated from the #include directives to allow it to be directly
 // included without polluting the test namespace with non-EMotionFX names
#include "SimulatedObjectSetup_Interface.inl"
