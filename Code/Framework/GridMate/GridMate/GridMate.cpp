/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Budget.h>
#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/std/hash.h>

#include <GridMate/GridMate.h>
#include <GridMate/GridMateService.h>
#include <GridMate/GridMateEventsBus.h>
#include <GridMate/Version.h>

AZ_DEFINE_BUDGET(GridMate);

namespace GridMate
{
    class GridMateImpl
        : public IGridMate
    {
    private:
        struct ServiceInfo
        {
            GridMateService* m_service;
            GridMateServiceId m_serviceId;
            bool m_isOwnService;
        };

        typedef AZStd::vector<ServiceInfo, GridMateStdAlloc> ServiceTable;

    public:
        AZ_CLASS_ALLOCATOR(GridMateImpl, GridMateAllocator, 0);

        GridMateImpl(const GridMateDesc& desc);
        ~GridMateImpl() override;

        void Update() override;

        EndianType GetDefaultEndianType() const override { return m_endianType; }

        void RegisterService(GridMateServiceId id, GridMateService* service, bool delegateOwnership = false) override;
        void UnregisterService(GridMateServiceId id) override;
        bool HasService(GridMateServiceId id) override;
        GridMateService* GetServiceById(GridMateServiceId id) override;

        EndianType m_endianType;

        ServiceTable m_services;

        struct StaticInfo
        {
            StaticInfo()
                : m_numGridMates(0)
                , m_gridMateAllocatorRefCount(0)
            { }

            int m_numGridMates;
            int m_gridMateAllocatorRefCount;
        };
        static StaticInfo   s_info;
    };
}

GridMate::GridMateImpl::StaticInfo GridMate::GridMateImpl::s_info;

using namespace GridMate;

//=========================================================================
// GridMateCreate
//=========================================================================
IGridMate* GridMate::GridMateCreate(const GridMateDesc& desc)
{
    // Memory
    if (AZ::AllocatorInstance<GridMateAllocator>::IsReady())
    {
        AZ_TracePrintf("GridMate", "GridMate Allocator has already started! Ignoring current allocator descriptor!\n");
        if (GridMateImpl::s_info.m_numGridMates == 0) // add ref count if we did not start it at all
        {
            GridMateImpl::s_info.m_gridMateAllocatorRefCount = 1;
        }
    }
    else
    {
        AZ::AllocatorInstance<GridMateAllocator>::Create(desc.m_allocatorDesc);
    }

    GridMateImpl::s_info.m_numGridMates++;
    GridMateImpl::s_info.m_gridMateAllocatorRefCount++;

    GridMateImpl* impl = aznew GridMateImpl(desc);
    EBUS_EVENT_ID(impl, GridMateEventsBus, OnGridMateInitialized, impl);
    return impl;
}

//=========================================================================
// GridMateCreate
//=========================================================================
void GridMate::GridMateDestroy(IGridMate* gridMate)
{
    AZ_Assert(gridMate != nullptr, "Invalid GridMate interface pointer!");
    EBUS_EVENT_ID(gridMate, GridMateEventsBus, OnGridMateShutdown, gridMate);

    delete gridMate;
    GridMateImpl::s_info.m_numGridMates--;
    GridMateImpl::s_info.m_gridMateAllocatorRefCount--;

    if (GridMateImpl::s_info.m_gridMateAllocatorRefCount == 0)
    {
        AZ::AllocatorInstance<GridMateAllocator>::Destroy();
    }

    if (GridMateImpl::s_info.m_numGridMates == 0)
    {
        GridMateImpl::s_info.m_gridMateAllocatorRefCount = 0;
    }
}

//=========================================================================
// GridMateImpl
//=========================================================================
GridMateImpl::GridMateImpl(const GridMateDesc& desc)
{
    m_endianType = desc.m_endianType;
}

//=========================================================================
// ~GridMateImpl
//=========================================================================
GridMateImpl::~GridMateImpl()
{
    while (!m_services.empty())
    {
        ServiceInfo registeredService = m_services.back();
        m_services.pop_back();
        registeredService.m_service->OnServiceUnregistered(this);
        if (registeredService.m_isOwnService)
        {
            delete registeredService.m_service;
        }
    }
}

void GridMateImpl::RegisterService(GridMateServiceId id, GridMateService* service, bool delegateOwnership)
{
    AZ_Assert(service, "Invalid service");

    GridMateService* duplicate = GetServiceById(id);
    AZ_Assert(!duplicate, "Trying to register the same GridMate service id twice.");
    if (!duplicate)
    {
        service->OnServiceRegistered(this);
        m_services.push_back();
        ServiceInfo& serviceInfo = m_services.back();        
        serviceInfo.m_isOwnService = delegateOwnership;
        serviceInfo.m_service = service;
        serviceInfo.m_serviceId = id;
        EBUS_EVENT_ID(this, GridMateEventsBus, OnGridMateServiceAdded, this, service);
    }
    else
    {        
        if (delegateOwnership)
        {
            delete service;
        }
    }
}

void GridMateImpl::UnregisterService(GridMateServiceId id)
{
    for (auto iter = m_services.begin(); iter != m_services.end(); ++iter)
    {
        if (iter->m_serviceId == id)
        {
            ServiceInfo serviceInfo = *iter;
            m_services.erase(iter);
            serviceInfo.m_service->OnServiceUnregistered(this);
            if (serviceInfo.m_isOwnService)
            {
                delete serviceInfo.m_service;
            }
            return;
        }
    }
    AZ_Error("GridMate", false, "Trying to stop an unregistered session service.");
}

bool GridMateImpl::HasService(GridMateServiceId id)
{
    GridMateService* service = GetServiceById(id);
    return service != nullptr;
}

GridMateService* GridMateImpl::GetServiceById(GridMateServiceId id)
{
    for (auto iter = m_services.begin(); iter != m_services.end(); ++iter)
    {
        if (id == iter->m_serviceId)
        {
            return iter->m_service;
        }
    }
    return nullptr;
}

//=========================================================================
// Update
//=========================================================================
void
GridMateImpl::Update()
{
    for (auto serviceIter = m_services.begin(); serviceIter != m_services.end(); ++serviceIter)
    {
        serviceIter->m_service->OnGridMateUpdate(this);
    }

    EBUS_EVENT_ID(this, GridMateEventsBus, OnGridMateUpdate, this);

}
