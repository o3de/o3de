#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# this file allows you to specify all 3p packages (provided by O3DE or the operating system) for Linux.

# shared by other platforms:
ly_associate_package(PACKAGE_NAME md5-2.0-multiplatform                             TARGETS md5                         PACKAGE_HASH 29e52ad22c78051551f78a40c2709594f0378762ae03b417adca3f4b700affdf)
ly_associate_package(PACKAGE_NAME RapidJSON-1.1.0-rev1-multiplatform                TARGETS RapidJSON                   PACKAGE_HASH 2f5e26ecf86c3b7a262753e7da69ac59928e78e9534361f3d00c1ad5879e4023)
ly_associate_package(PACKAGE_NAME RapidXML-1.13-rev1-multiplatform                  TARGETS RapidXML                    PACKAGE_HASH 4b7b5651e47cfd019b6b295cc17bb147b65e53073eaab4a0c0d20a37ab74a246)
ly_associate_package(PACKAGE_NAME pybind11-2.4.3-rev3-multiplatform                 TARGETS pybind11                    PACKAGE_HASH dccb5546607b8b31cd207033aaf24ab044ce6e188a9f12411236a010f9e0c4ff)
ly_associate_package(PACKAGE_NAME cityhash-1.1-multiplatform                        TARGETS cityhash                    PACKAGE_HASH 0ace9e6f0b2438c5837510032d2d4109125845c0efd7d807f4561ec905512dd2)
ly_associate_package(PACKAGE_NAME zstd-1.35-multiplatform                           TARGETS zstd                        PACKAGE_HASH 45d466c435f1095898578eedde85acf1fd27190e7ea99aeaa9acfd2f09e12665)
ly_associate_package(PACKAGE_NAME glad-2.0.0-beta-rev2-multiplatform                TARGETS glad                        PACKAGE_HASH ff97ee9664e97d0854b52a3734c2289329d9f2b4cd69478df6d0ca1f1c9392ee)
ly_associate_package(PACKAGE_NAME xxhash-0.7.4-rev1-multiplatform                   TARGETS xxhash                      PACKAGE_HASH e81f3e6c4065975833996dd1fcffe46c3cf0f9e3a4207ec5f4a1b564ba75861e)

# platform-specific:
ly_associate_package(PACKAGE_NAME expat-2.4.2-rev2-linux                            TARGETS expat                       PACKAGE_HASH 755369a919e744b9b3f835d1acc684f02e43987832ad4a1c0b6bbf884e6cd45b)
ly_associate_package(PACKAGE_NAME assimp-5.1.6-rev1-linux                           TARGETS assimplib                   PACKAGE_HASH 40d64242d5d32a69af3a25690b76f051f3c1a573c1bafba0782cb771a53dfab7)
ly_associate_package(PACKAGE_NAME AWSGameLiftServerSDK-3.4.2-rev1-linux             TARGETS AWSGameLiftServerSDK        PACKAGE_HASH 875a8ee45ab5948b10eedfd9057b14db7f01c4b31820f8f998eb6dee1c05a176)
ly_associate_package(PACKAGE_NAME tiff-4.2.0.15-rev3-linux                          TARGETS TIFF                        PACKAGE_HASH 2377f48b2ebc2d1628d9f65186c881544c92891312abe478a20d10b85877409a)
ly_associate_package(PACKAGE_NAME freetype-2.11.1-rev1-linux                        TARGETS Freetype                    PACKAGE_HASH 28bbb850590507eff85154604787881ead6780e6eeee9e71ed09cd1d48d85983)
ly_associate_package(PACKAGE_NAME AWSNativeSDK-1.9.50-rev1-linux                    TARGETS AWSNativeSDK                PACKAGE_HASH f30b6969c6732a7c1a23a59d205a150633a7f219dcb60d837b543888d2c63ea1)
ly_associate_package(PACKAGE_NAME Lua-5.4.4-rev1-linux                              TARGETS Lua                         PACKAGE_HASH d582362c3ef90e1ef175a874abda2265839ffc2e40778fa293f10b443b4697ac)
ly_associate_package(PACKAGE_NAME PhysX-4.1.2.29882248-rev5-linux                   TARGETS PhysX                       PACKAGE_HASH fa72365df409376aef02d1763194dc91d255bdfcb4e8febcfbb64d23a3e50b96)
ly_associate_package(PACKAGE_NAME mcpp-2.7.2_az.2-rev1-linux                        TARGETS mcpp                        PACKAGE_HASH df7a998d0bc3fedf44b5bdebaf69ddad6033355b71a590e8642445ec77bc6c41)
ly_associate_package(PACKAGE_NAME mikkelsen-1.0.0.4-linux                           TARGETS mikkelsen                   PACKAGE_HASH 5973b1e71a64633588eecdb5b5c06ca0081f7be97230f6ef64365cbda315b9c8)
ly_associate_package(PACKAGE_NAME googletest-1.8.1-rev4-linux                       TARGETS googletest                  PACKAGE_HASH 7b7ad330f369450c316a4c4592d17fbb4c14c731c95bd8f37757203e8c2bbc1b)
ly_associate_package(PACKAGE_NAME googlebenchmark-1.5.0-rev2-linux                  TARGETS GoogleBenchmark             PACKAGE_HASH 4038878f337fc7e0274f0230f71851b385b2e0327c495fc3dd3d1c18a807928d)
ly_associate_package(PACKAGE_NAME unwind-1.2.1-linux                                TARGETS unwind                      PACKAGE_HASH 3453265fb056e25432f611a61546a25f60388e315515ad39007b5925dd054a77)
ly_associate_package(PACKAGE_NAME qt-5.15.2-rev8-linux                              TARGETS Qt                          PACKAGE_HASH 613d6a404b305ce0e715c57c936dc00318fb9f0d2d3f6609f8454c198f993095)
ly_associate_package(PACKAGE_NAME png-1.6.37-rev2-linux                             TARGETS PNG                         PACKAGE_HASH 5c82945a1648905a5c4c5cee30dfb53a01618da1bf58d489610636c7ade5adf5)
ly_associate_package(PACKAGE_NAME libsamplerate-0.2.1-rev2-linux                    TARGETS libsamplerate               PACKAGE_HASH 41643c31bc6b7d037f895f89d8d8d6369e906b92eff42b0fe05ee6a100f06261)
ly_associate_package(PACKAGE_NAME OpenMesh-8.1-rev3-linux                           TARGETS OpenMesh                    PACKAGE_HASH 805bd0b24911bb00c7f575b8c3f10d7ea16548a5014c40811894a9445f17a126)
ly_associate_package(PACKAGE_NAME OpenEXR-3.1.3-rev4-linux                          TARGETS OpenEXR Imath               PACKAGE_HASH fcbac68cfb4e3b694580bc3741443e111aced5f08fde21a92e0c768e8803c7af)
ly_associate_package(PACKAGE_NAME DirectXShaderCompilerDxc-1.6.2112-o3de-rev1-linux TARGETS DirectXShaderCompilerDxc    PACKAGE_HASH ac9f98e0e3b07fde0f9bbe1e6daa386da37699819cde06dcc8d3235421f6e977)
ly_associate_package(PACKAGE_NAME SPIRVCross-2021.04.29-rev1-linux                  TARGETS SPIRVCross                  PACKAGE_HASH 7889ee5460a688e9b910c0168b31445c0079d363affa07b25d4c8aeb608a0b80)
ly_associate_package(PACKAGE_NAME azslc-1.7.35-rev1-linux                           TARGETS azslc                       PACKAGE_HASH 273484be06dfc25e8da6a6e17937ae69a2efdb0b4c5f105efa83d6ad54d756e5)
ly_associate_package(PACKAGE_NAME zlib-1.2.11-rev5-linux                            TARGETS ZLIB                        PACKAGE_HASH 9be5ea85722fc27a8645a9c8a812669d107c68e6baa2ca0740872eaeb6a8b0fc)
ly_associate_package(PACKAGE_NAME squish-ccr-deb557d-rev1-linux                     TARGETS squish-ccr                  PACKAGE_HASH 85fecafbddc6a41a27c5f59ed4a5dfb123a94cb4666782cf26e63c0a4724c530)
ly_associate_package(PACKAGE_NAME astc-encoder-3.2-rev2-linux                       TARGETS astc-encoder                PACKAGE_HASH 71549d1ca9e4d48391b92a89ea23656d3393810e6777879f6f8a9def2db1610c)
ly_associate_package(PACKAGE_NAME ISPCTexComp-36b80aa-rev1-linux                    TARGETS ISPCTexComp                 PACKAGE_HASH 065fd12abe4247dde247330313763cf816c3375c221da030bdec35024947f259)
ly_associate_package(PACKAGE_NAME lz4-1.9.3-vcpkg-rev4-linux                        TARGETS lz4                         PACKAGE_HASH 5de3dbd3e2a3537c6555d759b3c5bb98e5456cf85c74ff6d046f809b7087290d)
ly_associate_package(PACKAGE_NAME pyside2-5.15.2-rev2-linux                         TARGETS pyside2                     PACKAGE_HASH 7589c397c8224d0c3ad691ff02e1afd55d9a1f9de1967c14eb8105dd2b0c4dd1)
ly_associate_package(PACKAGE_NAME SQLite-3.37.2-rev1-linux                          TARGETS SQLite                      PACKAGE_HASH bee80d6c6db3e312c1f4f089c90894436ea9c9b74d67256d8c1fb00d4d81fe46)
ly_associate_package(PACKAGE_NAME AwsIotDeviceSdkCpp-1.15.2-rev1-linux              TARGETS AwsIotDeviceSdkCpp          PACKAGE_HASH 83fc1711404d3e5b2faabb1134e97cc92b748d8b87ff4ea99599d8c750b8eff0)

# Use system default OpenSSL library instead of maintaining an O3DE version for Linux
include(${CMAKE_CURRENT_LIST_DIR}/OpenSSL_linux.cmake)

cmake_path(RELATIVE_PATH CMAKE_CURRENT_LIST_DIR BASE_DIRECTORY ${LY_ROOT_FOLDER} OUTPUT_VARIABLE openssl_install_code)

set(openssl_install_code "
# Use system default OpenSSL library instead of maintaining an O3DE version for Linux
include(${openssl_cmake_rel_directory}/OpenSSL_linux.cmake)
")

# Inject custom logic to include the OpenSSL_linux.cmake into the generated BuiltInPackages_linux.cmake
set_property(GLOBAL APPEND_STRING PROPERTY O3DE_BUILTIN_PACKAGES_INSTALL_CODE "${openssl_install_code}")
