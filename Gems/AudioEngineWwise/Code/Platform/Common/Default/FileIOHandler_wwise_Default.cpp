/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <FileIOHandler_wwise.h>
#include <AzCore/IO/FileIO.h>

#include <IAudioInterfacesCommonData.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AudioEngineWwise_Traits_Platform.h>


namespace Audio::Platform
{
    AkFileHandle GetAkFileHandle(AZ::IO::HandleType realFileHandle)
    {
        return (AkFileHandle)static_cast<uintptr_t>(realFileHandle);
    }

    AZ::IO::HandleType GetRealFileHandle(AkFileHandle akFileHandle)
    {
        // On 64 bit systems, strict compilers throw an error trying to reinterpret_cast
        // from AkFileHandle (a 64 bit pointer) to AZ::IO::HandleType (a uint32_t) because:
        //
        // cast from pointer to smaller type 'AZ::IO::HandleType' (aka 'unsigned int') loses information
        //
        // However, this is safe because AkFileHandle is a "blind" type that serves as a token.
        // We create the token and hand it off, and it is handed back whenever file IO is done.
        return static_cast<AZ::IO::HandleType>((uintptr_t)akFileHandle);
    }

    void SetThreadProperties(AkThreadProperties& threadProperties)
    {
        threadProperties.dwAffinityMask = AZ_TRAIT_AUDIOENGINEWWISE_FILEIO_AKDEVICE_THREAD_AFFINITY_MASK;
    }
} // namespace Audio::Platform
