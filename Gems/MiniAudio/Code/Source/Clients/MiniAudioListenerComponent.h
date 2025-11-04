/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>
#include <MiniAudio/MiniAudioConstants.h>
#include <MiniAudio/MiniAudioListenerBus.h>

#include "MiniAudioListenerComponentController.h"
#include "MiniAudioListenerComponentConfig.h"

namespace MiniAudio
{
    class MiniAudioListenerComponent final
        : public AzFramework::Components::ComponentAdapter<MiniAudioListenerComponentController, MiniAudioListenerComponentConfig>
    {
        using BaseClass = AzFramework::Components::ComponentAdapter<MiniAudioListenerComponentController, MiniAudioListenerComponentConfig>;
    public:
        AZ_COMPONENT(MiniAudioListenerComponent, MiniAudioListenerComponentTypeId, BaseClass);
        MiniAudioListenerComponent() = default;
        explicit MiniAudioListenerComponent(const MiniAudioListenerComponentConfig& config);

        static void Reflect(AZ::ReflectContext* context);
    };
} // namespace MiniAudio
