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
#include <native/FileWatcher/FileWatcher.h>

struct FolderRootWatch::PlatformImplementation
{
    PlatformImplementation() { }
};

//////////////////////////////////////////////////////////////////////////////
/// FolderWatchRoot
FolderRootWatch::FolderRootWatch(const QString rootFolder)
    : m_root(rootFolder)
    , m_shutdownThreadSignal(false)
    , m_fileWatcher(nullptr)
    , m_platformImpl(new PlatformImplementation())
{
}

FolderRootWatch::~FolderRootWatch()
{
    // Destructor is required in here since this file contains the definition of struct PlatformImplementation
    Stop();

    delete m_platformImpl;
}

bool FolderRootWatch::Start()
{
    // TODO: Implement for Linux
    return false;
}

void FolderRootWatch::Stop()
{
    // TODO: Implement for Linux
}

void FolderRootWatch::WatchFolderLoop()
{
    // TODO: Implement for Linux
}

