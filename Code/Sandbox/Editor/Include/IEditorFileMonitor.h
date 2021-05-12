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
