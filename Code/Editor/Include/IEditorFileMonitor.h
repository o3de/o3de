/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_INCLUDE_IEDITORFILEMONITOR_H
#define CRYINCLUDE_EDITOR_INCLUDE_IEDITORFILEMONITOR_H
#pragma once

struct IFileChangeListener
{
    enum EChangeType
    {
        //! error or unknown change type
        eChangeType_Unknown,
        //! the file was created
        eChangeType_Created,
        //! the file was deleted
        eChangeType_Deleted,
        //! the file was modified (size changed,write)
        eChangeType_Modified,
        //! this is the old name of a renamed file
        eChangeType_RenamedOldName,
        //! this is the new name of a renamed file
        eChangeType_RenamedNewName
    };

    virtual ~IFileChangeListener() = default;
    
    virtual void OnFileChange(const char* sFilename, EChangeType eType) = 0;
};

struct IFileChangeMonitor
{
    virtual ~IFileChangeMonitor() = default;

    // <interfuscator:shuffle>
    // Register the path of a file or directory to monitor
    // Path is relative to game directory, e.g. "Libs/WoundSystem/" or "Libs/WoundSystem/HitLocations.xml"
    virtual bool RegisterListener(IFileChangeListener* pListener, const char* sMonitorItem) = 0;
    // This function can be used to monitor files of specific type, e.g.
    //   RegisterListener(pListener, "Animations", "caf")
    virtual bool RegisterListener(IFileChangeListener* pListener, const char* sFolder, const char* sExtension) = 0;
    virtual bool UnregisterListener(IFileChangeListener* pListener) = 0;
    // </interfuscator:shuffle>
};

struct IEditorFileMonitor
    : public IFileChangeMonitor
{
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_IEDITORFILEMONITOR_H
