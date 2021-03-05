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
    virtual void    RenderWorld(const int nRenderFlags, const SRenderingPassInfo& passInfo, const char* szDebugName) = 0;
    virtual void    ShutDown() = 0;
    virtual void    SetFlags(int flags) = 0;
    virtual int     GetFlags(void) = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_IPROCESS_H
