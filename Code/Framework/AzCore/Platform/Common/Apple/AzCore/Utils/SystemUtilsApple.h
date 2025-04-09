/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Description : Utilities for iOS and Mac OS X.

#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string_view.h>

namespace AZ::SystemUtilsApple
{
    //! error code value returned when querying information about the Apple system fails
    enum class AppSupportErrorCode
    {
        BufferTooSmall = 1,
        // Path is not available is set when the running
        // application isn't a bundle
        PathNotAvailable
    };

    //! Return results from GetPath* App support methods
    //! The `m_requiredSize` value is set to the needed size of the buffer
    //! when the error code is `BufferTooSmall`
    //! This does not include the space needed for a null-terminator
    struct AppSupportErrorResult
    {
        //! Contains the error code of the App support operation
        AppSupportErrorCode m_errorCode{};
        //! Set only when the error code is `BufferTooSmall`
        size_t m_requiredSize{};
    };
    using AppSupportOutcome = AZ::Outcome<AZStd::string_view, AppSupportErrorResult>;

    //! Get the path to the application's bundle.
    //! @param buffer pre-sized buffer to used to fill result of query operation
    //! @return outcome with a string_view of the application bundle path on success
    //! On failure an AppSupportErrorResult is returned
    AppSupportOutcome GetPathToApplicationBundle(AZStd::span<char> buffer);

    //! Get the path to the application's executable.
    //! @param buffer pre-sized buffer to used to fill result of query operation
    //! @return outcome with a string_view of the application executable path on success
    //! On failure an AppSupportErrorResult is returned
    AppSupportOutcome GetPathToApplicationExecutable(AZStd::span<char> buffer);

    //! Get the path to the application's Frameworks directory.
    //! @param buffer pre-sized buffer to used to fill result of query operation
    //! @return outcome with a string_view of the application Frameworks path on success
    //! On failure an AppSupportErrorResult is returned
    AppSupportOutcome GetPathToApplicationFrameworks(AZStd::span<char> buffer);

    //! Get the path to the application's resources.
    //! @param buffer pre-sized buffer to used to fill result of query operation
    //! @return outcome with a string_view of the application resource path on success
    //! On failure an AppSupportErrorResult is returned
    AppSupportOutcome GetPathToApplicationResources(AZStd::span<char> buffer);

    //! Get the path to the user domain's application support directory.
    //! Used to store application generated content.
    //! iOS: Not available through file sharing.
    //! Persistent, backed up by iTunes.
    //! @param buffer pre-sized buffer to used to fill result of query operation
    //! @return outcome with a string_view of the users application support path on success
    //! On failure an AppSupportErrorResult is returned
    AppSupportOutcome GetPathToUserApplicationSupportDirectory(AZStd::span<char> buffer);

    //! Get the path to the user domain's caches directory.
    //! Used to store application generated content.
    //! iOS: Not available through file sharing.
    //! Temporary, not backed up by iTunes.
    //! @param buffer pre-sized buffer to used to fill result of query operation
    //! @return outcome with a string_view of the users caches path on success
    //! On failure an AppSupportErrorResult is returned
    AppSupportOutcome GetPathToUserCachesDirectory(AZStd::span<char> buffer);

    //! Get the path to the user domain's document directory.
    //! Used to store user generated content.
    //! iOS: Available through file sharing.
    //! Persistent, backed up by iTunes.
    //! @param buffer pre-sized buffer to used to fill result of query operation
    //! @return outcome with a string_view of the users document path on success
    //! On failure an AppSupportErrorResult is returned
    AppSupportOutcome GetPathToUserDocumentDirectory(AZStd::span<char> buffer);

    //! Get the path to the user domain's library directory.
    //! Parent directory of app support and caches.
    //! iOS: Not available through file sharing.
    //! Persistent, backed up by iTunes.
    //! @param buffer pre-sized buffer to used to fill result of query operation
    //! @return outcome with a string_view of the users library path on success
    //! On failure an AppSupportErrorResult is returned
    AppSupportOutcome GetPathToUserLibraryDirectory(AZStd::span<char> buffer);

    //! Get the user's name.
    //! @param buffer pre-sized buffer to used to fill result of query operation
    //! @return outcome with a string_view of the application bundle path on success
    //! On failure an AppSupportErrorResult is returned
    AppSupportOutcome GetUserName(AZStd::span<char> buffer);
}
