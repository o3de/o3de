/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "AzCore/Math/MathUtils.h"
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/EditContext.h>

namespace MiniAudio
{
    class MiniAudioListenerComponentConfig final
        : public AZ::ComponentConfig
    {
    public:
        AZ_RTTI(MiniAudioListenerComponentConfig, "{7987E444-3A98-469C-B38B-EDD9C247D7F1}");

        static void Reflect(AZ::ReflectContext* context);

        //! Listener follows the specified entity.
        AZ::EntityId m_followEntity;

        AZ::u32 m_listenerIndex = 0;

        //! Global volume
        float m_globalVolume = 100.0f;
        //! Inner cone angle
        float m_innerAngleInRadians = 3.f/5.f * AZ::Constants::TwoPi;
        float m_innerAngleInDegrees = AZ::RadToDeg(m_innerAngleInRadians);
        //! Outer cone angle
        float m_outerAngleInRadians = 1.5f * AZ::Constants::Pi;
        float m_outerAngleInDegrees = AZ::RadToDeg(m_outerAngleInRadians);
        //! Volume outside of outer cone
        float m_outerVolume = 50.0f;
    };
} // namespace MiniAudio
