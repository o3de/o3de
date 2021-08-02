/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Entity.h>
#include <AzToolsFramework/Entity/EditorEntityStartStatus.h>

namespace AzToolsFramework
{
    //! Editor Entity API Bus
    class EditorEntityAPIRequests
        : public AZ::EBusTraits
    {
    public:
        virtual ~EditorEntityAPIRequests() = default;

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void SetName(AZStd::string name) = 0;
        virtual void SetParent(AZ::EntityId parentId) = 0;
        virtual void SetLockState(bool isLocked) = 0;
        virtual void SetVisibilityState(bool isVisible) = 0;
        virtual void SetStartStatus(EditorEntityStartStatus status) = 0;
    };

    using EditorEntityAPIBus = AZ::EBus<EditorEntityAPIRequests>;
}
