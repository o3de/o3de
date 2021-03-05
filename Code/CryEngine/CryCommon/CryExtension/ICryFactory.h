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


#ifndef CRYINCLUDE_CRYEXTENSION_ICRYFACTORY_H
#define CRYINCLUDE_CRYEXTENSION_ICRYFACTORY_H
#pragma once


#include "CryTypeID.h"
#include <SmartPointersHelpers.h>

struct ICryUnknown;
DECLARE_SMART_POINTERS(ICryUnknown);

struct ICryFactory
{
    virtual const char* GetName() const = 0;
    virtual const CryClassID& GetClassID() const = 0;
    virtual bool ClassSupports(const CryInterfaceID& iid) const = 0;
    virtual void ClassSupports(const CryInterfaceID*& pIIDs, size_t& numIIDs) const = 0;
    virtual ICryUnknownPtr CreateClassInstance() const = 0;

protected:
    // prevent explicit destruction from client side (delete, shared_ptr, etc)
    virtual ~ICryFactory() {}
};

#endif // CRYINCLUDE_CRYEXTENSION_ICRYFACTORY_H
