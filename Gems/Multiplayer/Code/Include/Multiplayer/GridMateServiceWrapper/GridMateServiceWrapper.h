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

#include <AzCore/Memory/SystemAllocator.h>
#include <GridMate/Session/Session.h>

namespace Multiplayer
{   
    namespace Convert
    {
        template <typename T>
        T GridSessionParam(const GridMate::GridSessionParam& param, T orDefault)
        {
            static_assert(AZStd::is_pod<T>::value == false, "ConvertGridSessionParam requires a conversion specialization.");
            return orDefault;
        }

        template <>
        int GridSessionParam(const GridMate::GridSessionParam& param, int orDefault);

        template <>
        float GridSessionParam(const GridMate::GridSessionParam& param, float orDefault);

        template <>
        long long GridSessionParam(const GridMate::GridSessionParam& param, long long orDefault);

        template <>
        double GridSessionParam(const GridMate::GridSessionParam& param, double orDefault);
    }

    struct GridMateServiceParams
    {
        explicit GridMateServiceParams(const GridMate::SessionParams& sessionParams, AZStd::function<GridMate::GridSessionParam(const char*)> cb)
            : m_sessionParams(sessionParams)
            , m_version(1)
            , m_fetchSessionParam(cb)
        {}

        void AssignSessionParams(GridMate::SessionParams& other) const
        {
            other = m_sessionParams;
        }

        GridMate::string FetchString(const char* varName) const
        {
            if (m_fetchSessionParam)
            {
                GridMate::GridSessionParam p = m_fetchSessionParam(varName);
                if (p.m_type == GridMate::GridSessionParam::VT_STRING)
                {
                    return AZStd::move(p.m_value);
                }
            }
            return GridMate::string();
        }

        template <typename T>
        T FetchValueOrDefault(const char* varName, T orDefault) const
        {
            if (m_fetchSessionParam)
            {
                return Convert::GridSessionParam<T>(m_fetchSessionParam(varName), orDefault);
            }
            return orDefault;
        }

        const GridMate::SessionParams& m_sessionParams;
        GridMate::VersionType m_version;
        AZStd::function<GridMate::GridSessionParam(const char*)> m_fetchSessionParam;
    };

    class GridMateServiceWrapper
    {
    public:
        AZ_CLASS_ALLOCATOR(GridMateServiceWrapper, AZ::SystemAllocator, 0);
        
    public:
        virtual ~GridMateServiceWrapper() = default;
        
        virtual bool SanityCheck(GridMate::IGridMate* gridMate) = 0;
        virtual bool StartSessionService(GridMate::IGridMate* gridMate) = 0;
        virtual void StopSessionService(GridMate::IGridMate* gridMate) = 0;
        
        GridMate::GridSession* CreateServer(GridMate::IGridMate* gridMate, GridMate::CarrierDesc& carrierDesc, const GridMateServiceParams& params);
        GridMate::GridSearch* ListServers(GridMate::IGridMate* gridMate, const GridMateServiceParams& params);
        GridMate::GridSession* JoinSession(GridMate::IGridMate* gridMate, GridMate::CarrierDesc& carrierDesc, const GridMate::SearchInfo* searchInfo);       
        
    protected:
        virtual GridMate::GridSession* CreateServerForService(GridMate::IGridMate* gridMate, GridMate::CarrierDesc& carrierDesc, const GridMateServiceParams& params) = 0;
        virtual GridMate::GridSearch* ListServersForService(GridMate::IGridMate* gridMate, const GridMateServiceParams& params) = 0;
        virtual GridMate::GridSession* JoinSessionForService(GridMate::IGridMate* gridMate, GridMate::CarrierDesc& carrierDesc, const GridMate::SearchInfo* searchInfo) = 0;
       
    };
}
