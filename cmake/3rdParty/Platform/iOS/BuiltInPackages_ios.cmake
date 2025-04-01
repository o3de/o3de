#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# this file allows you to specify all 3p packages (provided by O3DE or the operating system) for iOS.

# shared by other platforms:
ly_associate_package(PACKAGE_NAME RapidJSON-1.1.0-rev1-multiplatform TARGETS RapidJSON  PACKAGE_HASH 2f5e26ecf86c3b7a262753e7da69ac59928e78e9534361f3d00c1ad5879e4023)
ly_associate_package(PACKAGE_NAME RapidXML-1.13-rev1-multiplatform   TARGETS RapidXML   PACKAGE_HASH 4b7b5651e47cfd019b6b295cc17bb147b65e53073eaab4a0c0d20a37ab74a246)
ly_associate_package(PACKAGE_NAME cityhash-1.1-multiplatform         TARGETS cityhash   PACKAGE_HASH 0ace9e6f0b2438c5837510032d2d4109125845c0efd7d807f4561ec905512dd2)
ly_associate_package(PACKAGE_NAME zstd-1.35-multiplatform            TARGETS zstd       PACKAGE_HASH 45d466c435f1095898578eedde85acf1fd27190e7ea99aeaa9acfd2f09e12665)
ly_associate_package(PACKAGE_NAME glad-2.0.0-beta-rev2-multiplatform TARGETS glad       PACKAGE_HASH ff97ee9664e97d0854b52a3734c2289329d9f2b4cd69478df6d0ca1f1c9392ee)

# platform-specific:
ly_associate_package(PACKAGE_NAME expat-2.4.2-rev2-ios           TARGETS expat           PACKAGE_HASH 7c2635422aec23610f1633d5ed69189eb7704647133e009276c5dc566b5dedc1)
ly_associate_package(PACKAGE_NAME tiff-4.2.0.15-rev3-ios         TARGETS TIFF            PACKAGE_HASH e9067e88649fb6e93a926d9ed38621a9fae360a2e6f6eb24ebca63c1bc7761ea)
ly_associate_package(PACKAGE_NAME freetype-2.11.1-rev1-ios       TARGETS Freetype        PACKAGE_HASH b619eaa7da16469ae455a6418fbcd7bae8be406b69a46f5bc1706b275fb3558f)
ly_associate_package(PACKAGE_NAME AWSNativeSDK-1.11.288-rev2-ios TARGETS AWSNativeSDK                PACKAGE_HASH bea50bcade9f322bdc540bdf84dba9103cb993b6e021c8836cc7777ef4a44015)
ly_associate_package(PACKAGE_NAME Lua-5.4.4-rev1-ios             TARGETS Lua             PACKAGE_HASH 82f27bf6c745c98395dcea7ec72f82cb5254fd19fca9f5ac7a6246527a30bacb)
ly_associate_package(PACKAGE_NAME mikkelsen-1.0.0.4-ios          TARGETS mikkelsen       PACKAGE_HASH 976aaa3ccd8582346132a10af253822ccc5d5bcc9ea5ba44d27848f65ee88a8a)
ly_associate_package(PACKAGE_NAME googlebenchmark-1.7.0-rev1-ios TARGETS GoogleBenchmark PACKAGE_HASH 05addc0d87e5a3f3b0eed50aea107fdead05da232c4212e8c3662a53f15f8b08)
ly_associate_package(PACKAGE_NAME png-1.6.37-rev3-ios            TARGETS PNG             PACKAGE_HASH b476f7d3686a08c4adaec6a6e889db1138483863ddf7b0c7b4fafb553951e87c)
ly_associate_package(PACKAGE_NAME libsamplerate-0.2.1-rev2-ios   TARGETS libsamplerate   PACKAGE_HASH 7656b961697f490d4f9c35d2e61559f6fc38c32102e542a33c212cd618fc2119)
ly_associate_package(PACKAGE_NAME OpenSSL-1.1.1o-rev1-ios        TARGETS OpenSSL         PACKAGE_HASH 1325bbb2dcc97fe33d8db71e4b0a61e95e9e77558e3ce6767a1b5d330240f78f)
ly_associate_package(PACKAGE_NAME zlib-1.2.11-rev5-ios           TARGETS ZLIB            PACKAGE_HASH c7f10b4d0fe63192054d926f53b08e852cdf472bc2b18e2f7be5aecac1869f7f)
ly_associate_package(PACKAGE_NAME lz4-1.9.4-rev1-ios             TARGETS lz4             PACKAGE_HASH 62bc1f8eb8da82ca083ae54423e8cba358e5feb6d0012779b15d4aae0134423c)

set(AZ_USE_PHYSX5 OFF CACHE BOOL "When ON PhysX Gem will use PhysX 5 SDK")
if(AZ_USE_PHYSX5)
    ly_associate_package(PACKAGE_NAME PhysX-5.1.1-rev1-ios           TARGETS PhysX       PACKAGE_HASH 3495e0bc1404a369fb6f4880ffc525c0a9d860867f5c524b33887cfd08f5c376)
else()
    ly_associate_package(PACKAGE_NAME PhysX-4.1.2.29882248-rev5-ios  TARGETS PhysX       PACKAGE_HASH 4a5e38b385837248590018eb133444b4e440190414e6756191200a10c8fa5615)
endif()
