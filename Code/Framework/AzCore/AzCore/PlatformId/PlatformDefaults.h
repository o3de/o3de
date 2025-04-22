/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/std/string/string_view.h>

// On IOS builds IOS will be defined and interfere with the below enums
#pragma push_macro("IOS")
#undef IOS

namespace AZ
{
    inline namespace PlatformDefaults
    {
        constexpr char PlatformPC[] = "pc";
        constexpr char PlatformLinux[] = "linux";
        constexpr char PlatformAndroid[] = "android";
        constexpr char PlatformIOS[] = "ios";
        constexpr char PlatformMac[] = "mac";
        constexpr char PlatformProvo[] = "provo";
        constexpr char PlatformSalem[] = "salem";
        constexpr char PlatformJasper[] = "jasper";
        constexpr char PlatformServer[] = "server";

        constexpr char PlatformCodeNameWindows[] = "Windows";
        constexpr char PlatformCodeNameLinux[] = "Linux";
        constexpr char PlatformCodeNameAndroid[] = "Android";
        constexpr char PlatformCodeNameiOS[] = "iOS";
        constexpr char PlatformCodeNameMac[] = "Mac";
        constexpr char PlatformCodeNameProvo[] = "Provo";
        constexpr char PlatformCodeNameSalem[] = "Salem";
        constexpr char PlatformCodeNameJasper[] = "Jasper";
        constexpr char PlatformAll[] = "all";
        constexpr char PlatformAllClient[] = "all_client";

        // Used for the capacity of a fixed vector to store the code names of platforms
        // The value needs to be higher than the number of unique OS platforms that are supported(at this time 8)
        constexpr size_t MaxPlatformCodeNames = 16;

        //! This platform enum have platform values in sequence and can also be used to get the platform count.
        AZ_ENUM_WITH_UNDERLYING_TYPE(PlatformId, int,
            (Invalid, -1),
            PC,
            LINUX_ID,
            ANDROID_ID,
            IOS,
            MAC_ID,
            PROVO,
            SALEM,
            JASPER,
            SERVER, // Corresponds to the customer's flavor of "server" which could be windows, ubuntu, etc
            ALL,
            ALL_CLIENT,

            // Add new platforms above this
            NumPlatformIds
        );
        constexpr int NumClientPlatforms = 8;
        constexpr int NumPlatforms = NumClientPlatforms + 1; // 1 "Server" platform currently
        enum class PlatformFlags : AZ::u32
        {
            Platform_NONE = 0x00,
            Platform_PC = 1 << PlatformId::PC,
            Platform_LINUX = 1 << PlatformId::LINUX_ID,
            Platform_ANDROID = 1 << PlatformId::ANDROID_ID,
            Platform_IOS = 1 << PlatformId::IOS,
            Platform_MAC = 1 << PlatformId::MAC_ID,
            Platform_PROVO = 1 << PlatformId::PROVO,
            Platform_SALEM = 1 << PlatformId::SALEM,
            Platform_JASPER = 1 << PlatformId::JASPER,
            Platform_SERVER = 1 << PlatformId::SERVER,

            // A special platform that will always correspond to all platforms, even if new ones are added
            Platform_ALL = 1ULL << 30,

            // A special platform that will always correspond to all non-server platforms, even if new ones are added
            Platform_ALL_CLIENT = 1ULL << 31,

            AllNamedPlatforms = Platform_PC | Platform_LINUX | Platform_ANDROID | Platform_IOS | Platform_MAC | Platform_PROVO | Platform_SALEM | Platform_JASPER | Platform_SERVER,

            UnrestrictedPlatforms = Platform_PC | Platform_LINUX | Platform_ANDROID | Platform_IOS | Platform_MAC | Platform_SERVER,
        };

        AZ_DEFINE_ENUM_BITWISE_OPERATORS(PlatformFlags);

        // 32 characters should be more than enough to store a platform name
        using AssetPlatformFixedString = AZStd::fixed_string<32>;
        // Fixed string which can store a comma separated list of platforms names
        // Additional byte is added to take into account the comma
        using AssetPlatformCombinedString = AZStd::fixed_string < (AssetPlatformFixedString{}.max_size() + 1)* PlatformId::NumPlatformIds > ;

        const char* PlatformIdToPalFolder(PlatformId platform);

        const char* OSPlatformToDefaultAssetPlatform(AZStd::string_view osPlatform);

        //! Platform Helper is an utility class that can be used to retrieve platform related information
        class PlatformHelper
        {
        public:

            //! Given a platformIndex returns the platform name
            static const char* GetPlatformName(PlatformId platform);

            //! Converts the platform name to the platform code names as defined in AZ_TRAIT_OS_PLATFORM_CODENAME.
            static void AppendPlatformCodeNames(AZStd::fixed_vector<AZStd::string_view, MaxPlatformCodeNames>& platformCodes, AZStd::string_view platformName);

            //! Converts the platform name to the platform code names as defined in AZ_TRAIT_OS_PLATFORM_CODENAME.
            static void AppendPlatformCodeNames(AZStd::fixed_vector<AZStd::string_view, MaxPlatformCodeNames>& platformCodes, PlatformId platformId);

            //! Given a platform name returns a platform index. 
            //! If the platform is not found, the method returns -1. 
            static int GetPlatformIndexFromName(AZStd::string_view platformName);

            //! Given a platform name returns a platform id. 
            //! If the platform is not found, the method returns -1. 
            static PlatformId GetPlatformIdFromName(AZStd::string_view platformName);

            //! Given a platformIndex returns the platformFlags
            static PlatformFlags GetPlatformFlagFromPlatformIndex(PlatformId platform);

            //! Given a platformFlags returns all the platform identifiers that are set.
            static AZStd::fixed_vector<AZStd::string_view, PlatformId::NumPlatformIds> GetPlatforms(PlatformFlags platformFlags);
            //! Given a platformFlags returns all the platform identifiers that are set, with special flags interpreted.  Do not use the result for saving
            static AZStd::fixed_vector<AZStd::string_view, PlatformId::NumPlatformIds> GetPlatformsInterpreted(PlatformFlags platformFlags);

            //! Given a platformFlags return a list of PlatformId indices
            static AZStd::fixed_vector<PlatformId, PlatformId::NumPlatformIds> GetPlatformIndices(PlatformFlags platformFlags);
            //! Given a platformFlags return a list of PlatformId indices, with special flags interpreted.  Do not use the result for saving
            static AZStd::fixed_vector<PlatformId, PlatformId::NumPlatformIds> GetPlatformIndicesInterpreted(PlatformFlags platformFlags);

            //! Given a platform identifier returns its corresponding platform flag.
            static PlatformFlags GetPlatformFlag(AZStd::string_view platform);

            //! Given any platformFlags returns a string listing the input platforms
            static AssetPlatformCombinedString GetCommaSeparatedPlatformList(PlatformFlags platformFlags);

            //! If platformFlags contains any special flags, they are removed and replaced with the normal flags they represent
            static PlatformFlags GetPlatformFlagsInterpreted(PlatformFlags platformFlags);

            //! Returns true if platformFlags contains any special flags
            static bool IsSpecialPlatform(PlatformFlags platformFlags);

            //! Returns true if platformFlags has checkPlatform flag set.
            static bool HasPlatformFlag(PlatformFlags platformFlags, PlatformId checkPlatform);
        };
    }
}
#pragma pop_macro("IOS")
