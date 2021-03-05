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
#include "Multiplayer_precompiled.h"

#include "Multiplayer/GridMateServiceWrapper/GridMateServiceWrapper.h"
#include <AzCore/std/string/conversions.h>

namespace Multiplayer
{
    namespace Convert
    {
        template <>
        int GridSessionParam(const GridMate::GridSessionParam& param, int orDefault)
        {
            if (param.m_type != GridMate::GridSessionParam::VT_INT32)
            {
                return orDefault;
            }
            return AZStd::stoi(param.m_value);
        }

        template <>
        float GridSessionParam(const GridMate::GridSessionParam& param, float orDefault)
        {
            if (param.m_type != GridMate::GridSessionParam::VT_FLOAT)
            {
                return orDefault;
            }
            return AZStd::stof(param.m_value);
        }

        template <>
        long long GridSessionParam(const GridMate::GridSessionParam& param, long long orDefault)
        {
            if (param.m_type != GridMate::GridSessionParam::VT_INT64)
            {
                return orDefault;
            }
            return AZStd::stoll(param.m_value);
        }

        template <>
        double GridSessionParam(const GridMate::GridSessionParam& param, double orDefault)
        {
            if (param.m_type != GridMate::GridSessionParam::VT_DOUBLE)
            {
                return orDefault;
            }
            return AZStd::stod(param.m_value);
        }
    }

    GridMate::GridSession* GridMateServiceWrapper::CreateServer(GridMate::IGridMate* gridMate, GridMate::CarrierDesc& carrierDesc, const GridMateServiceParams& params)
    {
        GridMate::GridSession* gridSession = nullptr;
        if (StartSessionService(gridMate) && SanityCheck(gridMate))
        {
            gridSession = CreateServerForService(gridMate,carrierDesc, params);
        }
        
        return gridSession;
    }
    
    GridMate::GridSearch* GridMateServiceWrapper::ListServers(GridMate::IGridMate* gridMate, const GridMateServiceParams& params)
    {
        GridMate::GridSearch* gridSearch = nullptr;
        
        if (StartSessionService(gridMate) && SanityCheck(gridMate))
        {
            gridSearch = ListServersForService(gridMate, params);
        }
        
        return gridSearch;
    }
    
    GridMate::GridSession* GridMateServiceWrapper::JoinSession(GridMate::IGridMate* gridMate, GridMate::CarrierDesc& carrierDesc, const GridMate::SearchInfo* searchInfo)
    {
        GridMate::GridSession* gridSession = nullptr;
        
        if (StartSessionService(gridMate) && SanityCheck(gridMate))
        {
            gridSession = JoinSessionForService(gridMate, carrierDesc, searchInfo);
        }
        
        return gridSession;
    }
}