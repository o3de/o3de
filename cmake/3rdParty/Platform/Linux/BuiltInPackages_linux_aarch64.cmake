#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# this file allows you to specify all 3p packages (provided by O3DE or the operating system) for Linux.

# shared by other platforms:
ly_associate_package(PACKAGE_NAME RapidJSON-1.1.0-rev1-multiplatform                TARGETS RapidJSON                   PACKAGE_HASH 2f5e26ecf86c3b7a262753e7da69ac59928e78e9534361f3d00c1ad5879e4023)
ly_associate_package(PACKAGE_NAME RapidXML-1.13-rev1-multiplatform                  TARGETS RapidXML                    PACKAGE_HASH 4b7b5651e47cfd019b6b295cc17bb147b65e53073eaab4a0c0d20a37ab74a246)
ly_associate_package(PACKAGE_NAME pybind11-2.10.0-rev1-multiplatform                TARGETS pybind11                    PACKAGE_HASH 6690acc531d4b8cd453c19b448e2fb8066b2362cbdd2af1ad5df6e0019e6c6c4)
ly_associate_package(PACKAGE_NAME glad-2.0.0-beta-rev2-multiplatform                TARGETS glad                        PACKAGE_HASH ff97ee9664e97d0854b52a3734c2289329d9f2b4cd69478df6d0ca1f1c9392ee)
ly_associate_package(PACKAGE_NAME xxhash-0.7.4-rev1-multiplatform                   TARGETS xxhash                      PACKAGE_HASH e81f3e6c4065975833996dd1fcffe46c3cf0f9e3a4207ec5f4a1b564ba75861e)

# platform-specific:
ly_associate_package(PACKAGE_NAME expat-2.4.2-rev2-linux-aarch64                             TARGETS expat                       PACKAGE_HASH 934a535c1492d11906789d7ddf105b1a530cf8d8fb126063ffde16c5caeb0179)
ly_associate_package(PACKAGE_NAME assimp-5.4.3-rev3-linux-aarch64                            TARGETS assimp                      PACKAGE_HASH e8e434eddd6ef418c38ce69c243b751c800d02e7691fe779144f5d5cb8716732)
ly_associate_package(PACKAGE_NAME AWSNativeSDK-1.11.288-rev1-linux-aarch64                   TARGETS AWSNativeSDK                PACKAGE_HASH 55791343d3aaa07a4242190cc9d6f1f2448c55125839255bdec25efdbab46efa)
ly_associate_package(PACKAGE_NAME cityhash-1.1-rev1-linux-aarch64                            TARGETS cityhash                    PACKAGE_HASH c4fafa13add81c6ca03338462af78eabbdea917de68c599f11c4a36b0982cec2)
ly_associate_package(PACKAGE_NAME tiff-4.2.0.15-rev3-linux-aarch64                           TARGETS TIFF                        PACKAGE_HASH 429461014b21a530dcad597c2d91072ae39d937a04b7bbbf5c34491c41767f7f)
ly_associate_package(PACKAGE_NAME freetype-2.11.1-rev1-linux-aarch64                         TARGETS Freetype                    PACKAGE_HASH b4e3069acdcdae2f977108679d0986fb57371b9a7d4a3a496ab16909feabcba6)
ly_associate_package(PACKAGE_NAME Lua-5.4.4-rev1-linux-aarch64                               TARGETS Lua                         PACKAGE_HASH 4d30067fc494ac27acd72b0bf18099c19c0a44ac9bd46b23db66ad780e72374a)
ly_associate_package(PACKAGE_NAME mcpp-2.7.2_az.1-rev1-linux-aarch64                         TARGETS mcpp                        PACKAGE_HASH 817d31b94d1217b6e47bd5357b3a07a79ab6aa93452c65ff56831d0590c5169d)
ly_associate_package(PACKAGE_NAME mikkelsen-1.0.0.4-linux-aarch64                            TARGETS mikkelsen                   PACKAGE_HASH 62f3f316c971239a2b86d7c47a68fee9be744de3a4f9b00533b32f33a4764f8b)
ly_associate_package(PACKAGE_NAME googlebenchmark-1.7.0-rev1-linux-aarch64                   TARGETS GoogleBenchmark             PACKAGE_HASH 06fbfeaba2aeae20197da631019e52105dc1f69e702151a76c6aba2c27c03acb)
ly_associate_package(PACKAGE_NAME qt-5.15.2-rev9-linux-aarch64                               TARGETS Qt                          PACKAGE_HASH da80840ecd3f7a074edecbb3dedb1ff36c568cfe4943e18d9559e9fca9f151bc)
ly_associate_package(PACKAGE_NAME png-1.6.37-rev2-linux-aarch64                              TARGETS PNG                         PACKAGE_HASH fcf646c1b1b4163000efdb56d7c8f086b6ce0a520da5c8d3ffce4e1329ae798a)
ly_associate_package(PACKAGE_NAME libsamplerate-0.2.1-rev2-linux-aarch64                     TARGETS libsamplerate               PACKAGE_HASH 751484da1527432cd19263909f69164d67b25644f87ec1d4ec974a343defacea)
ly_associate_package(PACKAGE_NAME openimageio-opencolorio-2.3.17-rev2-linux-aarch64          TARGETS OpenImageIO OpenColorIO OpenColorIO::Runtime OpenImageIO::Tools::Binaries OpenImageIO::Tools::PythonPlugins PACKAGE_HASH 2bc6a43f60c8206b2606a65738e0fcf3b3b17e0db16089404d8389d337c85ad6)
ly_associate_package(PACKAGE_NAME OpenMesh-8.1-rev3-linux-aarch64                            TARGETS OpenMesh                    PACKAGE_HASH 0d53d215c4b2185879e3b27d1a4bdf61a53bcdb059eae30377ea4573bcd9ebc1)
ly_associate_package(PACKAGE_NAME OpenEXR-3.1.3-rev4-linux-aarch64                           TARGETS OpenEXR Imath               PACKAGE_HASH c9a81050f0d550ab03d2f5801e2f67f9f02747c26f4b39647e9919278585ad6a)
ly_associate_package(PACKAGE_NAME OpenSSL-1.1.1t-rev1-linux-aarch64                          TARGETS OpenSSL                     PACKAGE_HASH f32721bec9c82d1bd7fb244d78d5dc4e2a47e7b808bb36027236ad377e241ea5)
ly_associate_package(PACKAGE_NAME DirectXShaderCompilerDxc-1.7.2308-o3de-rev1-linux-aarch64  TARGETS DirectXShaderCompilerDxc    PACKAGE_HASH 226eaf982fa182c3460e79a00edfa19d0df6144f089ee1746adfaa24f1f4e5d0)
ly_associate_package(PACKAGE_NAME SPIRVCross-1.3.275.0-rev1-linux-aarch64                    TARGETS SPIRVCross                  PACKAGE_HASH 8cd6e4b26202d657e221c4513916ba82b17c73caf1533c8dd833bce6c5c88c2b)
ly_associate_package(PACKAGE_NAME azslc-1.8.22-rev1-linux-aarch64                            TARGETS azslc                       PACKAGE_HASH 5f7c59e4991a22439bbe4af8deab079ec056f90bdd3642eb417c72a15613ecc9)
ly_associate_package(PACKAGE_NAME zlib-1.2.11-rev5-linux-aarch64                             TARGETS ZLIB                        PACKAGE_HASH ce9d1ed2883d77ffc69c7982c078595c1f89ca55ec19d89fe7e6beb05f774775)
ly_associate_package(PACKAGE_NAME squish-ccr-deb557d-rev1-linux-aarch64                      TARGETS squish-ccr                  PACKAGE_HASH d3e54df2defff9f9254085acbf7c61dfda56f72ad10d34e1dd3b5d1bd2b8129f)
ly_associate_package(PACKAGE_NAME astc-encoder-3.2-rev3-linux-aarch64                        TARGETS astc-encoder                PACKAGE_HASH 60ef2a8adc15767dc263860e1e3befc2f3acea26987442a7e80783f1b2158c73)
ly_associate_package(PACKAGE_NAME ISPCTexComp-36b80aa-rev2-linux-aarch64                     TARGETS ISPCTexComp                 PACKAGE_HASH c29aafa32f13839a394424cf674b5cdb323fab22bcca43c38b43adfe13fc415c)
ly_associate_package(PACKAGE_NAME lz4-1.9.4-rev2-linux-aarch64                               TARGETS lz4                         PACKAGE_HASH 725ca4a02bcf961dc68fb525d0509c311536b5a0f0f9885244fab282cdc55d1f)
ly_associate_package(PACKAGE_NAME pyside2-5.15.2.1-py3.10-rev7-linux-aarch64                 TARGETS pyside2                     PACKAGE_HASH 3210d697299d9c943ac4dfddb95513a7781d8505da0f241f445bd15101529e69)
ly_associate_package(PACKAGE_NAME SQLite-3.37.2-rev1-linux-aarch64                           TARGETS SQLite                      PACKAGE_HASH 5cc1fd9294af72514eba60509414e58f1a268996940be31d0ab6919383f05118)
ly_associate_package(PACKAGE_NAME AwsIotDeviceSdkCpp-1.15.2-rev1-linux-aarch64               TARGETS AwsIotDeviceSdkCpp          PACKAGE_HASH 0bac80fc09094c4fd89a845af57ebe4ef86ff8d46e92a448c6986f9880f9ee62)
ly_associate_package(PACKAGE_NAME vulkan-validationlayers-1.2.198-rev1-linux-aarch64         TARGETS vulkan-validationlayers     PACKAGE_HASH e67a15a95e14397ccdffd70d17f61079e5720fea22b0d21e135497312419a23f)
