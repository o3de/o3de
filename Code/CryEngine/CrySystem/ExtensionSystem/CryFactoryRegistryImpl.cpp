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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Part of CryEngine's extension framework.


#include "CrySystem_precompiled.h"
#include "CryFactoryRegistryImpl.h"
#include "../System.h"

#include <CryExtension/ICryUnknown.h>
#include <CryExtension/Impl/RegFactoryNode.h>
#include <CryExtension/Impl/CryGUIDHelper.h>

#include <algorithm>


CCryFactoryRegistryImpl::CCryFactoryRegistryImpl()
    : m_guard()
    , m_byCName()
    , m_byCID()
    , m_byIID()
    , m_callbacks()
{
}


CCryFactoryRegistryImpl::~CCryFactoryRegistryImpl()
{
}


CCryFactoryRegistryImpl& CCryFactoryRegistryImpl::Access()
{
    static StaticInstance<CCryFactoryRegistryImpl, AZStd::no_destruct<CCryFactoryRegistryImpl>> s_registry;
    return s_registry;
}


ICryFactory* CCryFactoryRegistryImpl::GetFactory(const char* cname) const
{
    AUTO_READLOCK(m_guard);

    if (!cname)
    {
        return 0;
    }

    const FactoryByCName search(cname);
    FactoriesByCNameConstIt it = std::lower_bound(m_byCName.begin(), m_byCName.end(), search);
    return it != m_byCName.end() && !(search < *it) ? (*it).m_ptr : 0;
}


ICryFactory* CCryFactoryRegistryImpl::GetFactory(const CryClassID& cid) const
{
    AUTO_READLOCK(m_guard);

    const FactoryByCID search(cid);
    FactoriesByCIDConstIt it = std::lower_bound(m_byCID.begin(), m_byCID.end(), search);
    return it != m_byCID.end() && !(search < *it) ? (*it).m_ptr : 0;
}


void CCryFactoryRegistryImpl::IterateFactories(const CryInterfaceID& iid, ICryFactory** pFactories, size_t& numFactories) const
{
    AUTO_READLOCK(m_guard);

    typedef std::pair<FactoriesByIIDConstIt, FactoriesByIIDConstIt> SearchResult;
    SearchResult res = std::equal_range(m_byIID.begin(), m_byIID.end(), FactoryByIID(iid, 0), LessPredFactoryByIIDOnly());

    const size_t numFactoriesFound = std::distance(res.first, res.second);
    if (pFactories)
    {
        numFactories = min(numFactories, numFactoriesFound);
        FactoriesByIIDConstIt it = res.first;
        for (size_t i = 0; i < numFactories; ++i, ++it)
        {
            pFactories[i] = (*it).m_ptr;
        }
    }
    else
    {
        numFactories = numFactoriesFound;
    }
}


void CCryFactoryRegistryImpl::RegisterCallback(ICryFactoryRegistryCallback* pCallback)
{
    if (!pCallback)
    {
        return;
    }

    {
        AUTO_MODIFYLOCK(m_guard);

        Callbacks::iterator it = std::lower_bound(m_callbacks.begin(), m_callbacks.end(), pCallback);
        if (it == m_callbacks.end() || pCallback < *it)
        {
            m_callbacks.insert(it, pCallback);
        }
        else
        {
            assert(0 && "CCryFactoryRegistryImpl::RegisterCallback() -- pCallback already registered!");
        }
    }
    {
        AUTO_READLOCK(m_guard);

        typedef std::pair<FactoriesByIIDConstIt, FactoriesByIIDConstIt> SearchResult;
        SearchResult res = std::equal_range(m_byIID.begin(), m_byIID.end(), FactoryByIID(cryiidof<ICryUnknown>(), 0), LessPredFactoryByIIDOnly());

        for (; res.first != res.second; ++res.first)
        {
            pCallback->OnNotifyFactoryRegistered((*res.first).m_ptr);
        }
    }
}


void CCryFactoryRegistryImpl::UnregisterCallback(ICryFactoryRegistryCallback* pCallback)
{
    if (!pCallback)
    {
        return;
    }

    AUTO_MODIFYLOCK(m_guard);

    Callbacks::iterator it = std::lower_bound(m_callbacks.begin(), m_callbacks.end(), pCallback);
    if (it != m_callbacks.end() && !(pCallback < *it))
    {
        m_callbacks.erase(it);
    }
}


bool CCryFactoryRegistryImpl::GetInsertionPos(ICryFactory* pFactory, FactoriesByCNameIt& itPosForCName, FactoriesByCIDIt& itPosForCID)
{
    assert(pFactory);

    struct FatalError
    {
        static void Report(ICryFactory* pKnownFactory, ICryFactory* pNewFactory)
        {
            char err[1024];
            sprintf_s(err, sizeof(err), "Conflicting factories...\n"
                "Factory (0x%p): ClassID = %s, ClassName = \"%s\"\n"
                "Factory (0x%p): ClassID = %s, ClassName = \"%s\"",
                pKnownFactory, pKnownFactory ? CryGUIDHelper::Print(pKnownFactory->GetClassID()).c_str() : "$unknown$", pKnownFactory ? pKnownFactory->GetName() : "$unknown$",
                pNewFactory, pNewFactory ? CryGUIDHelper::Print(pNewFactory->GetClassID()).c_str() : "$unknown$", pNewFactory ? pNewFactory->GetName() : "$unknown$");

#if AZ_LEGACY_CRYSYSTEM_TRAIT_FACTORY_REGISTRY_USE_PRINTF_FOR_FATAL
            printf("\n!!! Fatal error !!!\n");
            printf(err);
            printf("\n");
#elif defined(WIN32) || defined(WIN64)
            OutputDebugStringA("\n!!! Fatal error !!!\n");
            OutputDebugStringA(err);
            OutputDebugStringA("\n");
            MessageBoxA(0, err, "!!! Fatal error !!!", MB_OK | MB_ICONERROR);
#endif

            assert(0);
            exit(0);
        }
    };

    FactoryByCName searchByCName(pFactory);
    FactoriesByCNameIt itForCName = std::lower_bound(m_byCName.begin(), m_byCName.end(), searchByCName);
    if (itForCName != m_byCName.end())
    {
        // If the addresses match, then this factory is already registered. It's not really worth error-ing about, 
        // as double registration will not cause any harm.
        if (itForCName->m_ptr == pFactory)
        {
            return false;
        }

        if (!(searchByCName < *itForCName))
        {
            FatalError::Report((*itForCName).m_ptr, pFactory);
        }
    }

    FactoryByCID searchByCID(pFactory);
    FactoriesByCIDIt itForCID = std::lower_bound(m_byCID.begin(), m_byCID.end(), searchByCID);
    if (itForCID != m_byCID.end() && !(searchByCID < *itForCID))
    {
        FatalError::Report((*itForCID).m_ptr, pFactory);
    }

    itPosForCName = itForCName;
    itPosForCID = itForCID;

    return true;
}


void CCryFactoryRegistryImpl::RegisterFactories(const SRegFactoryNode* pFactories)
{
    size_t numFactoriesToAdd = 0;
    size_t numInterfacesSupported = 0;
    {
        const SRegFactoryNode* p = pFactories;
        while (p)
        {
            ICryFactory* pFactory = p->m_pFactory;
            assert(pFactory);
            if (pFactory)
            {
                const CryInterfaceID* pIIDs = 0;
                size_t numIIDs = 0;
                pFactory->ClassSupports(pIIDs, numIIDs);

                numInterfacesSupported += numIIDs;
                ++numFactoriesToAdd;
            }

            p = p->m_pNext;
        }
    }

    {
        AUTO_MODIFYLOCK(m_guard);

        m_byCName.reserve(m_byCName.size() + numFactoriesToAdd);
        m_byCID.reserve(m_byCID.size() + numFactoriesToAdd);
        m_byIID.reserve(m_byIID.size() + numInterfacesSupported);

        size_t numFactoriesAdded = 0;
        const SRegFactoryNode* p = pFactories;
        while (p)
        {
            ICryFactory* pFactory = p->m_pFactory;
            if (pFactory)
            {
                FactoriesByCNameIt itPosForCName;
                FactoriesByCIDIt itPosForCID;
                if (GetInsertionPos(pFactory, itPosForCName, itPosForCID))
                {
                    m_byCName.insert(itPosForCName, FactoryByCName(pFactory));
                    m_byCID.insert(itPosForCID, FactoryByCID(pFactory));

                    const CryInterfaceID* pIIDs = 0;
                    size_t numIIDs = 0;
                    pFactory->ClassSupports(pIIDs, numIIDs);

                    for (size_t i = 0; i < numIIDs; ++i)
                    {
                        const FactoryByIID newFactory(pIIDs[i], pFactory);
                        m_byIID.push_back(newFactory);
                    }

                    for (size_t i = 0, s = m_callbacks.size(); i < s; ++i)
                    {
                        m_callbacks[i]->OnNotifyFactoryRegistered(pFactory);
                    }

                    ++numFactoriesAdded;
                }
            }

            p = p->m_pNext;
        }

        if (numFactoriesAdded)
        {
            std::sort(m_byIID.begin(), m_byIID.end());
        }
    }
}


void CCryFactoryRegistryImpl::UnregisterFactories(const SRegFactoryNode* pFactories)
{
    AUTO_MODIFYLOCK(m_guard);

    const SRegFactoryNode* p = pFactories;
    while (p)
    {
        ICryFactory* pFactory = p->m_pFactory;
        UnregisterFactoryInternal(pFactory);
        p = p->m_pNext;
    }
}


void CCryFactoryRegistryImpl::UnregisterFactory(ICryFactory* const pFactory)
{
    AUTO_MODIFYLOCK(m_guard);

    UnregisterFactoryInternal(pFactory);
}


void CCryFactoryRegistryImpl::UnregisterFactoryInternal(ICryFactory* const pFactory)
{
    if (pFactory)
    {
        FactoryByCName searchByCName(pFactory);
        FactoriesByCNameIt itForCName = std::lower_bound(m_byCName.begin(), m_byCName.end(), searchByCName);
        if (itForCName != m_byCName.end() && !(searchByCName < *itForCName))
        {
            assert((*itForCName).m_ptr == pFactory);
            if ((*itForCName).m_ptr == pFactory)
            {
                m_byCName.erase(itForCName);
            }
        }

        FactoryByCID searchByCID(pFactory);
        FactoriesByCIDIt itForCID = std::lower_bound(m_byCID.begin(), m_byCID.end(), searchByCID);
        if (itForCID != m_byCID.end() && !(searchByCID < *itForCID))
        {
            assert((*itForCID).m_ptr == pFactory);
            if ((*itForCID).m_ptr == pFactory)
            {
                m_byCID.erase(itForCID);
            }
        }

        const CryInterfaceID* pIIDs = 0;
        size_t numIIDs = 0;
        pFactory->ClassSupports(pIIDs, numIIDs);

        for (size_t i = 0; i < numIIDs; ++i)
        {
            FactoryByIID searchByIID(pIIDs[i], pFactory);
            FactoriesByIIDIt itForIID = std::lower_bound(m_byIID.begin(), m_byIID.end(), searchByIID);
            if (itForIID != m_byIID.end() && !(searchByIID < *itForIID))
            {
                m_byIID.erase(itForIID);
            }
        }

        for (size_t i = 0, s = m_callbacks.size(); i < s; ++i)
        {
            m_callbacks[i]->OnNotifyFactoryUnregistered(pFactory);
        }
    }
}

ICryFactoryRegistry* CSystem::GetCryFactoryRegistry() const
{
    return &CCryFactoryRegistryImpl::Access();
}
