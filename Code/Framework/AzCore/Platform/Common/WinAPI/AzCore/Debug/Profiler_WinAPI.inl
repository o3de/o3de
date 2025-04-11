/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifdef USE_PIX
    #include <AzCore/PlatformIncl.h>
    #include <WinPixEventRuntime/pix3.h>
#endif

namespace AZ::Debug::Platform
{
    template<typename... T>
    void BeginProfileRegion([[maybe_unused]] Budget* budget, [[maybe_unused]] const char* eventName, [[maybe_unused]] T const&... args)
    {
    #ifdef USE_PIX
        PIXBeginEvent(PIX_COLOR_INDEX(budget->Crc() & 0xff), eventName, args...);
    #endif
    }

    inline void BeginProfileRegion([[maybe_unused]] Budget* budget, [[maybe_unused]] const char* eventName)
    {
    #ifdef USE_PIX
        PIXBeginEvent(PIX_COLOR_INDEX(budget->Crc() & 0xff), eventName);
    #endif
    }

    inline void EndProfileRegion([[maybe_unused]] Budget* budget)
    {
    #ifdef USE_PIX
        PIXEndEvent();
    #endif
    }

    template<typename T>
    inline void ReportCounter(
        [[maybe_unused]] const Budget* budget, [[maybe_unused]] const wchar_t* counterName, [[maybe_unused]] const T& value)
    {
#ifdef USE_PIX
        PIXReportCounter(counterName, aznumeric_cast<float>(value)); // PIX API requires a float value
#endif
    }

    inline void ReportProfileEvent([[maybe_unused]] const Budget* budget,
        [[maybe_unused]] const char* eventName)
    {
#ifdef USE_PIX
        PIXSetMarker(PIX_COLOR_INDEX(budget->Crc() & 0xff), eventName);
#endif
    }

} // namespace AZ::Debug::Platform
