/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AudioSystemControl_wwise.h>

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    IAudioSystemControl_wwise::IAudioSystemControl_wwise(const AZStd::string& name, CID id, TImplControlType type)
        : IAudioSystemControl(name, id, type)
    {
    }
} // namespace AudioControls
