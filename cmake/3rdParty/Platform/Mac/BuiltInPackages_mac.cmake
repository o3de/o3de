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
ly_associate_package(PACKAGE_NAME expat-2.4.2-rev2-mac                              TARGETS expat                       PACKAGE_HASH 70f195977a17b08a4dc8687400fd7f2589e3b414d4961b562129166965b6f658)
ly_associate_package(PACKAGE_NAME assimp-5.4.3-rev3-mac                             TARGETS assimp                      PACKAGE_HASH bfda2c319bb4cc26aea8445a9ad33347e0e5ead2a959a5eafb5eed47431f56ef)
ly_associate_package(PACKAGE_NAME DirectXShaderCompilerDxc-1.7.2308-o3de-rev2-mac   TARGETS DirectXShaderCompilerDxc    PACKAGE_HASH bdbf069d5704cf16079e60421857bda150ba6eed69900d03ac7ffefc2335cda3)
ly_associate_package(PACKAGE_NAME SPIRVCross-1.3.275.0-rev1-mac                     TARGETS SPIRVCross                  PACKAGE_HASH 5ad9629f677c42847daf8b097728323685d7018d3ac8af0508d1bd0727a81304)
ly_associate_package(PACKAGE_NAME tiff-4.2.0.15-rev3-mac                            TARGETS TIFF                        PACKAGE_HASH c2615ccdadcc0e1d6c5ed61e5965c4d3a82193d206591b79b805c3b3ff35a4bf)
ly_associate_package(PACKAGE_NAME freetype-2.11.1-rev1-mac                          TARGETS Freetype                    PACKAGE_HASH b66107d3499f2e9c072bd88db26e0e5c1b8013128699393c6a8495afca3d2548)
ly_associate_package(PACKAGE_NAME AWSNativeSDK-1.11.288-rev1-mac                    TARGETS AWSNativeSDK                PACKAGE_HASH 4cf12956d005d4025bea48ddb3856874f6234bc3118f0471fcaae5f28a92e42a)
ly_associate_package(PACKAGE_NAME Lua-5.4.4-rev1-mac                                TARGETS Lua                         PACKAGE_HASH b44daae6bfdf092c7935e4aebafded6772853250c6f0a209866a1ac599857d58)
ly_associate_package(PACKAGE_NAME mcpp-2.7.2_az.2-rev1-mac                          TARGETS mcpp                        PACKAGE_HASH be9558905c9c49179ef3d7d84f0a5472415acdf7fe2d76eb060d9431723ddf2e)
ly_associate_package(PACKAGE_NAME mikkelsen-1.0.0.4-mac                             TARGETS mikkelsen                   PACKAGE_HASH 83af99ca8bee123684ad254263add556f0cf49486c0b3e32e6d303535714e505)
ly_associate_package(PACKAGE_NAME googlebenchmark-1.7.0-rev1-mac                    TARGETS GoogleBenchmark             PACKAGE_HASH a1c8793eb1760905290065929b45600a4b4457345fcc129fce253d1a8980bbce)
ly_associate_package(PACKAGE_NAME openimageio-opencolorio-2.3.17-rev3-mac           TARGETS OpenImageIO OpenColorIO OpenColorIO::Runtime OpenImageIO::Tools::Binaries OpenImageIO::Tools::PythonPlugins PACKAGE_HASH bc322f9e28d519ab5959a638b38ee3b773fefb868802823fad2396ab4f7bcbc8)
ly_associate_package(PACKAGE_NAME OpenMesh-8.1-rev3-mac                             TARGETS OpenMesh                    PACKAGE_HASH af92db02a25c1f7e1741ec898f49d81d52631e00336bf9bddd1e191590063c2f)
ly_associate_package(PACKAGE_NAME OpenSSL-1.1.1o-rev1-mac                           TARGETS OpenSSL                     PACKAGE_HASH 73a4bd7856b53edf5ab9d2ff1d31ebb02301be818680a59206ce8ec5940f3468)
ly_associate_package(PACKAGE_NAME OpenEXR-3.1.3-rev4-mac                            TARGETS OpenEXR Imath               PACKAGE_HASH 927b8ca6cc5815fa8ee4efe6ea2845487cba2540f7958d537692e7c9481a68fc)
ly_associate_package(PACKAGE_NAME qt-5.15.2-rev8-mac                                TARGETS Qt                          PACKAGE_HASH d0f97579ea2822c73f0b316a26c68ceb5332763e691d7e78d6b02fe3104b1d31)
ly_associate_package(PACKAGE_NAME png-1.6.37-rev2-mac                               TARGETS PNG                         PACKAGE_HASH 515252226a6958c459f53d8598d80ec4f90df33d2f1637104fd1a636f4962f07)
ly_associate_package(PACKAGE_NAME libsamplerate-0.2.1-rev2-mac                      TARGETS libsamplerate               PACKAGE_HASH b912af40c0ac197af9c43d85004395ba92a6a859a24b7eacd920fed5854a97fe)
ly_associate_package(PACKAGE_NAME zlib-1.2.11-rev5-mac                              TARGETS ZLIB                        PACKAGE_HASH b6fea9c79b8bf106d4703b67fecaa133f832ad28696c2ceef45fb5f20013c096)
ly_associate_package(PACKAGE_NAME squish-ccr-deb557d-rev1-mac                       TARGETS squish-ccr                  PACKAGE_HASH 155bfbfa17c19a9cd2ef025de14c5db598f4290045d5b0d83ab58cb345089a77)
ly_associate_package(PACKAGE_NAME astc-encoder-3.2-rev5-mac                         TARGETS astc-encoder                PACKAGE_HASH bdb1146cc6bbacc07901564fe884529d7cacc9bb44895597327341d3b9833ab0)
ly_associate_package(PACKAGE_NAME ISPCTexComp-36b80aa-rev1-mac                      TARGETS ISPCTexComp                 PACKAGE_HASH 8a4e93277b8face6ea2fd57c6d017bdb55643ed3d6387110bc5f6b3b884dd169)
ly_associate_package(PACKAGE_NAME lz4-1.9.4-rev1-mac                                TARGETS lz4                         PACKAGE_HASH d52e34e5e2f93acb914fd5f8e5247b67f2e7f15fae0586b5813c2721c8345a0d)
ly_associate_package(PACKAGE_NAME azslc-1.8.22-rev1-mac                             TARGETS azslc                       PACKAGE_HASH d34b416b181b4ad5f8bee7c4976fe74dd503318060d2062385351601fa9f967d)
ly_associate_package(PACKAGE_NAME SQLite-3.37.2-rev2-mac                            TARGETS SQLite                      PACKAGE_HASH b7d9abdb68045003e030e1a9a805db1aefa5e8fde6dccfbb4fab3a06249a41fc)
ly_associate_package(PACKAGE_NAME AwsIotDeviceSdkCpp-1.15.2-rev2-mac                TARGETS AwsIotDeviceSdkCpp          PACKAGE_HASH 4854edb7b88fa6437b4e69e87d0ee111a25313ac2a2db5bb2f8b674ba0974f95)
