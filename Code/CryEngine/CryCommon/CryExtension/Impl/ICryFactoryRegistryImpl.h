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


#ifndef CRYINCLUDE_CRYEXTENSION_IMPL_ICRYFACTORYREGISTRYIMPL_H
#define CRYINCLUDE_CRYEXTENSION_IMPL_ICRYFACTORYREGISTRYIMPL_H
#pragma once


#include "../ICryFactoryRegistry.h"


struct SRegFactoryNode;


struct ICryFactoryRegistryCallback
{
    virtual void OnNotifyFactoryRegistered(ICryFactory* pFactory) = 0;
    virtual void OnNotifyFactoryUnregistered(ICryFactory* pFactory) = 0;

protected:
    virtual ~ICryFactoryRegistryCallback() {}
};


struct ICryFactoryRegistryImpl
    : public ICryFactoryRegistry
{
    virtual ICryFactory* GetFactory(const char* cname) const = 0;
    virtual ICryFactory* GetFactory(const CryClassID& cid) const = 0;
    virtual void IterateFactories(const CryInterfaceID& iid, ICryFactory** pFactories, size_t& numFactories) const = 0;

    virtual void RegisterCallback(ICryFactoryRegistryCallback* pCallback) = 0;
    virtual void UnregisterCallback(ICryFactoryRegistryCallback* pCallback) = 0;

    virtual void RegisterFactories(const SRegFactoryNode* pFactories) = 0;
    virtual void UnregisterFactories(const SRegFactoryNode* pFactories) = 0;

    virtual void UnregisterFactory(ICryFactory* const pFactory) = 0;

protected:
    // prevent explicit destruction from client side (delete, shared_ptr, etc)
    virtual ~ICryFactoryRegistryImpl() {}
};

#endif // CRYINCLUDE_CRYEXTENSION_IMPL_ICRYFACTORYREGISTRYIMPL_H
