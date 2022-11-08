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
        ModeRead = (1 << 0),
        ModeWrite = (1 << 1),
        ModeAppend = (1 << 2),
        ModeBinary = (1 << 3),
        ModeText = (1 << 4),
        ModeUpdate = (1 << 5),
        ModeCreatePath = (1 << 6),
    };

    inline bool AnyFlag(OpenMode a)
    {
        return a != OpenMode::Invalid;
    }

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(OpenMode)

    OpenMode GetOpenModeFromStringMode(const char* mode);

    const char* GetStringModeFromOpenMode(OpenMode mode);

    // when reading, we ignore text mode requests, becuase text mode will not be supported from reading from
    // odd devices such as pack files, remote reads, and within compressed volumes anyway.  This makes it behave the same as
    // pak mode as it behaves loose.  In practice, developers should avoid depending on text mode for reading, specifically depending
    // on the platform-specific concept of removing '\r' characters from a text stream.
    // having the OS swallow characters without letting us know also messes up things like lookahead cache, optizing functions like FGetS and so on.
    void UpdateOpenModeForReading(OpenMode& mode);

    int TranslateOpenModeToSystemFileMode(const char* path, OpenMode mode);

}   // namespace AZ::IO
