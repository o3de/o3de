/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/UI/PropertyEditor/PropertyAudioCtrl.h>

namespace AudioControls
{
    class AudioControlSelectorHandler
        : public AzToolsFramework::AudioControlSelectorRequestBus::MultiHandler
    {
    public:
        AudioControlSelectorHandler();
        ~AudioControlSelectorHandler();
        AZStd::string SelectResource(AZStd::string_view previousValue) override;
    };
}
