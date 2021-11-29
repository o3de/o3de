/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// On some platforms, there is an issue with environment variables that are removed before some objects are deallocated (during the process of 
// ComponentApplication::Destroy). Until all of the shutdown issues are solved, the following trait will skip the parent ::Destroy() and exit 
// the application as soon as possible if set to true.
// (Tracked by GHI - 4806)
#define AZ_TRAIT_ATOMTOOLSFRAMEWORK_SKIP_APP_DESTROY true

