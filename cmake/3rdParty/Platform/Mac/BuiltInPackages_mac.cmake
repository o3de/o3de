#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# shared by other platforms:
ly_associate_package(PACKAGE_NAME zlib-1.2.8-rev2-multiplatform                     TARGETS zlib                        PACKAGE_HASH e6f34b8ac16acf881e3d666ef9fd0c1aee94c3f69283fb6524d35d6f858eebbb)
ly_associate_package(PACKAGE_NAME ilmbase-2.3.0-rev4-multiplatform                  TARGETS ilmbase                     PACKAGE_HASH 97547fdf1fbc4d81b8ccf382261f8c25514ed3b3c4f8fd493f0a4fa873bba348)
ly_associate_package(PACKAGE_NAME hdf5-1.0.11-rev2-multiplatform                    TARGETS hdf5                        PACKAGE_HASH 11d5e04df8a93f8c52a5684a4cacbf0d9003056360983ce34f8d7b601082c6bd)
ly_associate_package(PACKAGE_NAME alembic-1.7.11-rev3-multiplatform                 TARGETS alembic                     PACKAGE_HASH ba7a7d4943dd752f5a662374f6c48b93493df1d8e2c5f6a8d101f3b50700dd25)
ly_associate_package(PACKAGE_NAME assimp-5.0.1-rev11-multiplatform                  TARGETS assimplib                   PACKAGE_HASH 1a9113788b893ef4a2ee63ac01eb71b981a92894a5a51175703fa225f5804dec)
ly_associate_package(PACKAGE_NAME squish-ccr-20150601-rev3-multiplatform            TARGETS squish-ccr                  PACKAGE_HASH c878c6c0c705e78403c397d03f5aa7bc87e5978298710e14d09c9daf951a83b3)
ly_associate_package(PACKAGE_NAME ASTCEncoder-2017_11_14-rev2-multiplatform         TARGETS ASTCEncoder                 PACKAGE_HASH c240ffc12083ee39a5ce9dc241de44d116e513e1e3e4cc1d05305e7aa3bdc326)
ly_associate_package(PACKAGE_NAME md5-2.0-multiplatform                             TARGETS md5                         PACKAGE_HASH 29e52ad22c78051551f78a40c2709594f0378762ae03b417adca3f4b700affdf)
ly_associate_package(PACKAGE_NAME RapidJSON-1.1.0-multiplatform                     TARGETS RapidJSON                   PACKAGE_HASH 18b0aef4e6e849389916ff6de6682ab9c591ebe15af6ea6017014453c1119ea1)
ly_associate_package(PACKAGE_NAME RapidXML-1.13-multiplatform                       TARGETS RapidXML                    PACKAGE_HASH 510b3c12f8872c54b34733e34f2f69dd21837feafa55bfefa445c98318d96ebf)
ly_associate_package(PACKAGE_NAME pybind11-2.4.3-rev2-multiplatform                 TARGETS pybind11                    PACKAGE_HASH d8012f907b6c54ac990b899a0788280857e7c93a9595405a28114b48c354eb1b)
ly_associate_package(PACKAGE_NAME cityhash-1.1-multiplatform                        TARGETS cityhash                    PACKAGE_HASH 0ace9e6f0b2438c5837510032d2d4109125845c0efd7d807f4561ec905512dd2)
ly_associate_package(PACKAGE_NAME lz4-r128-multiplatform                            TARGETS lz4                         PACKAGE_HASH d7b1d5651191db2c339827ad24f669d9d37754143e9173abc986184532f57c9d)
ly_associate_package(PACKAGE_NAME expat-2.1.0-multiplatform                         TARGETS expat                       PACKAGE_HASH 452256acd1fd699cef24162575b3524fccfb712f5321c83f1df1ce878de5b418)
ly_associate_package(PACKAGE_NAME zstd-1.35-multiplatform                           TARGETS zstd                        PACKAGE_HASH 45d466c435f1095898578eedde85acf1fd27190e7ea99aeaa9acfd2f09e12665)
ly_associate_package(PACKAGE_NAME SQLite-3.32.2-rev3-multiplatform                  TARGETS SQLite                      PACKAGE_HASH dd4d3de6cbb4ce3d15fc504ba0ae0587e515dc89a25228037035fc0aef4831f4)
ly_associate_package(PACKAGE_NAME azslc-1.7.22-rev1-multiplatform                   TARGETS azslc                       PACKAGE_HASH 71b4545d221d4fcd564ccc121c249a8f8f164bcc616faf146f926c3d5c78d527)
ly_associate_package(PACKAGE_NAME glad-2.0.0-beta-rev2-multiplatform                TARGETS glad                        PACKAGE_HASH ff97ee9664e97d0854b52a3734c2289329d9f2b4cd69478df6d0ca1f1c9392ee)
ly_associate_package(PACKAGE_NAME lux_core-2.2-rev5-multiplatform                   TARGETS lux_core                    PACKAGE_HASH c8c13cf7bc351643e1abd294d0841b24dee60e51647dff13db7aec396ad1e0b5)
ly_associate_package(PACKAGE_NAME xxhash-0.7.4-rev1-multiplatform                   TARGETS xxhash                      PACKAGE_HASH e81f3e6c4065975833996dd1fcffe46c3cf0f9e3a4207ec5f4a1b564ba75861e)
ly_associate_package(PACKAGE_NAME PVRTexTool-4.24.0-rev4-multiplatform              TARGETS PVRTexTool                  PACKAGE_HASH d0d6da61c7557de0d2c71fc35ba56c3be49555b703f0e853d4c58225537acf1e)

# platform-specific:
ly_associate_package(PACKAGE_NAME DirectXShaderCompilerDxc-1.6.2104-o3de-rev2-mac   TARGETS DirectXShaderCompilerDxc    PACKAGE_HASH 2bede9a7ef3573027c005e38139237559eebf845c13ffb54c33c5b8675f962e2)
ly_associate_package(PACKAGE_NAME SPIRVCross-2021.04.29-rev1-mac                    TARGETS SPIRVCross                  PACKAGE_HASH 78c6376ed2fd195b9b1f5fb2b56e5267a32c3aa21fb399e905308de470eb4515)
ly_associate_package(PACKAGE_NAME freetype-2.10.4.14-mac-ios                        TARGETS freetype                    PACKAGE_HASH 67b4f57aed92082d3fd7c16aa244a7d908d90122c296b0a63f73e0a0b8761977)
ly_associate_package(PACKAGE_NAME tiff-4.2.0.15-mac-ios                             TARGETS tiff                        PACKAGE_HASH a23ae1f8991a29f8e5df09d6d5b00d7768a740f90752cef465558c1768343709)
ly_associate_package(PACKAGE_NAME AWSNativeSDK-1.7.167-rev3-mac                     TARGETS AWSNativeSDK                PACKAGE_HASH 21920372e90355407578b45ac19580df1463a39a25a867bcd0ffd8b385c8254a)
ly_associate_package(PACKAGE_NAME Lua-5.3.5-rev6-mac                                TARGETS Lua                         PACKAGE_HASH b9079fd35634774c9269028447562c6b712dbc83b9c64975c095fd423ff04c08)
ly_associate_package(PACKAGE_NAME PhysX-4.1.2.29882248-rev3-mac                     TARGETS PhysX                       PACKAGE_HASH 5e092a11d5c0a50c4dd99bb681a04b566a4f6f29aa08443d9bffc8dc12c27c8e)
ly_associate_package(PACKAGE_NAME etc2comp-9cd0f9cae0-rev1-mac                      TARGETS etc2comp                    PACKAGE_HASH 1966ab101c89db7ecf30984917e0a48c0d02ee0e4d65b798743842b9469c0818)
ly_associate_package(PACKAGE_NAME mcpp-2.7.2_az.1-rev1-mac                          TARGETS mcpp                        PACKAGE_HASH 48a9c5197bf72843fb9ac44825501ee16bbe3e72e086a32b8c9c05bf47db12ab)
ly_associate_package(PACKAGE_NAME mikkelsen-1.0.0.4-mac                             TARGETS mikkelsen                   PACKAGE_HASH 83af99ca8bee123684ad254263add556f0cf49486c0b3e32e6d303535714e505)
ly_associate_package(PACKAGE_NAME googletest-1.8.1-rev4-mac                         TARGETS googletest                  PACKAGE_HASH cbf020d5ef976c5db8b6e894c6c63151ade85ed98e7c502729dd20172acae5a8)
ly_associate_package(PACKAGE_NAME googlebenchmark-1.5.0-rev2-mac                    TARGETS GoogleBenchmark             PACKAGE_HASH ad25de0146769c91e179953d845de2bec8ed4a691f973f47e3eb37639381f665)
ly_associate_package(PACKAGE_NAME OpenSSL-1.1.1b-rev1-mac                           TARGETS OpenSSL                     PACKAGE_HASH 28adc1c0616ac0482b2a9d7b4a3a3635a1020e87b163f8aba687c501cf35f96c)
ly_associate_package(PACKAGE_NAME qt-5.15.2-rev5-mac                                TARGETS Qt                          PACKAGE_HASH 9d25918351898b308ded3e9e571fff6f26311b2071aeafd00dd5b249fdf53f7e)
ly_associate_package(PACKAGE_NAME libsamplerate-0.2.1-rev2-mac                      TARGETS libsamplerate               PACKAGE_HASH b912af40c0ac197af9c43d85004395ba92a6a859a24b7eacd920fed5854a97fe)
