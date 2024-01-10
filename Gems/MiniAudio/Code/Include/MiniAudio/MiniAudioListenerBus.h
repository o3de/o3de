/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector3.h>

namespace MiniAudio
{
    class MiniAudioListenerRequests : public AZ::ComponentBus
    {
    public:
        ~MiniAudioListenerRequests() override = default;

        virtual void SetFollowEntity(AZ::EntityId followEntity) = 0;
        virtual void SetPosition(const AZ::Vector3& position) = 0;
    };

    using MiniAudioListenerRequestBus = AZ::EBus<MiniAudioListenerRequests>;

} // namespace MiniAudio
