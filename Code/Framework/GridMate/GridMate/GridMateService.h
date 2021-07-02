/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GRIDMATESERVICE_H
#define GRIDMATESERVICE_H

#include <GridMate/Types.h>

#define GRIDMATE_SERVICE_ID(GridMateService) static GridMate::GridMateServiceId GetGridMateServiceId()  { return AZ::Crc32(#GridMateService); }

namespace GridMate
{
    class IGridMate;

    /*
    * Generic GridMate service interface
    * All services should implement this interface
    */
    class GridMateService
    {
    public:
        virtual ~GridMateService() {}

        // Called when service is bound to GridMate instance
        virtual void OnServiceRegistered(IGridMate* gridMate) = 0;

        // Called when service is unregistered from given GridMate instance
        virtual void OnServiceUnregistered(IGridMate* gridMate) = 0;

        // Called on GridMate tick
        virtual void OnGridMateUpdate(IGridMate* gridMate) { (void)gridMate; }
    };
}

#endif
