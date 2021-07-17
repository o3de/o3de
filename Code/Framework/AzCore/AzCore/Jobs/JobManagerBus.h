/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_JOB_MANAGER_BUS_H
#define AZCORE_JOB_MANAGER_BUS_H

#include <AzCore/EBus/EBus.h>

namespace AZ
{
    class JobManager;
    class JobContext;
    /**
     * Event for communication with the job manager component. There can
     * be only one job manager at a time. This is why this but is set to support
     * only one client/listener.
     */
    class JobManagerEvents
        : public AZ::EBusTraits
    {
    public:
        virtual ~JobManagerEvents() {}

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides - application is a singleton
        static const AZ::EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;  // we sort components on m_initOrder
        //////////////////////////////////////////////////////////////////////////

        virtual JobManager* GetManager() = 0;
        virtual JobContext* GetGlobalContext() = 0;
    };

    typedef AZ::EBus<JobManagerEvents>  JobManagerBus;
}

#endif // AZCORE_JOB_MANAGER_BUS_H
#pragma once
