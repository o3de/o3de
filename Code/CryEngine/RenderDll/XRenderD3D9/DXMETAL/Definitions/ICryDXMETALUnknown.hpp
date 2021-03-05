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

// Description : Contains the cross platform replacement for COM IUnknown
//               interface on non-windows platforms

#ifndef __CRYMETALGLUNKNOWN__
#define __CRYMETALGLUNKNOWN__

#if !defined(WIN32)

#include "CryDXMETALGuid.hpp"

struct IUnknown
{
public:
#if !DXGL_FULL_EMULATION
    virtual ~IUnknown(){};
#endif //!DXGL_FULL_EMULATION

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) = 0;
    virtual ULONG STDMETHODCALLTYPE AddRef(void) = 0;
    virtual ULONG STDMETHODCALLTYPE Release(void) = 0;
};

#endif //!defined(WIN32)

#endif //__CRYMETALGLUNKNOWN__
