#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# this file allows you to specify all 3p packages (provided by O3DE or the operating system) for Emscripten.

# shared by other platforms:
ly_associate_package(PACKAGE_NAME RapidJSON-1.1.0-rev1-multiplatform                TARGETS RapidJSON               PACKAGE_HASH 2f5e26ecf86c3b7a262753e7da69ac59928e78e9534361f3d00c1ad5879e4023)
ly_associate_package(PACKAGE_NAME RapidXML-1.13-rev1-multiplatform                  TARGETS RapidXML                PACKAGE_HASH 4b7b5651e47cfd019b6b295cc17bb147b65e53073eaab4a0c0d20a37ab74a246)
ly_associate_package(PACKAGE_NAME cityhash-1.1-multiplatform                        TARGETS cityhash                PACKAGE_HASH 0ace9e6f0b2438c5837510032d2d4109125845c0efd7d807f4561ec905512dd2)
ly_associate_package(PACKAGE_NAME zstd-1.35-multiplatform                           TARGETS zstd                    PACKAGE_HASH 45d466c435f1095898578eedde85acf1fd27190e7ea99aeaa9acfd2f09e12665)
ly_associate_package(PACKAGE_NAME glad-2.0.0-beta-rev2-multiplatform                TARGETS glad                    PACKAGE_HASH ff97ee9664e97d0854b52a3734c2289329d9f2b4cd69478df6d0ca1f1c9392ee)

# platform-specific:
ly_associate_package(PACKAGE_NAME expat-2.4.2-rev2-emscripten                       TARGETS expat                   PACKAGE_HASH 1a1e4bb25ccaa6033082e4e6562c837ebb274938b01ff368c2c2546c87707047)
ly_associate_package(PACKAGE_NAME Lua-5.4.4-rev1-emscripten                         TARGETS Lua                     PACKAGE_HASH cbead8f644a4906326a9f5437a5941da1b5fed8bb76cb3e2d41812176a20e6ef)
ly_associate_package(PACKAGE_NAME zlib-1.2.11-rev5-emscripten                       TARGETS ZLIB                    PACKAGE_HASH 2f6a82fe4a193d8eb51e92ef4a6de15ec222dc9b4a1802a631f4cdc149bb6a72)
ly_associate_package(PACKAGE_NAME lz4-1.9.4-rev2-emscripten                         TARGETS lz4                     PACKAGE_HASH 1231052cba13575d4f41fc88f227e8169e86bc287b918176cd3226b4da4b4d48)
