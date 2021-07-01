/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
