/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/PlatformId/PlatformDefaults.h>

#include <AzCore/StringFunc/StringFunc.h>

namespace AZ
{
    inline namespace PlatformDefaults
    {
        static const char* PlatformNames[PlatformId::NumPlatformIds] = { PlatformPC, PlatformLinux, PlatformAndroid, PlatformIOS, PlatformMac, PlatformProvo, PlatformSalem, PlatformJasper, PlatformServer, PlatformAll, PlatformAllClient };

        const char* PlatformIdToPalFolder(AZ::PlatformId platform)
        {
#ifdef IOS
#define AZ_REDEFINE_IOS_AT_END IOS
#undef IOS
#endif
            switch (platform)
            {
            case AZ::PC:
                return "PC";
            case AZ::LINUX_ID:
                return "Linux";
            case AZ::ANDROID_ID:
                return "Android";
            case AZ::IOS:
                return "iOS";
            case AZ::MAC_ID:
                return "Mac";
            case AZ::PROVO:
                return "Provo";
            case AZ::SALEM:
                return "Salem";
            case AZ::JASPER:
                return "Jasper";
            case AZ::SERVER:
                return "Server";
            case AZ::ALL:
            case AZ::ALL_CLIENT:
            case AZ::NumPlatformIds:
            case AZ::Invalid:
            default:
                return "";
            }

#ifdef AZ_REDEFINE_IOS_AT_END
#define IOS AZ_REDEFINE_IOS_AT_END
#endif
        }

        const char* OSPlatformToDefaultAssetPlatform(AZStd::string_view osPlatform)
        {
            if (osPlatform == PlatformCodeNameWindows)
            {
                return PlatformPC;
            }
            if (osPlatform == PlatformCodeNameLinux)
            {
                return PlatformLinux;
            }
            else if (osPlatform == PlatformCodeNameMac)
            {
                return PlatformMac;
            }
            else if (osPlatform == PlatformCodeNameAndroid)
            {
                return PlatformAndroid;
            }
            else if (osPlatform == PlatformCodeNameiOS)
            {
                return PlatformIOS;
            }
            else if (osPlatform == PlatformCodeNameProvo)
            {
                return PlatformProvo;
            }
            else if (osPlatform == PlatformCodeNameSalem)
            {
                return PlatformSalem;
            }
            else if (osPlatform == PlatformCodeNameJasper)
            {
                return PlatformJasper;
            }

            AZ_Error("PlatformDefault", false, R"(Supplied OS platform "%.*s" does not have a corresponding default asset platform)",
                aznumeric_cast<int>(osPlatform.size()), osPlatform.data());
            return "";
        }

        PlatformFlags PlatformHelper::GetPlatformFlagFromPlatformIndex(PlatformId platformIndex)
        {
            if (platformIndex < 0 || platformIndex > PlatformId::NumPlatformIds)
            {
                return PlatformFlags::Platform_NONE;
            }
            if (platformIndex == PlatformId::ALL)
            {
                return PlatformFlags::Platform_ALL;
            }
            if (platformIndex == PlatformId::ALL_CLIENT)
            {
                return PlatformFlags::Platform_ALL_CLIENT;
            }
            return static_cast<PlatformFlags>(1 << platformIndex);
        }

        AZStd::fixed_vector<AZStd::string_view, PlatformId::NumPlatformIds> PlatformHelper::GetPlatforms(PlatformFlags platformFlags)
        {
            AZStd::fixed_vector<AZStd::string_view, PlatformId::NumPlatformIds> platforms;
            for (int platformNum = 0; platformNum < PlatformId::NumPlatformIds; ++platformNum)
            {
                const bool isAllPlatforms = PlatformId::ALL == static_cast<PlatformId>(platformNum)
                    && ((platformFlags & PlatformFlags::Platform_ALL) != PlatformFlags::Platform_NONE);

                const bool isAllClientPlatforms = PlatformId::ALL_CLIENT == static_cast<PlatformId>(platformNum)
                    && ((platformFlags & PlatformFlags::Platform_ALL_CLIENT) != PlatformFlags::Platform_NONE);

                if (isAllPlatforms || isAllClientPlatforms
                    || (platformFlags & static_cast<PlatformFlags>(1 << platformNum)) != PlatformFlags::Platform_NONE)
                {
                    platforms.push_back(PlatformNames[platformNum]);
                }
            }

            return platforms;
        }

        AZStd::fixed_vector<AZStd::string_view, PlatformId::NumPlatformIds> PlatformHelper::GetPlatformsInterpreted(PlatformFlags platformFlags)
        {
            return GetPlatforms(GetPlatformFlagsInterpreted(platformFlags));
        }

        AZStd::fixed_vector<PlatformId, PlatformId::NumPlatformIds> PlatformHelper::GetPlatformIndices(PlatformFlags platformFlags)
        {
            AZStd::fixed_vector<PlatformId, PlatformId::NumPlatformIds> platformIndices;
            for (int i = 0; i < PlatformId::NumPlatformIds; i++)
            {
                PlatformId index = static_cast<PlatformId>(i);
                if ((GetPlatformFlagFromPlatformIndex(index) & platformFlags) != PlatformFlags::Platform_NONE)
                {
                    platformIndices.emplace_back(index);
                }
            }
            return platformIndices;
        }

        AZStd::fixed_vector<PlatformId, PlatformId::NumPlatformIds> PlatformHelper::GetPlatformIndicesInterpreted(PlatformFlags platformFlags)
        {
            return GetPlatformIndices(GetPlatformFlagsInterpreted(platformFlags));
        }

        PlatformFlags PlatformHelper::GetPlatformFlag(AZStd::string_view platform)
        {
            int platformIndex = GetPlatformIndexFromName(platform);
            if (platformIndex == PlatformId::Invalid)
            {
                AZ_Error("PlatformDefault", false, "Invalid Platform ( %.*s ).\n", static_cast<int>(platform.length()), platform.data());
                return PlatformFlags::Platform_NONE;
            }

            if (platformIndex == PlatformId::ALL)
            {
                return PlatformFlags::Platform_ALL;
            }

            if (platformIndex == PlatformId::ALL_CLIENT)
            {
                return PlatformFlags::Platform_ALL_CLIENT;
            }

            return static_cast<PlatformFlags>(1 << platformIndex);
        }

        const char* PlatformHelper::GetPlatformName(PlatformId platform)
        {
            if (platform < 0 || platform > PlatformId::NumPlatformIds)
            {
                return "invalid";
            }
            return PlatformNames[platform];
        }

        void PlatformHelper::AppendPlatformCodeNames(AZStd::fixed_vector<AZStd::string_view, MaxPlatformCodeNames>& platformCodes, AZStd::string_view platformId)
        {
            PlatformId platform = GetPlatformIdFromName(platformId);
            AZ_Assert(platform != PlatformId::Invalid, "Unsupported Platform ID: %.*s", static_cast<int>(platformId.length()), platformId.data());
            AppendPlatformCodeNames(platformCodes, platform);
        }

        void PlatformHelper::AppendPlatformCodeNames(AZStd::fixed_vector<AZStd::string_view, MaxPlatformCodeNames>& platformCodes, PlatformId platformId)
        {
            // The IOS SDK has a macro that defines IOS as 1 which causes the enum below to be incorrectly converted to "PlatformId::1".
#pragma push_macro("IOS")
#undef IOS
        // To reduce work the Asset Processor groups assets that can be shared between hardware platforms together. For this
        // reason "PC" can for instance cover both the Windows and Linux platforms and "IOS" can cover AppleTV and iOS.
            switch (platformId)
            {
            case PlatformId::PC:
                platformCodes.emplace_back(PlatformCodeNameWindows);
                break;
            case PlatformId::LINUX_ID:
                platformCodes.emplace_back(PlatformCodeNameLinux);
                break;
            case PlatformId::ANDROID_ID:
                platformCodes.emplace_back(PlatformCodeNameAndroid);
                break;
            case PlatformId::IOS:
                platformCodes.emplace_back(PlatformCodeNameiOS);
                break;
            case PlatformId::MAC_ID:
                platformCodes.emplace_back(PlatformCodeNameMac);
                break;
            case PlatformId::PROVO:
                platformCodes.emplace_back(PlatformCodeNameProvo);
                break;
            case PlatformId::SALEM:
                platformCodes.emplace_back(PlatformCodeNameSalem);
                break;
            case PlatformId::JASPER:
                platformCodes.emplace_back(PlatformCodeNameJasper);
                break;
            case PlatformId::SERVER:
                // For 'server' we default to the host
                platformCodes.emplace_back(AZ_TRAIT_OS_PLATFORM_CODENAME);
                break;
            default:
                AZ_Assert(false, "Unsupported Platform ID: %i", platformId);
                break;
            }
#pragma pop_macro("IOS")
        }

        int PlatformHelper::GetPlatformIndexFromName(AZStd::string_view platformName)
        {
            for (int idx = 0; idx < PlatformId::NumPlatformIds; idx++)
            {
                if (platformName == PlatformNames[idx])
                {
                    return idx;
                }
            }

            return PlatformId::Invalid;
        }

        PlatformId PlatformHelper::GetPlatformIdFromName(AZStd::string_view platformName)
        {
            return aznumeric_caster(GetPlatformIndexFromName(platformName));
        }

        AssetPlatformCombinedString PlatformHelper::GetCommaSeparatedPlatformList(PlatformFlags platformFlags)
        {
            AZStd::fixed_vector<AZStd::string_view, PlatformId::NumPlatformIds> platformNames = GetPlatforms(platformFlags);
            AssetPlatformCombinedString platformsString;
            AZ::StringFunc::Join(platformsString, platformNames.begin(), platformNames.end(), ", ");
            return platformsString;
        }

        PlatformFlags PlatformHelper::GetPlatformFlagsInterpreted(PlatformFlags platformFlags)
        {
            PlatformFlags returnFlags = PlatformFlags::Platform_NONE;

            if ((platformFlags & PlatformFlags::Platform_ALL) != PlatformFlags::Platform_NONE)
            {
                for (int i = 0; i < NumPlatforms; ++i)
                {
                    auto platformId = static_cast<PlatformId>(i);

                    if (platformId != PlatformId::ALL && platformId != PlatformId::ALL_CLIENT)
                    {
                        returnFlags |= GetPlatformFlagFromPlatformIndex(platformId);
                    }
                }
            }
            else if ((platformFlags & PlatformFlags::Platform_ALL_CLIENT) != PlatformFlags::Platform_NONE)
            {
                for (int i = 0; i < NumPlatforms; ++i)
                {
                    auto platformId = static_cast<PlatformId>(i);

                    if (platformId != PlatformId::ALL && platformId != PlatformId::ALL_CLIENT && platformId != PlatformId::SERVER)
                    {
                        returnFlags |= GetPlatformFlagFromPlatformIndex(platformId);
                    }
                }
            }
            else
            {
                returnFlags = platformFlags;
            }

            return returnFlags;
        }

        bool PlatformHelper::IsSpecialPlatform(PlatformFlags platformFlags)
        {
            return (platformFlags & PlatformFlags::Platform_ALL) != PlatformFlags::Platform_NONE
                || (platformFlags & PlatformFlags::Platform_ALL_CLIENT) != PlatformFlags::Platform_NONE;
        }

        bool HasFlagHelper(PlatformFlags flags, PlatformFlags checkPlatform)
        {
            return (flags & checkPlatform) == checkPlatform;
        }


        bool PlatformHelper::HasPlatformFlag(PlatformFlags flags, PlatformId checkPlatform)
        {
            // If checkPlatform contains any kind of invalid id, just exit out here
            if (checkPlatform == PlatformId::Invalid || checkPlatform == NumPlatforms)
            {
                return false;
            }

            // ALL_CLIENT + SERVER = ALL
            if (HasFlagHelper(flags, PlatformFlags::Platform_ALL_CLIENT | PlatformFlags::Platform_SERVER))
            {
                flags = PlatformFlags::Platform_ALL;
            }

            if (HasFlagHelper(flags, PlatformFlags::Platform_ALL))
            {
                // It doesn't matter what checkPlatform is set to in this case, just return true
                return true;
            }

            if (HasFlagHelper(flags, PlatformFlags::Platform_ALL_CLIENT))
            {
                return checkPlatform != PlatformId::SERVER;
            }

            return HasFlagHelper(flags, GetPlatformFlagFromPlatformIndex(checkPlatform));
        }
    }
}
