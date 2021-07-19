/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <IAudioInterfacesCommonData.h>

#include <IAudioSystemControl.h>

namespace AudioControls
{
    enum EWwiseControlTypes
    {
        eWCT_INVALID = 0,
        eWCT_WWISE_EVENT = AUDIO_BIT(0),
        eWCT_WWISE_RTPC = AUDIO_BIT(1),
        eWCT_WWISE_SWITCH = AUDIO_BIT(2),
        eWCT_WWISE_AUX_BUS = AUDIO_BIT(3),
        eWCT_WWISE_SOUND_BANK = AUDIO_BIT(4),
        eWCT_WWISE_GAME_STATE = AUDIO_BIT(5),
        eWCT_WWISE_SWITCH_GROUP = AUDIO_BIT(6),
        eWCT_WWISE_GAME_STATE_GROUP = AUDIO_BIT(7),
    };

    //-------------------------------------------------------------------------------------------//
    class IAudioSystemControl_wwise
        : public IAudioSystemControl
    {
    public:
        IAudioSystemControl_wwise() {}
        IAudioSystemControl_wwise(const AZStd::string& name, CID id, TImplControlType type);
        ~IAudioSystemControl_wwise() override {}
    };
} // namespace AudioControls
