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

// Description : This file declares the interface used by the texture viewer
//               and (implemented first implemented by the Texture Database Creator) to
//               syncronize their threads. A thread interace could be useful there.

#ifndef CRYINCLUDE_EDITOR_INCLUDE_ITEXTUREDATABASEUPDATER_H
#define CRYINCLUDE_EDITOR_INCLUDE_ITEXTUREDATABASEUPDATER_H
#pragma once


class CTextureDatabaseItem;

struct ITextureDatabaseUpdater
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Thread control
    virtual void NotifyShutDown() = 0;
    virtual void Lock() = 0;
    virtual void Unlock() = 0;
    virtual void WaitForThread() = 0;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Data access
    virtual CTextureDatabaseItem*   GetItem(const char* szAddItem) = 0;
    //////////////////////////////////////////////////////////////////////////
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_ITEXTUREDATABASEUPDATER_H
