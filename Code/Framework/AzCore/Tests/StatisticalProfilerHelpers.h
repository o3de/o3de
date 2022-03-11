/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzTest/AzTest.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/Statistics/StatisticalProfilerProxy.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/std/parallel/spin_mutex.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/string/string.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/Utils/TypeHash.h>

namespace UnitTest
{
    AZ_TYPE_SAFE_INTEGRAL(StringHash, size_t);

    static constexpr int SmallIterationCount  = 10;
    static constexpr int MediumIterationCount = 1000;
    static constexpr int LargeIterationCount  = 100000;

    static constexpr AZ::u32 ProfilerProxyGroup = AZ_CRC_CE("StatisticalProfilerProxyTests");

    template<typename StatIdType>
    StatIdType ConvertNameToStatId(const AZStd::string& name);

    template<>
    inline AZStd::string ConvertNameToStatId<AZStd::string>(const AZStd::string& name)
    {
        return name;
    }

    template<>
    inline StringHash ConvertNameToStatId<StringHash>(const AZStd::string& name)
    {
        AZStd::hash<AZStd::string> hasher;
        return StringHash(hasher(name));
    }

    template<>
    inline AZ::Crc32 ConvertNameToStatId<AZ::Crc32>(const AZStd::string& name)
    {
        return AZ::Crc32(name);
    }

    template<>
    inline AZ::HashValue32 ConvertNameToStatId<AZ::HashValue32>(const AZStd::string& name)
    {
        return AZ::TypeHash32(name.c_str());
    }

    template<>
    inline AZ::HashValue64 ConvertNameToStatId<AZ::HashValue64>(const AZStd::string& name)
    {
        return AZ::TypeHash64(name.c_str());
    }
}//namespace UnitTest
