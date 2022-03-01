/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>

namespace ScriptCanvas::Nodeables::Spawning
{
    //! Wrapper around EntitySpawnTicket that can be used by ScriptCanvas
    struct SpawnTicketInstance final
    {
        AZ_RTTI(SpawnTicketInstance, "{2B5EB938-8962-4A43-A97B-112F398C604B}");
        static void Reflect(AZ::ReflectContext* context);

        AZStd::shared_ptr<AzFramework::EntitySpawnTicket> m_ticket;
    };
}
