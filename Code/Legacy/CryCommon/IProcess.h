/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Process common interface


#ifndef CRYINCLUDE_CRYCOMMON_IPROCESS_H
#define CRYINCLUDE_CRYCOMMON_IPROCESS_H
#pragma once


// forward declaration
struct      SRenderingPassInfo;
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
struct IProcess
{
    // <interfuscator:shuffle>
    virtual ~IProcess(){}
    virtual bool    Init() = 0;
    virtual void    Update() = 0;
    virtual void    RenderWorld(int nRenderFlags, const SRenderingPassInfo& passInfo, const char* szDebugName) = 0;
    virtual void    ShutDown() = 0;
    virtual void    SetFlags(int flags) = 0;
    virtual int     GetFlags(void) = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_IPROCESS_H
