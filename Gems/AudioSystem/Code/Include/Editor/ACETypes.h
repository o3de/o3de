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

#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>

namespace AudioControls
{
    enum EACEControlType
    {
        eACET_TRIGGER = 0,
        eACET_RTPC,
        eACET_SWITCH,
        eACET_SWITCH_STATE,
        eACET_ENVIRONMENT,
        eACET_PRELOAD,
        eACET_NUM_TYPES
    };

    using TImplControlType = unsigned int;
    using CID = unsigned int;
    using ControlList = AZStd::vector<CID>;

    class IAudioConnection;
    using TConnectionPtr = AZStd::shared_ptr<IAudioConnection>;

    static const CID ACE_INVALID_CID = 0;
    static const TImplControlType AUDIO_IMPL_INVALID_TYPE = 0;

    using FilepathSet = AZStd::set<AZStd::string>;

} // namespace AudioControls
