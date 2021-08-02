/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

//
// Use this plugin registration helpers header to auto-register plugin libraries.
// This will give a standard set of plugins, check <AK/Plugin/AllPluginFactories.h> for what it includes.
//
#include <AK/Plugin/AllPluginsRegistrationHelpers.h>

//
// Prior to finalization of a game, it is recommended that you include only the plugin headers used by the game.
// Third party plugins and/or plugins not included in <AK/Plugin/AllPluginFactories.h> should be added below.
//
// For example:
//
// #include <AK/Plugin/AkConvolutionReverbFXFactory.h>
// #include <AK/Plugin/AkReflectFXFactory.h>
// ...
