/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/XML/rapidxml.h>

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

    using XmlAllocator = AZ::rapidxml::memory_pool<>;
    inline XmlAllocator s_xmlAllocator;

} // namespace AudioControls
