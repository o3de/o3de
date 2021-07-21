/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>

namespace Maestro
{
    /**
    * Global Notifications from the Editor Sequences
    */
    class EditorSequenceNotification
        : public AZ::EBusTraits
    {
    public:

        /**
        * Called when a Sequence has been selected in Track View.
        */
        virtual void OnSequenceSelected([[maybe_unused]] const AZ::EntityId& sequenceEntityId) {}
    };

    using EditorSequenceNotificationBus = AZ::EBus<EditorSequenceNotification>;

} // namespace Maestro
