#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# shared by other platforms:
ly_associate_package(PACKAGE_NAME zlib-1.2.8-rev2-multiplatform                         TARGETS zlib                        PACKAGE_HASH e6f34b8ac16acf881e3d666ef9fd0c1aee94c3f69283fb6524d35d6f858eebbb)
ly_associate_package(PACKAGE_NAME ilmbase-2.3.0-rev4-multiplatform                      TARGETS ilmbase                     PACKAGE_HASH 97547fdf1fbc4d81b8ccf382261f8c25514ed3b3c4f8fd493f0a4fa873bba348)
ly_associate_package(PACKAGE_NAME hdf5-1.0.11-rev2-multiplatform                        TARGETS hdf5                        PACKAGE_HASH 11d5e04df8a93f8c52a5684a4cacbf0d9003056360983ce34f8d7b601082c6bd)
ly_associate_package(PACKAGE_NAME alembic-1.7.11-rev3-multiplatform                     TARGETS alembic                     PACKAGE_HASH ba7a7d4943dd752f5a662374f6c48b93493df1d8e2c5f6a8d101f3b50700dd25)
ly_associate_package(PACKAGE_NAME assimp-5.0.1-rev11-multiplatform                      TARGETS assimplib                   PACKAGE_HASH 1a9113788b893ef4a2ee63ac01eb71b981a92894a5a51175703fa225f5804dec)
ly_associate_package(PACKAGE_NAME squish-ccr-20150601-rev3-multiplatform                TARGETS squish-ccr                  PACKAGE_HASH c878c6c0c705e78403c397d03f5aa7bc87e5978298710e14d09c9daf951a83b3)
ly_associate_package(PACKAGE_NAME ASTCEncoder-2017_11_14-rev2-multiplatform             TARGETS ASTCEncoder                 PACKAGE_HASH c240ffc12083ee39a5ce9dc241de44d116e513e1e3e4cc1d05305e7aa3bdc326)
ly_associate_package(PACKAGE_NAME md5-2.0-multiplatform                                 TARGETS md5                         PACKAGE_HASH 29e52ad22c78051551f78a40c2709594f0378762ae03b417adca3f4b700affdf)
ly_associate_package(PACKAGE_NAME RapidJSON-1.1.0-multiplatform                         TARGETS RapidJSON                   PACKAGE_HASH 18b0aef4e6e849389916ff6de6682ab9c591ebe15af6ea6017014453c1119ea1)
ly_associate_package(PACKAGE_NAME RapidXML-1.13-multiplatform                           TARGETS RapidXML                    PACKAGE_HASH 510b3c12f8872c54b34733e34f2f69dd21837feafa55bfefa445c98318d96ebf)
ly_associate_package(PACKAGE_NAME pybind11-2.4.3-rev2-multiplatform                     TARGETS pybind11                    PACKAGE_HASH d8012f907b6c54ac990b899a0788280857e7c93a9595405a28114b48c354eb1b)
ly_associate_package(PACKAGE_NAME cityhash-1.1-multiplatform                            TARGETS cityhash                    PACKAGE_HASH 0ace9e6f0b2438c5837510032d2d4109125845c0efd7d807f4561ec905512dd2)
ly_associate_package(PACKAGE_NAME lz4-r128-multiplatform                                TARGETS lz4                         PACKAGE_HASH d7b1d5651191db2c339827ad24f669d9d37754143e9173abc986184532f57c9d)
ly_associate_package(PACKAGE_NAME expat-2.1.0-multiplatform                             TARGETS expat                       PACKAGE_HASH 452256acd1fd699cef24162575b3524fccfb712f5321c83f1df1ce878de5b418)
ly_associate_package(PACKAGE_NAME zstd-1.35-multiplatform                               TARGETS zstd                        PACKAGE_HASH 45d466c435f1095898578eedde85acf1fd27190e7ea99aeaa9acfd2f09e12665)
ly_associate_package(PACKAGE_NAME SQLite-3.32.2-rev3-multiplatform                      TARGETS SQLite                      PACKAGE_HASH dd4d3de6cbb4ce3d15fc504ba0ae0587e515dc89a25228037035fc0aef4831f4)
ly_associate_package(PACKAGE_NAME azslc-1.7.22-rev1-multiplatform                       TARGETS azslc                       PACKAGE_HASH 71b4545d221d4fcd564ccc121c249a8f8f164bcc616faf146f926c3d5c78d527)
ly_associate_package(PACKAGE_NAME glad-2.0.0-beta-rev2-multiplatform                    TARGETS glad                        PACKAGE_HASH ff97ee9664e97d0854b52a3734c2289329d9f2b4cd69478df6d0ca1f1c9392ee)
ly_associate_package(PACKAGE_NAME lux_core-2.2-rev5-multiplatform                       TARGETS lux_core                    PACKAGE_HASH c8c13cf7bc351643e1abd294d0841b24dee60e51647dff13db7aec396ad1e0b5)
ly_associate_package(PACKAGE_NAME xxhash-0.7.4-rev1-multiplatform                       TARGETS xxhash                      PACKAGE_HASH e81f3e6c4065975833996dd1fcffe46c3cf0f9e3a4207ec5f4a1b564ba75861e)
ly_associate_package(PACKAGE_NAME PVRTexTool-4.24.0-rev4-multiplatform                  TARGETS PVRTexTool                  PACKAGE_HASH d0d6da61c7557de0d2c71fc35ba56c3be49555b703f0e853d4c58225537acf1e)

# platform-specific:
ly_associate_package(PACKAGE_NAME AWSGameLiftServerSDK-3.4.1-rev1-windows               TARGETS AWSGameLiftServerSDK        PACKAGE_HASH a0586b006e4def65cc25f388de17dc475e417dc1e6f9d96749777c88aa8271b0)
ly_associate_package(PACKAGE_NAME Blast-v1.1.7_rc2-9-geb169fe-rev1-windows              TARGETS Blast                       PACKAGE_HASH 216df71f4ffaf4a6ea3f2e77e5f27d68f2325e717fbd1626b00c785b82cd1b67)
ly_associate_package(PACKAGE_NAME DirectXShaderCompilerDxc-1.6.2104-o3de-rev2-windows   TARGETS DirectXShaderCompilerDxc    PACKAGE_HASH decc53e97c7ddda9c7f853a30af7808a7b652a912f59ad2cd4bca5d308aae2c4)
ly_associate_package(PACKAGE_NAME SPIRVCross-2021.04.29-rev1-windows                    TARGETS SPIRVCross                  PACKAGE_HASH 7d601ea9d625b1d509d38bd132a1f433d7e895b16adab76bac6103567a7a6817)
ly_associate_package(PACKAGE_NAME freetype-2.10.4.14-windows                            TARGETS freetype                    PACKAGE_HASH 88dedc86ccb8c92f14c2c033e51ee7d828fa08eafd6475c6aa963938a99f4bf3)
ly_associate_package(PACKAGE_NAME tiff-4.2.0.14-windows                                 TARGETS tiff                        PACKAGE_HASH ab60d1398e4e1e375ec0f1a00cdb1d812a07c0096d827db575ce52dd6d714207)
ly_associate_package(PACKAGE_NAME AWSNativeSDK-1.7.167-rev3-windows                     TARGETS AWSNativeSDK                PACKAGE_HASH 929873d4252c464620a9d288e41bd5d47c0bd22750aeb3a1caa68a3da8247c48)
ly_associate_package(PACKAGE_NAME Lua-5.3.5-rev5-windows                                TARGETS Lua                         PACKAGE_HASH 136faccf1f73891e3fa3b95f908523187792e56f5b92c63c6a6d7e72d1158d40)
ly_associate_package(PACKAGE_NAME PhysX-4.1.2.29882248-rev3-windows                     TARGETS PhysX                       PACKAGE_HASH 0c5ffbd9fa588e5cf7643721a7cfe74d0fe448bf82252d39b3a96d06dfca2298)
ly_associate_package(PACKAGE_NAME etc2comp-9cd0f9cae0-rev1-windows                      TARGETS etc2comp                    PACKAGE_HASH fc9ae937b2ec0d42d5e7d0e9e8c80e5e4d257673fb33bc9b7d6db76002117123)
ly_associate_package(PACKAGE_NAME mcpp-2.7.2_az.1-rev1-windows                          TARGETS mcpp                        PACKAGE_HASH 511672598fa319bfb8db87f965b59abff1620bb7c1dcf7669e039a8acd8d3ff8)
ly_associate_package(PACKAGE_NAME mikkelsen-1.0.0.4-windows                             TARGETS mikkelsen                   PACKAGE_HASH 872c4d245a1c86139aa929f2b465b63ea4ea55b04ced50309135dd4597457a4e)
ly_associate_package(PACKAGE_NAME googletest-1.8.1-rev4-windows                         TARGETS googletest                  PACKAGE_HASH 7e8f03ae8a01563124e3daa06386f25a2b311c10bb95bff05cae6c41eff83837)
ly_associate_package(PACKAGE_NAME googlebenchmark-1.5.0-rev2-windows                    TARGETS GoogleBenchmark             PACKAGE_HASH 0c94ca69ae8e7e4aab8e90032b5c82c5964410429f3dd9dbb1f9bf4fe032b1d4)
ly_associate_package(PACKAGE_NAME d3dx12-headers-rev1-windows                           TARGETS d3dx12                      PACKAGE_HASH 088c637159fba4a3e4c0cf08fb4921906fd4cca498939bd239db7c54b5b2f804)
ly_associate_package(PACKAGE_NAME pyside2-qt-5.15.1-rev2-windows                        TARGETS pyside2                     PACKAGE_HASH c90f3efcc7c10e79b22a33467855ad861f9dbd2e909df27a5cba9db9fa3edd0f)
ly_associate_package(PACKAGE_NAME openimageio-2.1.16.0-rev2-windows                     TARGETS OpenImageIO                 PACKAGE_HASH 85a2a6cf35cbc4c967c56ca8074babf0955c5b490c90c6e6fd23c78db99fc282)
ly_associate_package(PACKAGE_NAME qt-5.15.2-rev4-windows                                TARGETS Qt                          PACKAGE_HASH a4634caaf48192cad5c5f408504746e53d338856148285057274f6a0ccdc071d)
ly_associate_package(PACKAGE_NAME libsamplerate-0.2.1-rev2-windows                      TARGETS libsamplerate               PACKAGE_HASH dcf3c11a96f212a52e2c9241abde5c364ee90b0f32fe6eeb6dcdca01d491829f)
ly_associate_package(PACKAGE_NAME OpenMesh-8.1-rev1-windows                             TARGETS OpenMesh                    PACKAGE_HASH 1c1df639358526c368e790dfce40c45cbdfcfb1c9a041b9d7054a8949d88ee77)
ly_associate_package(PACKAGE_NAME civetweb-1.8-rev1-windows                             TARGETS civetweb                    PACKAGE_HASH 36d0e58a59bcdb4dd70493fb1b177aa0354c945b06c30416348fd326cf323dd4)
ly_associate_package(PACKAGE_NAME OpenSSL-1.1.1b-rev2-windows                           TARGETS OpenSSL                     PACKAGE_HASH 9af1c50343f89146b4053101a7aeb20513319a3fe2f007e356d7ce25f9241040)
ly_associate_package(PACKAGE_NAME Crashpad-0.8.0-rev1-windows                           TARGETS Crashpad                    PACKAGE_HASH d162aa3070147bc0130a44caab02c5fe58606910252caf7f90472bd48d4e31e2)
