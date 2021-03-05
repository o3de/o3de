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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once
#include "ResourceSelector.h"

namespace Serialization
{
    template<class T>
    ResourceSelector<T> AudioTrigger(T& s) { return ResourceSelector<T>(s, "AudioTrigger"); }
    template<class T>
    ResourceSelector<T> AudioSwitch(T& s) { return ResourceSelector<T>(s, "AudioSwitch"); }
    template<class T>
    ResourceSelector<T> AudioSwitchState(T& s) { return ResourceSelector<T>(s, "AudioSwitchState"); }
    template<class T>
    ResourceSelector<T> AudioRTPC(T& s) { return ResourceSelector<T>(s, "AudioRTPC"); }
    template<class T>
    ResourceSelector<T> AudioEnvironment(T& s) { return ResourceSelector<T>(s, "AudioEnvironment"); }
    template<class T>
    ResourceSelector<T> AudioPreloadRequest(T& s) { return ResourceSelector<T>(s, "AudioPreloadRequest"); }
};
