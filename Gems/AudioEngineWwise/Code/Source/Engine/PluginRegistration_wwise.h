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
