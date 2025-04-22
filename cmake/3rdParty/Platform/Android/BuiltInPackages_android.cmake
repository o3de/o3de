#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# this file allows you to specify all 3p packages (provided by O3DE or the operating system) for Android.

# shared by other platforms:
ly_associate_package(PACKAGE_NAME RapidJSON-1.1.0-rev1-multiplatform                TARGETS RapidJSON               PACKAGE_HASH 2f5e26ecf86c3b7a262753e7da69ac59928e78e9534361f3d00c1ad5879e4023)
ly_associate_package(PACKAGE_NAME RapidXML-1.13-rev1-multiplatform                  TARGETS RapidXML                PACKAGE_HASH 4b7b5651e47cfd019b6b295cc17bb147b65e53073eaab4a0c0d20a37ab74a246)
ly_associate_package(PACKAGE_NAME cityhash-1.1-multiplatform                        TARGETS cityhash                PACKAGE_HASH 0ace9e6f0b2438c5837510032d2d4109125845c0efd7d807f4561ec905512dd2)
ly_associate_package(PACKAGE_NAME zstd-1.35-multiplatform                           TARGETS zstd                    PACKAGE_HASH 45d466c435f1095898578eedde85acf1fd27190e7ea99aeaa9acfd2f09e12665)
ly_associate_package(PACKAGE_NAME glad-2.0.0-beta-rev2-multiplatform                TARGETS glad                    PACKAGE_HASH ff97ee9664e97d0854b52a3734c2289329d9f2b4cd69478df6d0ca1f1c9392ee)

# platform-specific:
ly_associate_package(PACKAGE_NAME expat-2.4.2-rev2-android                          TARGETS expat                   PACKAGE_HASH ddeb8e6e0e81714e9addc58d370b3b4360a7ac2935e51f05b00ed2df447d1c21)
ly_associate_package(PACKAGE_NAME tiff-4.2.0.15-rev4-android                        TARGETS TIFF                    PACKAGE_HASH 2c62cdf34a8ee6c7eb091d05d98f60b4da7634c74054d4dbb8736886182f4589)
ly_associate_package(PACKAGE_NAME freetype-2.11.1-rev1-android                      TARGETS Freetype                PACKAGE_HASH 31cd0411425f3d69064849190c3cafb09fc3cdd64507f40bf49342630d76f861)
ly_associate_package(PACKAGE_NAME AWSNativeSDK-1.11.288-rev2-android                TARGETS AWSNativeSDK                PACKAGE_HASH 61620cb4c79f752328e4722fa36020dabe04167d83c94a0d302c08f3efa367b7)
ly_associate_package(PACKAGE_NAME Lua-5.4.4-rev1-android                            TARGETS Lua                     PACKAGE_HASH 2adda1831577336454090f249baf09519f41bb73160cd1d5b5b33564729af4a2)
ly_associate_package(PACKAGE_NAME mikkelsen-1.0.0.4-android                         TARGETS mikkelsen               PACKAGE_HASH 075e8e4940884971063b5a9963014e2e517246fa269c07c7dc55b8cf2cd99705)
ly_associate_package(PACKAGE_NAME googlebenchmark-1.7.0-rev2-android                TARGETS GoogleBenchmark         PACKAGE_HASH 972c1fd1dec3d1cecd7b05f1064e47e20775ea4ba211a08a53c4d1c7c1ccc905)
ly_associate_package(PACKAGE_NAME png-1.6.37-rev2-android                           TARGETS PNG                     PACKAGE_HASH c2240299251d97d963d2e9f4320fbc384bfb2e1b1e073419d1171df0e8ea983d)
ly_associate_package(PACKAGE_NAME libsamplerate-0.2.1-rev2-android                  TARGETS libsamplerate           PACKAGE_HASH bf13662afe65d02bcfa16258a4caa9b875534978227d6f9f36c9cfa92b3fb12b)
ly_associate_package(PACKAGE_NAME OpenSSL-1.1.1o-rev2-android                       TARGETS OpenSSL                 PACKAGE_HASH 28fa781be8fa233e3074b08e5d6d5064d1a5a5cffc80b04e1bb8d407aca459a0)
ly_associate_package(PACKAGE_NAME zlib-1.2.11-rev5-android                          TARGETS ZLIB                    PACKAGE_HASH 73c9e88892c237a3fc6eafc04268ccd9d479e6d55f9df2ed58b236c8f9cf2cae)
ly_associate_package(PACKAGE_NAME lz4-1.9.4-rev1-android                            TARGETS lz4                     PACKAGE_HASH 97a4758f07bea6792dc68d7e9b84952800628bc482b33d9a93e2c52f9e758662)
ly_associate_package(PACKAGE_NAME vulkan-validationlayers-1.3.261-rev1-android      TARGETS vulkan-validationlayers PACKAGE_HASH bb0742fef8e46069e027b57534da71c6cd176d9b3e129f4cf2a2194e3fad75ea)
