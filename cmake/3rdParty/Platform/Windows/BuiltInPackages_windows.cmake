#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# shared by other platforms:
ly_associate_package(PACKAGE_NAME ilmbase-2.3.0-rev4-multiplatform                      TARGETS ilmbase                     PACKAGE_HASH 97547fdf1fbc4d81b8ccf382261f8c25514ed3b3c4f8fd493f0a4fa873bba348)
ly_associate_package(PACKAGE_NAME assimp-5.0.1-rev11-multiplatform                      TARGETS assimplib                   PACKAGE_HASH 1a9113788b893ef4a2ee63ac01eb71b981a92894a5a51175703fa225f5804dec)
ly_associate_package(PACKAGE_NAME md5-2.0-multiplatform                                 TARGETS md5                         PACKAGE_HASH 29e52ad22c78051551f78a40c2709594f0378762ae03b417adca3f4b700affdf)
ly_associate_package(PACKAGE_NAME RapidJSON-1.1.0-rev1-multiplatform                    TARGETS RapidJSON                   PACKAGE_HASH 2f5e26ecf86c3b7a262753e7da69ac59928e78e9534361f3d00c1ad5879e4023)
ly_associate_package(PACKAGE_NAME RapidXML-1.13-rev1-multiplatform                      TARGETS RapidXML                    PACKAGE_HASH 4b7b5651e47cfd019b6b295cc17bb147b65e53073eaab4a0c0d20a37ab74a246)
ly_associate_package(PACKAGE_NAME pybind11-2.4.3-rev2-multiplatform                     TARGETS pybind11                    PACKAGE_HASH d8012f907b6c54ac990b899a0788280857e7c93a9595405a28114b48c354eb1b)
ly_associate_package(PACKAGE_NAME cityhash-1.1-multiplatform                            TARGETS cityhash                    PACKAGE_HASH 0ace9e6f0b2438c5837510032d2d4109125845c0efd7d807f4561ec905512dd2)
ly_associate_package(PACKAGE_NAME expat-2.1.0-multiplatform                             TARGETS expat                       PACKAGE_HASH 452256acd1fd699cef24162575b3524fccfb712f5321c83f1df1ce878de5b418)
ly_associate_package(PACKAGE_NAME zstd-1.35-multiplatform                               TARGETS zstd                        PACKAGE_HASH 45d466c435f1095898578eedde85acf1fd27190e7ea99aeaa9acfd2f09e12665)
ly_associate_package(PACKAGE_NAME SQLite-3.32.2-rev3-multiplatform                      TARGETS SQLite                      PACKAGE_HASH dd4d3de6cbb4ce3d15fc504ba0ae0587e515dc89a25228037035fc0aef4831f4)
ly_associate_package(PACKAGE_NAME glad-2.0.0-beta-rev2-multiplatform                    TARGETS glad                        PACKAGE_HASH ff97ee9664e97d0854b52a3734c2289329d9f2b4cd69478df6d0ca1f1c9392ee)
ly_associate_package(PACKAGE_NAME lux_core-2.2-rev5-multiplatform                       TARGETS lux_core                    PACKAGE_HASH c8c13cf7bc351643e1abd294d0841b24dee60e51647dff13db7aec396ad1e0b5)
ly_associate_package(PACKAGE_NAME xxhash-0.7.4-rev1-multiplatform                       TARGETS xxhash                      PACKAGE_HASH e81f3e6c4065975833996dd1fcffe46c3cf0f9e3a4207ec5f4a1b564ba75861e)

# platform-specific:
ly_associate_package(PACKAGE_NAME AWSGameLiftServerSDK-3.4.1-rev1-windows               TARGETS AWSGameLiftServerSDK        PACKAGE_HASH a0586b006e4def65cc25f388de17dc475e417dc1e6f9d96749777c88aa8271b0)
ly_associate_package(PACKAGE_NAME DirectXShaderCompilerDxc-1.6.2104-o3de-rev3-windows   TARGETS DirectXShaderCompilerDxc    PACKAGE_HASH 803e10b94006b834cbbdd30f562a8ddf04174c2cb6956c8399ec164ef8418d1f)
ly_associate_package(PACKAGE_NAME SPIRVCross-2021.04.29-rev1-windows                    TARGETS SPIRVCross                  PACKAGE_HASH 7d601ea9d625b1d509d38bd132a1f433d7e895b16adab76bac6103567a7a6817)
ly_associate_package(PACKAGE_NAME tiff-4.2.0.15-rev3-windows                            TARGETS TIFF                        PACKAGE_HASH c6000a906e6d2a0816b652e93dfbeab41c9ed73cdd5a613acd53e553d0510b60)
ly_associate_package(PACKAGE_NAME freetype-2.10.4.16-windows                            TARGETS freetype                    PACKAGE_HASH 9809255f1c59b07875097aa8d8c6c21c97c47a31fb35e30f2bb93188e99a85ff)
ly_associate_package(PACKAGE_NAME AWSNativeSDK-1.7.167-rev4-windows                     TARGETS AWSNativeSDK                PACKAGE_HASH a900e80f7259e43aed5c847afee2599ada37f29db70505481397675bcbb6c76c)
ly_associate_package(PACKAGE_NAME Lua-5.3.5-rev5-windows                                TARGETS Lua                         PACKAGE_HASH 136faccf1f73891e3fa3b95f908523187792e56f5b92c63c6a6d7e72d1158d40)
ly_associate_package(PACKAGE_NAME PhysX-4.1.2.29882248-rev5-windows                     TARGETS PhysX                       PACKAGE_HASH 4e31a3e1f5bf3952d8af8e28d1a29f04167995a6362fc3a7c20c25f74bf01e23)
ly_associate_package(PACKAGE_NAME mcpp-2.7.2_az.2-rev1-windows                          TARGETS mcpp                        PACKAGE_HASH 794789aba639bfe2f4e8fcb4424d679933dd6290e523084aa0a4e287ac44acb2)
ly_associate_package(PACKAGE_NAME mikkelsen-1.0.0.4-windows                             TARGETS mikkelsen                   PACKAGE_HASH 872c4d245a1c86139aa929f2b465b63ea4ea55b04ced50309135dd4597457a4e)
ly_associate_package(PACKAGE_NAME googletest-1.8.1-rev4-windows                         TARGETS googletest                  PACKAGE_HASH 7e8f03ae8a01563124e3daa06386f25a2b311c10bb95bff05cae6c41eff83837)
ly_associate_package(PACKAGE_NAME googlebenchmark-1.5.0-rev2-windows                    TARGETS GoogleBenchmark             PACKAGE_HASH 0c94ca69ae8e7e4aab8e90032b5c82c5964410429f3dd9dbb1f9bf4fe032b1d4)
ly_associate_package(PACKAGE_NAME d3dx12-headers-rev1-windows                           TARGETS d3dx12                      PACKAGE_HASH 088c637159fba4a3e4c0cf08fb4921906fd4cca498939bd239db7c54b5b2f804)
ly_associate_package(PACKAGE_NAME pyside2-qt-5.15.1-rev2-windows                        TARGETS pyside2                     PACKAGE_HASH c90f3efcc7c10e79b22a33467855ad861f9dbd2e909df27a5cba9db9fa3edd0f)
ly_associate_package(PACKAGE_NAME openimageio-2.1.16.0-rev2-windows                     TARGETS OpenImageIO                 PACKAGE_HASH 85a2a6cf35cbc4c967c56ca8074babf0955c5b490c90c6e6fd23c78db99fc282)
ly_associate_package(PACKAGE_NAME qt-5.15.2-rev4-windows                                TARGETS Qt                          PACKAGE_HASH a4634caaf48192cad5c5f408504746e53d338856148285057274f6a0ccdc071d)
ly_associate_package(PACKAGE_NAME libpng-1.6.37-rev1-windows                            TARGETS libpng                      PACKAGE_HASH aa20c894fbd7cdaea585a54e37620b3454a7e414a58128acd68ccf6fe76c47d6)
ly_associate_package(PACKAGE_NAME libsamplerate-0.2.1-rev2-windows                      TARGETS libsamplerate               PACKAGE_HASH dcf3c11a96f212a52e2c9241abde5c364ee90b0f32fe6eeb6dcdca01d491829f)
ly_associate_package(PACKAGE_NAME OpenMesh-8.1-rev3-windows                             TARGETS OpenMesh                    PACKAGE_HASH 7a6309323ad03bfc646bd04ecc79c3711de6790e4ff5a72f83a8f5a8f496d684)
ly_associate_package(PACKAGE_NAME civetweb-1.8-rev1-windows                             TARGETS civetweb                    PACKAGE_HASH 36d0e58a59bcdb4dd70493fb1b177aa0354c945b06c30416348fd326cf323dd4)
ly_associate_package(PACKAGE_NAME OpenSSL-1.1.1b-rev2-windows                           TARGETS OpenSSL                     PACKAGE_HASH 9af1c50343f89146b4053101a7aeb20513319a3fe2f007e356d7ce25f9241040)
ly_associate_package(PACKAGE_NAME Crashpad-0.8.0-rev1-windows                           TARGETS Crashpad                    PACKAGE_HASH d162aa3070147bc0130a44caab02c5fe58606910252caf7f90472bd48d4e31e2)
ly_associate_package(PACKAGE_NAME zlib-1.2.11-rev5-windows                              TARGETS ZLIB                        PACKAGE_HASH 8847112429744eb11d92c44026fc5fc53caa4a06709382b5f13978f3c26c4cbd)
ly_associate_package(PACKAGE_NAME squish-ccr-deb557d-rev1-windows                       TARGETS squish-ccr                  PACKAGE_HASH 5c3d9fa491e488ccaf802304ad23b932268a2b2846e383f088779962af2bfa84)
ly_associate_package(PACKAGE_NAME astc-encoder-3.2-rev2-windows                         TARGETS astc-encoder                PACKAGE_HASH 17249bfa438afb34e21449865d9c9297471174ae0cea9b2f9def2ee206038295)
ly_associate_package(PACKAGE_NAME ISPCTexComp-36b80aa-rev1-windows                      TARGETS ISPCTexComp                 PACKAGE_HASH b6fa6ea28a2808a9a5524c72c37789c525925e435770f2d94eb2d387360fa2d0)
ly_associate_package(PACKAGE_NAME lz4-1.9.3-vcpkg-rev4-windows                          TARGETS lz4                         PACKAGE_HASH 4ea457b833cd8cfaf8e8e06ed6df601d3e6783b606bdbc44a677f77e19e0db16)
ly_associate_package(PACKAGE_NAME azslc-1.7.34-rev1-windows                             TARGETS azslc                       PACKAGE_HASH 44eb2e0fc4b0f1c75d0fb6f24c93a5753655b84dbc3e6ad45389ed3b9cf7a4b0)
