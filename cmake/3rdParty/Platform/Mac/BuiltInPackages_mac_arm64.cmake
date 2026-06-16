#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# this file allows you to specify all 3p packages (provided by O3DE or the operating system) for Mac.

# shared by other platforms:
ly_associate_package(PACKAGE_NAME RapidJSON-1.1.0-rev1-multiplatform                TARGETS RapidJSON                   PACKAGE_HASH 2f5e26ecf86c3b7a262753e7da69ac59928e78e9534361f3d00c1ad5879e4023)
ly_associate_package(PACKAGE_NAME RapidXML-1.13-rev1-multiplatform                  TARGETS RapidXML                    PACKAGE_HASH 4b7b5651e47cfd019b6b295cc17bb147b65e53073eaab4a0c0d20a37ab74a246)
ly_associate_package(PACKAGE_NAME pybind11-2.10.0-rev1-multiplatform                TARGETS pybind11                    PACKAGE_HASH 6690acc531d4b8cd453c19b448e2fb8066b2362cbdd2af1ad5df6e0019e6c6c4)
ly_associate_package(PACKAGE_NAME cityhash-1.1-rev1-mac-arm64                       TARGETS cityhash                    PACKAGE_HASH c5844582b4fe819e74ca923dbb58405dd687a4b9acb82d7de04e3e766addb4ed)
ly_associate_package(PACKAGE_NAME zstd-1.35-rev1-mac-arm64                          TARGETS zstd                        PACKAGE_HASH bb401d198d9fd2be2669acb6fe8dbe59fd7d33a66b5f2fd7d3e0221e5b72d1f4)
ly_associate_package(PACKAGE_NAME glad-2.0.0-beta-rev2-multiplatform                TARGETS glad                        PACKAGE_HASH ff97ee9664e97d0854b52a3734c2289329d9f2b4cd69478df6d0ca1f1c9392ee)
ly_associate_package(PACKAGE_NAME xxhash-0.7.4-rev1-multiplatform                   TARGETS xxhash                      PACKAGE_HASH e81f3e6c4065975833996dd1fcffe46c3cf0f9e3a4207ec5f4a1b564ba75861e)

# platform-specific:
ly_associate_package(PACKAGE_NAME expat-2.7.3-rev1-mac-arm64                        TARGETS expat                       PACKAGE_HASH 76a6793f180f6df456394d02d9a23df585af6a10689308e539b50e26c5edf437)
ly_associate_package(PACKAGE_NAME assimp-5.4.3-rev4-mac-arm64                       TARGETS assimp                      PACKAGE_HASH 5c8ca84757e14b581a508e58304ff5133f01d160182a1bfbb02c4a6f9c501dbd)
ly_associate_package(PACKAGE_NAME DirectXShaderCompilerDxc-1.8.2505.1-o3de-rev4-mac-arm64 TARGETS DirectXShaderCompilerDxc    PACKAGE_HASH 75a9c9c9bad393f6571737def7b8d8a09c00e02e415680e8c0d1652459740676)
ly_associate_package(PACKAGE_NAME SPIRVCross-1.3.275.0-rev2-mac-arm64               TARGETS SPIRVCross                  PACKAGE_HASH a7068d9759888eeb81d4cfc6b3d6331ecaeea90bd37fb0f014fa9935a33b034a)
ly_associate_package(PACKAGE_NAME tiff-4.2.0.15-rev3-mac-arm64                      TARGETS TIFF                        PACKAGE_HASH bffbf8bf099ae5d3d49967536a8fcd7fcf747fd6fa92ba945a0e64eead9636d9)
ly_associate_package(PACKAGE_NAME freetype-2.11.1-rev1-mac-arm64                    TARGETS Freetype                    PACKAGE_HASH eae257c78c2da47ca02ca17e949c665c28a59215d756c137c87220c85a7f8488)
ly_associate_package(PACKAGE_NAME AWSNativeSDK-1.11.361-rev1-mac-arm64              TARGETS AWSNativeSDK                PACKAGE_HASH 88fb6ac72314b5993e2c24d90bd409016657658711996f416875ea3a0118a521)
ly_associate_package(PACKAGE_NAME Lua-5.4.4-rev2-mac-arm64                          TARGETS Lua                         PACKAGE_HASH f2089b3d513e614242be6dc4169ed7cb64d502668f0edae72f10815e494b1dbf)
ly_associate_package(PACKAGE_NAME mcpp-2.7.2_az.2-rev3-mac-arm64                    TARGETS mcpp                        PACKAGE_HASH 2c1e7d4154ebf26a35dea1cf9f9cc001aa68f82cbcba8f02f0cb877a07757d36)
ly_associate_package(PACKAGE_NAME mikkelsen-1.0.0.5-mac-arm64                  TARGETS mikkelsen                        PACKAGE_HASH 835f1ec2df64c4046a2f51bff76e72920b1773b504238b4a28b5e8c11ed40135)
ly_associate_package(PACKAGE_NAME googlebenchmark-1.7.0-rev2-mac-arm64              TARGETS GoogleBenchmark             PACKAGE_HASH c33706b0e495aa17ae8c169a708fdf7ec5b76ec396e63a469ec059bd05e79d7c)
ly_associate_package(PACKAGE_NAME openimageio-opencolorio-2.3.17-rev3-mac-arm64     TARGETS OpenImageIO OpenColorIO OpenColorIO::Runtime OpenImageIO::Tools::Binaries OpenImageIO::Tools::PythonPlugins PACKAGE_HASH 9cbbf7e66f3890af8eadac23076fe26d644f67d79f8fd5823678dde1d3af98bb)
ly_associate_package(PACKAGE_NAME OpenSSL-1.1.1w-rev1-mac-arm64                     TARGETS OpenSSL                     PACKAGE_HASH 3367bdf98e73cf2413eb495853972aa4ccd29c2ef58392fa7b7fa99001b1e2e0)
ly_associate_package(PACKAGE_NAME OpenEXR-3.4.4-rev1-mac-arm64                      TARGETS OpenEXR Imath               PACKAGE_HASH 4a093f5ca03836631dc66166b8f493925d0445467219efcbca3a5a0ee2ccbf4b)
ly_associate_package(PACKAGE_NAME qt-6.10.2-rev6-mac-arm64                          TARGETS Qt                          PACKAGE_HASH 48c698e2526ac31a0bd995e71e00a244b059557fbaa521b97edc0af76801cead)
ly_associate_package(PACKAGE_NAME png-1.6.53-rev2-mac-arm64                         TARGETS PNG                         PACKAGE_HASH e778f60475c9582e840b73543d5081c8e4e8a4115badfe3db394d46f6dc87496)
ly_associate_package(PACKAGE_NAME pyside6-6.10.2-py3.10-rev4-mac-arm64              TARGETS pyside6                     PACKAGE_HASH 9584521424003da60397d527217066763814001a18f57371795a3eb3bb0b2edf)
ly_associate_package(PACKAGE_NAME libsamplerate-0.2.1-rev2-mac-arm64                TARGETS libsamplerate               PACKAGE_HASH 1a4954bd2e24b04da6c121e36fde1884e1e3f9492f580cf347637d0bea4b65e0)
ly_associate_package(PACKAGE_NAME zlib-1.3.1-rev2-mac-arm64                         TARGETS ZLIB                        PACKAGE_HASH 52e62890329d3e003226fca88df30701cdd862a5f137eb5f75dff504377c13b3)
ly_associate_package(PACKAGE_NAME squish-ccr-deb557d-rev1-mac-arm64                 TARGETS squish-ccr                  PACKAGE_HASH 51346fba3ba2380cfe82d6af9e2e9284ccdfd6093349e9de88078c52c28c6327)
ly_associate_package(PACKAGE_NAME astc-encoder-3.2-rev6-mac-arm64                   TARGETS astc-encoder                PACKAGE_HASH a416c2d3b6d77c0c971691f5aef19bee2cade464cd4d368ebec40bb58eeb2a41)
ly_associate_package(PACKAGE_NAME ISPCTexComp-36b80aa-rev1-mac-arm64                TARGETS ISPCTexComp                 PACKAGE_HASH 0992e6662f193379cdc9ba8ab9b7a24404564df9bcc5f39d9527b7258ae4172c)
ly_associate_package(PACKAGE_NAME lz4-1.9.4-rev2-mac-arm64                          TARGETS lz4                         PACKAGE_HASH d85fe35ce176967199fe6e11fce684e6c05f0c5533892a3785a458872a1d5229)
ly_associate_package(PACKAGE_NAME azslc-1.8.22-rev1-mac-arm64                       TARGETS azslc                       PACKAGE_HASH ff7c0bb755ae1fc7f2f5e2b02bb4ddfdf85deea5b22ba2f8baae4ff7b0fc8374)
ly_associate_package(PACKAGE_NAME SQLite-3.37.2-rev2-mac-arm64                      TARGETS SQLite                      PACKAGE_HASH 6fa05df3f97fed97bdef293ac85b250ffe443a43e776ad54312b7b356d41fccb)
