/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
