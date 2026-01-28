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
ly_associate_package(PACKAGE_NAME cityhash-1.1-multiplatform                        TARGETS cityhash                    PACKAGE_HASH 0ace9e6f0b2438c5837510032d2d4109125845c0efd7d807f4561ec905512dd2)
ly_associate_package(PACKAGE_NAME zstd-1.35-multiplatform                           TARGETS zstd                        PACKAGE_HASH 45d466c435f1095898578eedde85acf1fd27190e7ea99aeaa9acfd2f09e12665)
ly_associate_package(PACKAGE_NAME glad-2.0.0-beta-rev2-multiplatform                TARGETS glad                        PACKAGE_HASH ff97ee9664e97d0854b52a3734c2289329d9f2b4cd69478df6d0ca1f1c9392ee)
ly_associate_package(PACKAGE_NAME xxhash-0.7.4-rev1-multiplatform                   TARGETS xxhash                      PACKAGE_HASH e81f3e6c4065975833996dd1fcffe46c3cf0f9e3a4207ec5f4a1b564ba75861e)

# platform-specific:
ly_associate_package(PACKAGE_NAME expat-2.7.3-rev1-mac-arm64                        TARGETS expat                       PACKAGE_HASH 76a6793f180f6df456394d02d9a23df585af6a10689308e539b50e26c5edf437)
ly_associate_package(PACKAGE_NAME assimp-5.4.3-rev3-mac-arm64                       TARGETS assimp                      PACKAGE_HASH bfda2c319bb4cc26aea8445a9ad33347e0e5ead2a959a5eafb5eed47431f56ef)
ly_associate_package(PACKAGE_NAME DirectXShaderCompilerDxc-1.8.2505.1-o3de-rev4-mac-arm64 TARGETS DirectXShaderCompilerDxc    PACKAGE_HASH 75a9c9c9bad393f6571737def7b8d8a09c00e02e415680e8c0d1652459740676)
ly_associate_package(PACKAGE_NAME SPIRVCross-1.3.275.0-rev1-mac-arm64               TARGETS SPIRVCross                  PACKAGE_HASH 5ad9629f677c42847daf8b097728323685d7018d3ac8af0508d1bd0727a81304)
ly_associate_package(PACKAGE_NAME tiff-4.2.0.15-rev3-mac-arm64                      TARGETS tiff                        PACKAGE_HASH bffbf8bf099ae5d3d49967536a8fcd7fcf747fd6fa92ba945a0e64eead9636d9)
ly_associate_package(PACKAGE_NAME freetype-2.11.1-rev1-mac-arm64                    TARGETS freetype                    PACKAGE_HASH eae257c78c2da47ca02ca17e949c665c28a59215d756c137c87220c85a7f8488)
ly_associate_package(PACKAGE_NAME AWSNativeSDK-1.11.361-rev1-mac-arm64              TARGETS AWSNativeSDK                PACKAGE_HASH 88fb6ac72314b5993e2c24d90bd409016657658711996f416875ea3a0118a521)
ly_associate_package(PACKAGE_NAME Lua-5.4.4-rev1-mac-arm64                          TARGETS Lua                         PACKAGE_HASH b44daae6bfdf092c7935e4aebafded6772853250c6f0a209866a1ac599857d58)
ly_associate_package(PACKAGE_NAME mcpp-2.7.2_az.2-rev1-mac-arm64                    TARGETS mcpp                        PACKAGE_HASH be9558905c9c49179ef3d7d84f0a5472415acdf7fe2d76eb060d9431723ddf2e)
ly_associate_package(PACKAGE_NAME mikkelsen-1.0.0.4-mac-arm64                       TARGETS mikkelsen                   PACKAGE_HASH 83af99ca8bee123684ad254263add556f0cf49486c0b3e32e6d303535714e505)
ly_associate_package(PACKAGE_NAME googlebenchmark-1.7.0-rev1-mac-arm64              TARGETS GoogleBenchmark             PACKAGE_HASH a1c8793eb1760905290065929b45600a4b4457345fcc129fce253d1a8980bbce)
ly_associate_package(PACKAGE_NAME openimageio-opencolorio-2.3.17-rev3-mac-arm64     TARGETS OpenImageIO OpenColorIO OpenColorIO::Runtime OpenImageIO::Tools::Binaries OpenImageIO::Tools::PythonPlugins PACKAGE_HASH bc322f9e28d519ab5959a638b38ee3b773fefb868802823fad2396ab4f7bcbc8)
ly_associate_package(PACKAGE_NAME OpenSSL-1.1.1o-rev1-mac-arm64                     TARGETS OpenSSL                     PACKAGE_HASH 73a4bd7856b53edf5ab9d2ff1d31ebb02301be818680a59206ce8ec5940f3468)
ly_associate_package(PACKAGE_NAME OpenEXR-3.4.4-rev1-mac-arm64                      TARGETS OpenEXR                     PACKAGE_HASH 4a093f5ca03836631dc66166b8f493925d0445467219efcbca3a5a0ee2ccbf4b)
ly_associate_package(PACKAGE_NAME qt-5.15.2-rev8-mac-arm64                          TARGETS Qt                          PACKAGE_HASH d0f97579ea2822c73f0b316a26c68ceb5332763e691d7e78d6b02fe3104b1d31)
ly_associate_package(PACKAGE_NAME png-1.6.37-rev2-mac-arm64                         TARGETS PNG                         PACKAGE_HASH 515252226a6958c459f53d8598d80ec4f90df33d2f1637104fd1a636f4962f07)
ly_associate_package(PACKAGE_NAME libsamplerate-0.2.1-rev2-mac-arm64                TARGETS libsamplerate               PACKAGE_HASH 1a4954bd2e24b04da6c121e36fde1884e1e3f9492f580cf347637d0bea4b65e0)
ly_associate_package(PACKAGE_NAME zlib-1.3.1-rev2-mac-arm64                         TARGETS zlib                        PACKAGE_HASH 52e62890329d3e003226fca88df30701cdd862a5f137eb5f75dff504377c13b3)
ly_associate_package(PACKAGE_NAME squish-ccr-deb557d-rev1-mac-arm64                 TARGETS squish-ccr                  PACKAGE_HASH 155bfbfa17c19a9cd2ef025de14c5db598f4290045d5b0d83ab58cb345089a77)
ly_associate_package(PACKAGE_NAME astc-encoder-3.2-rev5-mac-arm64                   TARGETS astc-encoder                PACKAGE_HASH bdb1146cc6bbacc07901564fe884529d7cacc9bb44895597327341d3b9833ab0)
ly_associate_package(PACKAGE_NAME ISPCTexComp-36b80aa-rev1-mac-arm64                TARGETS ISPCTexComp                 PACKAGE_HASH 8a4e93277b8face6ea2fd57c6d017bdb55643ed3d6387110bc5f6b3b884dd169)
ly_associate_package(PACKAGE_NAME lz4-1.9.4-rev2-mac-arm64                          TARGETS lz4                         PACKAGE_HASH d85fe35ce176967199fe6e11fce684e6c05f0c5533892a3785a458872a1d5229)
ly_associate_package(PACKAGE_NAME azslc-1.8.22-rev1-mac-arm64                       TARGETS azslc                       PACKAGE_HASH ff7c0bb755ae1fc7f2f5e2b02bb4ddfdf85deea5b22ba2f8baae4ff7b0fc8374)
ly_associate_package(PACKAGE_NAME SQLite-3.37.2-rev2-mac-arm64                      TARGETS SQLite                      PACKAGE_HASH 6fa05df3f97fed97bdef293ac85b250ffe443a43e776ad54312b7b356d41fccb)
ly_associate_package(PACKAGE_NAME AwsIotDeviceSdkCpp-1.15.2-rev2-mac-arm64          TARGETS AwsIotDeviceSdkCpp          PACKAGE_HASH 4854edb7b88fa6437b4e69e87d0ee111a25313ac2a2db5bb2f8b674ba0974f95)
