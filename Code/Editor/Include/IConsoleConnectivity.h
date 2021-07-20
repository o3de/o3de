/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Standard interface for console connectivity plugins.


#ifndef CRYINCLUDE_EDITOR_INCLUDE_ICONSOLECONNECTIVITY_H
#define CRYINCLUDE_EDITOR_INCLUDE_ICONSOLECONNECTIVITY_H
#pragma once


//////////////////////////////////////////////////////////////////////////
// Description
//    This interface provide access to the console connectivity
// functionality.
//////////////////////////////////////////////////////////////////////////
struct IConsoleConnectivity
    : public IUnknown
{
    DEFINE_UUID(0x4DAA85E1, 0x8498, 0x402f, 0x9B, 0x85, 0x7F, 0x62, 0x9D, 0x76, 0x79, 0x8A);

    //////////////////////////////////////////////////////////////////////////
    //TODO: Must add the useful interface here.
    //////////////////////////////////////////////////////////////////////////

    // Description:
    //   Checks if a development console is connected to the development PC.
    // See Also:
    // Arguments:
    //   Nothing
    // Return:
    //   bool - true if it is connected, false otherwise.
    virtual bool IsConnectedToConsole() = 0;

    // Description:
    //   Send a file from the specified local filename to the console platform creating the full path
    // as required so it can copy to the remote filename.
    // See Also:
    //   Nothing
    // Arguments:
    //   szLocalFileName - is the local filename from which you want to copy the file.
    //   szRemoteFilename - is the full path and filename to where you want to copy the file.
    // Return:
    //   bool - true if the copy succeeded, false otherwise.
    virtual bool SendFile(const char* szLocalFileName, const char* szRemoteFilename) = 0;

    // Description:
    //   Notifies to the console that a file has been changed, typically uploaded.
    // This will be usually called after a SendFile (see above) call, so that the
    // system running on the console may decide what to do with this new file.
    //   Typically the system will have to load or reloads this new file.
    // See Also:
    //   SendFile
    // Arguments:
    //   szRemoteFilename - is the full path and filename in the console of the changed
    // file.
    // Return:
    //   bool - true if succeeded sending the notification, false otherwise.
    virtual bool NotifyFileChange(const char* szRemoteFilename) = 0;


    // Description:
    // Gets the the title IP for the connected console .
    // Arguments:
    //  dwConsoleAddressPlaceholder - is the pointer to the placeholder of the variable
    // which will contain the title IP of the console.
    // Return:
    //   bool - true if dwConsoleAddressPlaceholder now contains the IP address, else false.
    virtual bool GetConsoleAddress(DWORD* dwConsoleAddressPlaceholder) = 0;
    //////////////////////////////////////////////////////////////////////////
    // IUnknown
    //////////////////////////////////////////////////////////////////////////
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) { return E_NOINTERFACE; };
    virtual ULONG       STDMETHODCALLTYPE AddRef()  { return 0; };
    virtual ULONG       STDMETHODCALLTYPE Release() { return 0; };
    //////////////////////////////////////////////////////////////////////////
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_ICONSOLECONNECTIVITY_H
