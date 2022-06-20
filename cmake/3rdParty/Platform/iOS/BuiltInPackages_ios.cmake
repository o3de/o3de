#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# this file allows you to specify all 3p packages (provided by O3DE or the operating system) for iOS.

# shared by other platforms:
ly_associate_package(PACKAGE_NAME md5-2.0-multiplatform              TARGETS md5        PACKAGE_HASH 29e52ad22c78051551f78a40c2709594f0378762ae03b417adca3f4b700affdf)
ly_associate_package(PACKAGE_NAME RapidJSON-1.1.0-rev1-multiplatform TARGETS RapidJSON  PACKAGE_HASH 2f5e26ecf86c3b7a262753e7da69ac59928e78e9534361f3d00c1ad5879e4023)
ly_associate_package(PACKAGE_NAME RapidXML-1.13-rev1-multiplatform   TARGETS RapidXML   PACKAGE_HASH 4b7b5651e47cfd019b6b295cc17bb147b65e53073eaab4a0c0d20a37ab74a246)
ly_associate_package(PACKAGE_NAME cityhash-1.1-multiplatform         TARGETS cityhash   PACKAGE_HASH 0ace9e6f0b2438c5837510032d2d4109125845c0efd7d807f4561ec905512dd2)
ly_associate_package(PACKAGE_NAME zstd-1.35-multiplatform            TARGETS zstd       PACKAGE_HASH 45d466c435f1095898578eedde85acf1fd27190e7ea99aeaa9acfd2f09e12665)
ly_associate_package(PACKAGE_NAME glad-2.0.0-beta-rev2-multiplatform TARGETS glad       PACKAGE_HASH ff97ee9664e97d0854b52a3734c2289329d9f2b4cd69478df6d0ca1f1c9392ee)

# platform-specific:
ly_associate_package(PACKAGE_NAME expat-2.4.2-rev2-ios           TARGETS expat           PACKAGE_HASH 7c2635422aec23610f1633d5ed69189eb7704647133e009276c5dc566b5dedc1)
ly_associate_package(PACKAGE_NAME tiff-4.2.0.15-rev3-ios         TARGETS TIFF            PACKAGE_HASH e9067e88649fb6e93a926d9ed38621a9fae360a2e6f6eb24ebca63c1bc7761ea)
ly_associate_package(PACKAGE_NAME freetype-2.11.1-rev1-ios       TARGETS Freetype        PACKAGE_HASH b619eaa7da16469ae455a6418fbcd7bae8be406b69a46f5bc1706b275fb3558f)
ly_associate_package(PACKAGE_NAME AWSNativeSDK-1.9.50-rev1-ios   TARGETS AWSNativeSDK    PACKAGE_HASH c3c9478c259ecb569fb2ce6fcfa733647adc3b6bd2854e8eff9de64bcd18c745)
ly_associate_package(PACKAGE_NAME Lua-5.4.4-rev1-ios             TARGETS Lua             PACKAGE_HASH 82f27bf6c745c98395dcea7ec72f82cb5254fd19fca9f5ac7a6246527a30bacb)
ly_associate_package(PACKAGE_NAME PhysX-4.1.2.29882248-rev5-ios  TARGETS PhysX           PACKAGE_HASH 4a5e38b385837248590018eb133444b4e440190414e6756191200a10c8fa5615)
ly_associate_package(PACKAGE_NAME mikkelsen-1.0.0.4-ios          TARGETS mikkelsen       PACKAGE_HASH 976aaa3ccd8582346132a10af253822ccc5d5bcc9ea5ba44d27848f65ee88a8a)
ly_associate_package(PACKAGE_NAME googletest-1.8.1-rev4-ios      TARGETS googletest      PACKAGE_HASH 2f121ad9784c0ab73dfaa58e1fee05440a82a07cc556bec162eeb407688111a7)
ly_associate_package(PACKAGE_NAME googlebenchmark-1.5.0-rev2-ios TARGETS GoogleBenchmark PACKAGE_HASH c2ffaed2b658892b1bcf81dee4b44cd1cb09fc78d55584ef5cb8ab87f2d8d1ae)
ly_associate_package(PACKAGE_NAME png-1.6.37-rev2-ios            TARGETS PNG             PACKAGE_HASH f6a412a761cf3c3076b73a8115911328917b4b4ea6defc78b328a799b7bbed6d)
ly_associate_package(PACKAGE_NAME libsamplerate-0.2.1-rev2-ios   TARGETS libsamplerate   PACKAGE_HASH 7656b961697f490d4f9c35d2e61559f6fc38c32102e542a33c212cd618fc2119)
ly_associate_package(PACKAGE_NAME OpenSSL-1.1.1o-rev1-ios        TARGETS OpenSSL         PACKAGE_HASH 1325bbb2dcc97fe33d8db71e4b0a61e95e9e77558e3ce6767a1b5d330240f78f)
ly_associate_package(PACKAGE_NAME zlib-1.2.11-rev5-ios           TARGETS ZLIB            PACKAGE_HASH c7f10b4d0fe63192054d926f53b08e852cdf472bc2b18e2f7be5aecac1869f7f)
ly_associate_package(PACKAGE_NAME lz4-1.9.3-vcpkg-rev4-ios       TARGETS lz4             PACKAGE_HASH 588ea05739caa9231a9a17a1e8cf64c5b9a265e16528bc05420af7e2534e86c1)
