/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/std/typetraits/underlying_type.h>

namespace AZ::IO
{
    enum class OpenMode : AZ::u32
    {
        Invalid = 0,
        //! Open the file in read mode ("r")
        //! if ModeRead is specified without ModeWrite, the file is open in file share write mode
        //! when on platforms that use WinAPI
        ModeRead = (1 << 0),
        //! Open the file in write mode ("w")
        //! If the ModeAppend option is not specified, the file is truncated
        ModeWrite = (1 << 1),
        //! Opens the file in append mode ("a")
        //! The file is opened and sets the write offset to the end of the file
        ModeAppend = (1 << 2),
        //! Opens the file with the binary option("b") set
        ModeBinary = (1 << 3),
        //! Opens the file with the text option("t") set
        //! This affects may affect how newlines are handled on different platforms ("\n" vs "\r\n")
        ModeText = (1 << 4),
        //! Open the file in update mode with the ("+") symbol
        //! If ModeRead is also set and the file doe NOT exist, it will not be created
        //! in that scenario
        ModeUpdate = (1 << 5),
        //! Create any parent directory segments that are part of the file path if they don't exist.
        ModeCreatePath = (1 << 6),
    };

    inline bool AnyFlag(OpenMode a)
    {
        return a != OpenMode::Invalid;
    }

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(OpenMode)

    OpenMode GetOpenModeFromStringMode(const char* mode);

    const char* GetStringModeFromOpenMode(OpenMode mode);

    // when reading, we ignore text mode requests, because text mode will not be supported from reading from
    // odd devices such as pack files, remote reads, and within compressed volumes anyway.  This makes it behave the same as
    // pak mode as it behaves loose.  In practice, developers should avoid depending on text mode for reading, specifically depending
    // on the platform-specific concept of removing '\r' characters from a text stream.
    // having the OS swallow characters without letting us know also messes up things like lookahead cache, optimizing functions like FGetS and so on.
    void UpdateOpenModeForReading(OpenMode& mode);

    int TranslateOpenModeToSystemFileMode(const char* path, OpenMode mode);

}   // namespace AZ::IO
