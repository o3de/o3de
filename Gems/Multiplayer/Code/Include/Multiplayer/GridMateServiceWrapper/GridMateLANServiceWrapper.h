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
#pragma once

#include "Multiplayer/GridMateServiceWrapper/GridMateServiceWrapper.h"

namespace Multiplayer
{
    class GridMateLANServiceWrapper
        : public GridMateServiceWrapper
    {
    public:
        AZ_CLASS_ALLOCATOR(GridMateLANServiceWrapper, AZ::SystemAllocator, 0);
        
        bool SanityCheck(GridMate::IGridMate* gridMate) override;
        bool StartSessionService(GridMate::IGridMate* gridMate) override;
        void StopSessionService(GridMate::IGridMate* gridMate) override;
        
    protected:
        GridMate::GridSession* CreateServerForService(GridMate::IGridMate* gridMate, GridMate::CarrierDesc& carrierDesc, const GridMateServiceParams& params) override;
        GridMate::GridSearch* ListServersForService(GridMate::IGridMate* gridMate, const GridMateServiceParams& params) override;
        GridMate::GridSession* JoinSessionForService(GridMate::IGridMate* gridMate, GridMate::CarrierDesc& carrierDesc, const GridMate::SearchInfo* searchInfo) override;
        
    private:
        int GetServerPort(const GridMateServiceParams& params) const;
    };
}
