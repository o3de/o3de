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


#ifndef CRYINCLUDE_CRYEXTENSION_ICRYFACTORYREGISTRY_H
#define CRYINCLUDE_CRYEXTENSION_ICRYFACTORYREGISTRY_H
#pragma once


#include "CryTypeID.h"


struct ICryFactory;


struct ICryFactoryRegistry
{
    virtual ICryFactory* GetFactory(const char* cname) const = 0;
    virtual ICryFactory* GetFactory(const CryClassID& cid) const = 0;
    /**
     * Iterates all factories implementing the interface specified by \p iid.
     * \param[in]       iid             ID of the interface to iterate. Often procured using cryiidof<...>().
     * \param[out]      pFactories      A pointer of the array of factories to fill in. May be nullptr (see below).
     * \param[in] Size (in elements) of the pFactories array [out] Number of elements actually written to pFactories or, when pFactories is null, the number of elements that would be written if sufficient storage was available.
     *
     * Example:
     * \code{.cpp}
     * size_t factoryCount = 0;
     * // Assigns the number of found factories to factoryCount
     * factoryRegistry->IterateFactories(cryiidof<TPointer>(), 0, factoryCount);
     * // Allocate an array of the proper length on the stack
     * ICryFactory** factories = static_cast<ICryFactory**>(alloca(sizeof(ICryFactory*) * factoryCount);
     * // Fill in factories with factoryCount results.
     * factoryRegistry->IterateFactories(cryiidof<TPointer>(), factories, factoryCount);
     * \endcode
     */
    virtual void IterateFactories(const CryInterfaceID& iid, ICryFactory** pFactories, size_t& numFactories) const = 0;

protected:
    // prevent explicit destruction from client side (delete, shared_ptr, etc)
    virtual ~ICryFactoryRegistry() {}
};

#endif // CRYINCLUDE_CRYEXTENSION_ICRYFACTORYREGISTRY_H
