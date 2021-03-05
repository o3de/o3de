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


#ifndef CRYINCLUDE_CRYSYSTEM_EXTENSIONSYSTEM_CRYFACTORYREGISTRYIMPL_H
#define CRYINCLUDE_CRYSYSTEM_EXTENSIONSYSTEM_CRYFACTORYREGISTRYIMPL_H
#pragma once



#include <CryExtension/Impl/ICryFactoryRegistryImpl.h>
#include <CryExtension/ICryFactory.h>

#include <AzCore/std/containers/vector.h>


class CCryFactoryRegistryImpl
    : public ICryFactoryRegistryImpl
{
public:
    virtual ICryFactory* GetFactory(const char* cname) const;
    virtual ICryFactory* GetFactory(const CryClassID& cid) const;
    virtual void IterateFactories(const CryInterfaceID& iid, ICryFactory** pFactories, size_t& numFactories) const;

    virtual void RegisterCallback(ICryFactoryRegistryCallback* pCallback);
    virtual void UnregisterCallback(ICryFactoryRegistryCallback* pCallback);

    virtual void RegisterFactories(const SRegFactoryNode* pFactories);
    virtual void UnregisterFactories(const SRegFactoryNode* pFactories);

    virtual void UnregisterFactory(ICryFactory* const pFactory);

public:
    static CCryFactoryRegistryImpl& Access();
    CCryFactoryRegistryImpl();
    ~CCryFactoryRegistryImpl();

private:
    struct FactoryByCName
    {
        const char* m_cname;
        ICryFactory* m_ptr;

        FactoryByCName(const char* cname)
            : m_cname(cname)
            , m_ptr(0) {assert(m_cname); }
        FactoryByCName(ICryFactory* ptr)
            : m_cname(ptr ? ptr->GetName() : 0)
            , m_ptr(ptr) {assert(m_cname && m_ptr); }
        bool operator <(const FactoryByCName& rhs) const {return strcmp(m_cname, rhs.m_cname) < 0; }
    };
    typedef std::vector<FactoryByCName> FactoriesByCName;
    typedef FactoriesByCName::iterator FactoriesByCNameIt;
    typedef FactoriesByCName::const_iterator FactoriesByCNameConstIt;

    struct FactoryByCID
    {
        CryClassID m_cid;
        ICryFactory* m_ptr;

        FactoryByCID(const CryClassID& cid)
            : m_cid(cid)
            , m_ptr(0) {}
        FactoryByCID(ICryFactory* ptr)
            : m_cid(ptr ? ptr->GetClassID() : MAKE_CRYGUID(0, 0))
            , m_ptr(ptr) {assert(m_ptr); }
        bool operator <(const FactoryByCID& rhs) const {return m_cid < rhs.m_cid; }
    };
    typedef std::vector<FactoryByCID> FactoriesByCID;
    typedef FactoriesByCID::iterator FactoriesByCIDIt;
    typedef FactoriesByCID::const_iterator FactoriesByCIDConstIt;

    struct FactoryByIID
    {
        CryInterfaceID m_iid;
        ICryFactory* m_ptr;

        FactoryByIID(CryInterfaceID iid, ICryFactory* pFactory)
            : m_iid(iid)
            , m_ptr(pFactory) {}
        bool operator <(const FactoryByIID& rhs) const
        {
            if (m_iid != rhs.m_iid)
            {
                return m_iid < rhs.m_iid;
            }
            return m_ptr < rhs.m_ptr;
        }
    };
    typedef std::vector<FactoryByIID> FactoriesByIID;
    typedef FactoriesByIID::iterator FactoriesByIIDIt;
    typedef FactoriesByIID::const_iterator FactoriesByIIDConstIt;
    struct LessPredFactoryByIIDOnly
    {
        bool operator ()(const FactoryByIID& lhs, const FactoryByIID& rhs) const {return lhs.m_iid < rhs.m_iid; }
    };

    typedef std::vector<ICryFactoryRegistryCallback*> Callbacks;
    typedef Callbacks::iterator CallbacksIt;
    typedef Callbacks::const_iterator CallbacksConstIt;

private:
    bool GetInsertionPos(ICryFactory* pFactory, FactoriesByCNameIt& itPosForCName, FactoriesByCIDIt& itPosForCID);
    void UnregisterFactoryInternal(ICryFactory* const pFactory);

private:
    mutable CryReadModifyLock m_guard;

    FactoriesByCName m_byCName;
    FactoriesByCID m_byCID;
    FactoriesByIID m_byIID;

    Callbacks m_callbacks;
};

#endif // CRYINCLUDE_CRYSYSTEM_EXTENSIONSYSTEM_CRYFACTORYREGISTRYIMPL_H
