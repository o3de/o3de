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


#ifndef CRYINCLUDE_CRYEXTENSION_CRYCREATECLASSINSTANCE_H
#define CRYINCLUDE_CRYEXTENSION_CRYCREATECLASSINSTANCE_H
#pragma once


#include "ICryUnknown.h"
#include "ICryFactory.h"
#include "ICryFactoryRegistry.h"
#include <ISystem.h> // <> required for Interfuscator


template <class T>
bool CryCreateClassInstance(const CryClassID& cid, AZStd::shared_ptr<T>& p)
{
    p = AZStd::shared_ptr<T>();
    ICryFactoryRegistry* pFactoryReg = gEnv->pSystem->GetCryFactoryRegistry();
    if (pFactoryReg)
    {
        ICryFactory* pFactory = pFactoryReg->GetFactory(cid);
        if (pFactory && pFactory->ClassSupports(cryiidof<T>()))
        {
            ICryUnknownPtr pUnk = pFactory->CreateClassInstance();
            AZStd::shared_ptr<T> pT = cryinterface_cast<T>(pUnk);
            if (pT)
            {
                p = pT;
            }
        }
    }
    return p.get() != NULL;
}


template <class T>
bool CryCreateClassInstance(const char* cname, AZStd::shared_ptr<T>& p)
{
    p = AZStd::shared_ptr<T>();
    ICryFactoryRegistry* pFactoryReg = gEnv->pSystem->GetCryFactoryRegistry();
    if (pFactoryReg)
    {
        ICryFactory* pFactory = pFactoryReg->GetFactory(cname);
        if (pFactory != NULL && pFactory->ClassSupports(cryiidof<T>()))
        {
            ICryUnknownPtr pUnk = pFactory->CreateClassInstance();
            AZStd::shared_ptr<T> pT = cryinterface_cast<T>(pUnk);
            if (pT)
            {
                p = pT;
            }
        }
    }
    return p.get() != NULL;
}


template <class T>
bool CryCreateClassInstanceForInterface(const CryInterfaceID& iid, AZStd::shared_ptr<T>& p)
{
    p = AZStd::shared_ptr<T>();
    ICryFactoryRegistry* pFactoryReg = gEnv->pSystem->GetCryFactoryRegistry();
    if (pFactoryReg)
    {
        size_t numFactories = 1;
        ICryFactory* pFactory = 0;
        pFactoryReg->IterateFactories(iid, &pFactory, numFactories);
        if (numFactories == 1 && pFactory)
        {
            ICryUnknownPtr pUnk = pFactory->CreateClassInstance();
            AZStd::shared_ptr<T> pT = cryinterface_cast<T>(pUnk);
            if (pT)
            {
                p = pT;
            }
        }
    }
    return p.get() != NULL;
}


#endif // CRYINCLUDE_CRYEXTENSION_CRYCREATECLASSINSTANCE_H
