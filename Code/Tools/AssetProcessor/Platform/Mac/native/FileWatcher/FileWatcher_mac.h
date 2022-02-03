/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <native/FileWatcher/FileWatcher.h>

#include <CoreServices/CoreServices.h>

class FileWatcher::PlatformImplementation
{
public:
    FSEventStreamRef m_stream = nullptr;
    CFRunLoopRef m_runLoop = nullptr;
};
