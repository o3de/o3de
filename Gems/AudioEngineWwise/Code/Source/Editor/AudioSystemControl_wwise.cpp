/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
