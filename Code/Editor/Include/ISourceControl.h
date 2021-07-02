/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <map>

#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#include "IEditorClassFactory.h" 

// Source control status of item.
enum ESccFileAttributes
{
    SCC_FILE_ATTRIBUTE_INVALID      = 0x0000, // File is not found.
    SCC_FILE_ATTRIBUTE_NORMAL       = 0x0001, // Normal file on disk.
    SCC_FILE_ATTRIBUTE_READONLY     = 0x0002, // Read only files cannot be modified at all, either read only file not under source control or file in packfile.
    SCC_FILE_ATTRIBUTE_INPAK        = 0x0004, // File is inside pack file.
    SCC_FILE_ATTRIBUTE_MANAGED      = 0x0008, // File is managed under source control.
    SCC_FILE_ATTRIBUTE_CHECKEDOUT   = 0x0010, // File is under source control and is checked out.
    SCC_FILE_ATTRIBUTE_BYANOTHER    = 0x0020, // File is under source control and is checked out by another user.
    SCC_FILE_ATTRIBUTE_FOLDER       = 0x0040, // Managed folder.
    SCC_FILE_ATTRIBUTE_LOCKEDBYANOTHER = 0x0080, // File is under source control and is checked out and locked by another user.
    SCC_FILE_ATTRIBUTE_NOTATHEAD    = 0x0100, // File is under source control and is not the latest version of the file
    SCC_FILE_ATTRIBUTE_ADD          = 0x0200, // File is under source control and is marked for add
    SCC_FILE_ATTRIBUTE_MOVED        = 0x0400, // File is under source control and is marked for move/add
};


//////////////////////////////////////////////////////////////////////////
// Description
//    This interface provide access to the source control functionality.
//////////////////////////////////////////////////////////////////////////
struct ISourceControl
    : public IUnknown
{
    DEFINE_UUID(0x1D391E8C, 0xA124, 0x46bb, 0x80, 0x8D, 0x9B, 0xCA, 0x15, 0x5B, 0xCA, 0xFD)

    // Source Control State
    enum ConnectivityState
    {
        Connected = 0,
        BadConfiguration,
        Disconnected_Retrying,
        Disconnected,
    };

    using SourceControlState = AzToolsFramework::SourceControlState;

    //function to enable/disable source control
    virtual void SetSourceControlState(SourceControlState state) = 0;
    virtual ConnectivityState GetConnectivityState() = 0;

    // Show settings dialog
    virtual void ShowSettings() = 0;

    //////////////////////////////////////////////////////////////////////////
    // IUnknown
    //////////////////////////////////////////////////////////////////////////
    virtual HRESULT STDMETHODCALLTYPE QueryInterface([[maybe_unused]] REFIID riid, [[maybe_unused]] void** ppvObject) { return E_NOINTERFACE; };
    virtual ULONG STDMETHODCALLTYPE AddRef() { return 0; };
    virtual ULONG STDMETHODCALLTYPE Release() { return 0; };
    //////////////////////////////////////////////////////////////////////////
};

