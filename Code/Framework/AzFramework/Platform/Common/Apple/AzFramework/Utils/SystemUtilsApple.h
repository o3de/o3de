/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Utilities for iOS and Mac OS X. Needs to be separated
//               due to conflict with the system headers.


#ifndef CRYINCLUDE_CRYSYSTEM_SYSTEMUTILSAPPLE_H
#define CRYINCLUDE_CRYSYSTEM_SYSTEMUTILSAPPLE_H
#pragma once

#include <sys/resource.h>
#include <sys/types.h>
#include <AzCore/std/string/string.h>

namespace SystemUtilsApple
{
    // Get the path to the application's bundle.
    // - Returns length of the string or 0 on failure.
    size_t GetPathToApplicationBundle(char* buffer, size_t bufferLen);

    // Get the path to the application's executable.
    // - Returns length of the string or 0 on failure.
    size_t GetPathToApplicationExecutable(char* buffer, size_t bufferLen);

    // Get the path to the application's resources.
    // - Returns length of the string or 0 on failure.
    size_t GetPathToApplicationResources(char* buffer, size_t bufferLen);

    // Get the path to the user domain's application support directory.
    // - Returns length of the string or 0 on failure.
    // - Used to store application generated content.
    // - iOS: Not available through file sharing.
    // - Persistent, backed up by iTunes.
    size_t GetPathToUserApplicationSupportDirectory(char* buffer, size_t bufferLen);

    // Get the path to the user domain's caches directory.
    // - Returns length of the string or 0 on failure.
    // - Used to store application generated content.
    // - iOS: Not available through file sharing.
    // - Temporary, not backed up by iTunes.
    size_t GetPathToUserCachesDirectory(char* buffer, size_t bufferLen);

    // Get the path to the user domain's document directory.
    // - Returns length of the string or 0 on failure.
    // - Used to store user generated content.
    // - iOS: Available through file sharing.
    // - Persistent, backed up by iTunes.
    size_t GetPathToUserDocumentDirectory(char* buffer, size_t bufferLen);

    // Get the path to the user domain's library directory.
    // - Returns length of the string or 0 on failure.
    // - Parent directory of app support and caches.
    // - iOS: Not available through file sharing.
    // - Persistent, backed up by iTunes.
    size_t GetPathToUserLibraryDirectory(char* buffer, size_t bufferLen);

    // Get the user's name.
    // - Returns length of the string or 0 on failure.
    size_t GetUserName(char* buffer, size_t bufferLen);

    // Get the device's machine name
    // - Returns string representing device identifier or empty string on failure
    // - Unique for each model
    AZStd::string GetMachineName();
}

#endif // CRYINCLUDE_CRYSYSTEM_SYSTEMUTILSAPPLE_H
