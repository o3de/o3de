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
ly_associate_package(PACKAGE_NAME cityhash-1.1-multiplatform                        TARGETS cityhash                    PACKAGE_HASH 0ace9e6f0b2438c5837510032d2d4109125845c0efd7d807f4561ec905512dd2)

# platform-specific:
ly_associate_package(PACKAGE_NAME expat-2.4.2-rev2-linux                            TARGETS expat                       PACKAGE_HASH 755369a919e744b9b3f835d1acc684f02e43987832ad4a1c0b6bbf884e6cd45b)
ly_associate_package(PACKAGE_NAME assimp-5.4.3-rev3-linux                           TARGETS assimp                      PACKAGE_HASH 62ddd306d520b9b9ac6f587927ff82a90fb1e9e81d7609d67984551da376cf98)
ly_associate_package(PACKAGE_NAME AWSNativeSDK-1.11.288-rev1-linux                  TARGETS AWSNativeSDK                PACKAGE_HASH 20421c93a5d32feae636c6dc46323b10547f2c5e7e62b63db00319765bb45331)
ly_associate_package(PACKAGE_NAME tiff-4.2.0.15-rev3-linux                          TARGETS TIFF                        PACKAGE_HASH 2377f48b2ebc2d1628d9f65186c881544c92891312abe478a20d10b85877409a)
ly_associate_package(PACKAGE_NAME freetype-2.11.1-rev1-linux                        TARGETS Freetype                    PACKAGE_HASH 28bbb850590507eff85154604787881ead6780e6eeee9e71ed09cd1d48d85983)
ly_associate_package(PACKAGE_NAME Lua-5.4.4-rev1-linux                              TARGETS Lua                         PACKAGE_HASH d582362c3ef90e1ef175a874abda2265839ffc2e40778fa293f10b443b4697ac)
ly_associate_package(PACKAGE_NAME mcpp-2.7.2_az.2-rev1-linux                        TARGETS mcpp                        PACKAGE_HASH df7a998d0bc3fedf44b5bdebaf69ddad6033355b71a590e8642445ec77bc6c41)
ly_associate_package(PACKAGE_NAME mikkelsen-1.0.0.4-linux                           TARGETS mikkelsen                   PACKAGE_HASH 5973b1e71a64633588eecdb5b5c06ca0081f7be97230f6ef64365cbda315b9c8)
ly_associate_package(PACKAGE_NAME googlebenchmark-1.7.0-rev1-linux                  TARGETS GoogleBenchmark             PACKAGE_HASH 230e1881e31490820f0bd2059df4741455b52809ac73367e278e1e821ac89c9b)
ly_associate_package(PACKAGE_NAME qt-5.15.2-rev9-linux                              TARGETS Qt                          PACKAGE_HASH db4bcd2003262f4d8c7d7da832758824fc24e53da5895edef743f67a64a5c734)
ly_associate_package(PACKAGE_NAME png-1.6.37-rev2-linux                             TARGETS PNG                         PACKAGE_HASH 5c82945a1648905a5c4c5cee30dfb53a01618da1bf58d489610636c7ade5adf5)
ly_associate_package(PACKAGE_NAME libsamplerate-0.2.1-rev2-linux                    TARGETS libsamplerate               PACKAGE_HASH 41643c31bc6b7d037f895f89d8d8d6369e906b92eff42b0fe05ee6a100f06261)
ly_associate_package(PACKAGE_NAME openimageio-opencolorio-2.3.17-rev2-linux         TARGETS OpenImageIO OpenColorIO OpenColorIO::Runtime OpenImageIO::Tools::Binaries OpenImageIO::Tools::PythonPlugins PACKAGE_HASH c8a9f1d9d6c9f8c3defdbc3761ba391d175b1cb62a70473183af1eaeaef70c36)
ly_associate_package(PACKAGE_NAME OpenMesh-8.1-rev3-linux                           TARGETS OpenMesh                    PACKAGE_HASH 805bd0b24911bb00c7f575b8c3f10d7ea16548a5014c40811894a9445f17a126)
ly_associate_package(PACKAGE_NAME OpenEXR-3.1.3-rev4-linux                          TARGETS OpenEXR Imath               PACKAGE_HASH fcbac68cfb4e3b694580bc3741443e111aced5f08fde21a92e0c768e8803c7af)
ly_associate_package(PACKAGE_NAME OpenSSL-1.1.1t-rev1-linux                         TARGETS OpenSSL                     PACKAGE_HASH 63aea898b7afe8faccd0c7261e62d2f8b7b870f678a4520d5be81e5815542b39)
ly_associate_package(PACKAGE_NAME DirectXShaderCompilerDxc-1.7.2308-o3de-rev2-linux TARGETS DirectXShaderCompilerDxc    PACKAGE_HASH c0fa3034c88b5ad4efb81fd1f961ab52f9eddfb80506fd06ab7bd3dc857e5950)
ly_associate_package(PACKAGE_NAME SPIRVCross-1.3.275.0-rev1-linux                   TARGETS SPIRVCross                  PACKAGE_HASH 9df035eabcb33c95a940afb0dbdd0781465d4e2d8ba4d5ca874f9ee3fb2295fc)
ly_associate_package(PACKAGE_NAME azslc-1.8.22-rev1-linux                           TARGETS azslc                       PACKAGE_HASH 0b7bbda949f0991adccbca3fb91e5373180d43c54ed7740ad6d54c1c72dce661)
ly_associate_package(PACKAGE_NAME zlib-1.2.11-rev5-linux                            TARGETS ZLIB                        PACKAGE_HASH 9be5ea85722fc27a8645a9c8a812669d107c68e6baa2ca0740872eaeb6a8b0fc)
ly_associate_package(PACKAGE_NAME squish-ccr-deb557d-rev1-linux                     TARGETS squish-ccr                  PACKAGE_HASH 85fecafbddc6a41a27c5f59ed4a5dfb123a94cb4666782cf26e63c0a4724c530)
ly_associate_package(PACKAGE_NAME astc-encoder-3.2-rev2-linux                       TARGETS astc-encoder                PACKAGE_HASH 71549d1ca9e4d48391b92a89ea23656d3393810e6777879f6f8a9def2db1610c)
ly_associate_package(PACKAGE_NAME ISPCTexComp-36b80aa-rev1-linux                    TARGETS ISPCTexComp                 PACKAGE_HASH 065fd12abe4247dde247330313763cf816c3375c221da030bdec35024947f259)
ly_associate_package(PACKAGE_NAME lz4-1.9.4-rev2-linux                              TARGETS lz4                         PACKAGE_HASH 5d7e5d087c224dd26edb19deaa73673eefa2dc73f40d0709739e60f2ad35060b)
ly_associate_package(PACKAGE_NAME pyside2-5.15.2.1-py3.10-rev7-linux                TARGETS pyside2                     PACKAGE_HASH bae4598cb5579d835e90e8435181bb3c5222449ce9c2665143a618dac6122be7)
ly_associate_package(PACKAGE_NAME SQLite-3.37.2-rev1-linux                          TARGETS SQLite                      PACKAGE_HASH bee80d6c6db3e312c1f4f089c90894436ea9c9b74d67256d8c1fb00d4d81fe46)
ly_associate_package(PACKAGE_NAME AwsIotDeviceSdkCpp-1.15.2-rev1-linux              TARGETS AwsIotDeviceSdkCpp          PACKAGE_HASH 83fc1711404d3e5b2faabb1134e97cc92b748d8b87ff4ea99599d8c750b8eff0)
ly_associate_package(PACKAGE_NAME vulkan-validationlayers-1.2.198-rev1-linux        TARGETS vulkan-validationlayers     PACKAGE_HASH 9195c7959695bcbcd1bc1dc5c425c14639a759733b3abe2ffa87eb3915b12c71)
