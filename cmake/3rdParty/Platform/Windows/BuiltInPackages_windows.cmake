#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# this file allows you to specify all 3p packages (provided by O3DE or the operating system) for Windows.

# shared by other platforms:
ly_associate_package(PACKAGE_NAME RapidJSON-1.1.0-rev1-multiplatform                    TARGETS RapidJSON                   PACKAGE_HASH 2f5e26ecf86c3b7a262753e7da69ac59928e78e9534361f3d00c1ad5879e4023)
ly_associate_package(PACKAGE_NAME RapidXML-1.13-rev1-multiplatform                      TARGETS RapidXML                    PACKAGE_HASH 4b7b5651e47cfd019b6b295cc17bb147b65e53073eaab4a0c0d20a37ab74a246)
ly_associate_package(PACKAGE_NAME pybind11-2.10.0-rev1-multiplatform                    TARGETS pybind11                    PACKAGE_HASH 6690acc531d4b8cd453c19b448e2fb8066b2362cbdd2af1ad5df6e0019e6c6c4)
ly_associate_package(PACKAGE_NAME cityhash-1.1-multiplatform                            TARGETS cityhash                    PACKAGE_HASH 0ace9e6f0b2438c5837510032d2d4109125845c0efd7d807f4561ec905512dd2)
ly_associate_package(PACKAGE_NAME zstd-1.35-multiplatform                               TARGETS zstd                        PACKAGE_HASH 45d466c435f1095898578eedde85acf1fd27190e7ea99aeaa9acfd2f09e12665)
ly_associate_package(PACKAGE_NAME glad-2.0.0-beta-rev2-multiplatform                    TARGETS glad                        PACKAGE_HASH ff97ee9664e97d0854b52a3734c2289329d9f2b4cd69478df6d0ca1f1c9392ee)
ly_associate_package(PACKAGE_NAME xxhash-0.7.4-rev1-multiplatform                       TARGETS xxhash                      PACKAGE_HASH e81f3e6c4065975833996dd1fcffe46c3cf0f9e3a4207ec5f4a1b564ba75861e)

# platform-specific:
ly_associate_package(PACKAGE_NAME expat-2.4.2-rev2-windows                              TARGETS expat                       PACKAGE_HASH 748d08f21f5339757059a7887e72b52d15e954c549245c638b0b05bd5961e307)
ly_associate_package(PACKAGE_NAME assimp-5.4.3-rev3-windows                             TARGETS assimp                      PACKAGE_HASH 014f835db2921b0cad8569e1056636788fe7a7c3449d6d9bce6f8d2c6311ea50)
ly_associate_package(PACKAGE_NAME OpenEXR-3.1.3-rev5-windows                            TARGETS OpenEXR Imath               PACKAGE_HASH bff6dc78412bb1b04ded243753bee36e9229fdaf9a9e1fa85b1059238fba4c9b)
ly_associate_package(PACKAGE_NAME DirectXShaderCompilerDxc-1.7.2308-o3de-rev2-windows   TARGETS DirectXShaderCompilerDxc    PACKAGE_HASH 1002eaef605abf2f5080f1edb7efbdad36c5c53e27ae17957b736ab3cdc61b36)
ly_associate_package(PACKAGE_NAME SPIRVCross-1.3.275.0-rev1-windows                     TARGETS SPIRVCross                  PACKAGE_HASH 4c18c95834ebff103ff7f39fc814d65276668d2405d91376290f69723f2ab0f7)
ly_associate_package(PACKAGE_NAME tiff-4.2.0.15-rev3-windows                            TARGETS TIFF                        PACKAGE_HASH c6000a906e6d2a0816b652e93dfbeab41c9ed73cdd5a613acd53e553d0510b60)
ly_associate_package(PACKAGE_NAME freetype-2.11.1-rev1-windows                          TARGETS Freetype                    PACKAGE_HASH 861d059a5542cb8f58a5157f411eee2e78f69ac72e45117227ebe400efe49f61)
ly_associate_package(PACKAGE_NAME AWSNativeSDK-1.11.288-rev1-windows                    TARGETS AWSNativeSDK                PACKAGE_HASH 5112bfc8982f240757f22f32f5f761d7d7020d1c77b029ae480c082fa3ae19d4)
ly_associate_package(PACKAGE_NAME Lua-5.4.4-rev1-windows                                TARGETS Lua                         PACKAGE_HASH 8ac853288712267ec9763be152a9274ce87b54728b8add97e2ba73c0fd5a0345)
ly_associate_package(PACKAGE_NAME mcpp-2.7.2_az.2-rev1-windows                          TARGETS mcpp                        PACKAGE_HASH 794789aba639bfe2f4e8fcb4424d679933dd6290e523084aa0a4e287ac44acb2)
ly_associate_package(PACKAGE_NAME mikkelsen-1.0.0.4-windows                             TARGETS mikkelsen                   PACKAGE_HASH 872c4d245a1c86139aa929f2b465b63ea4ea55b04ced50309135dd4597457a4e)
ly_associate_package(PACKAGE_NAME googlebenchmark-1.7.0-rev1-windows                    TARGETS GoogleBenchmark             PACKAGE_HASH a19e41670b46ec1676b51f9d6ad862443c56415d61b505383c19909fb02feb9e)
ly_associate_package(PACKAGE_NAME d3dx12-headers-rev1-windows                           TARGETS d3dx12                      PACKAGE_HASH 088c637159fba4a3e4c0cf08fb4921906fd4cca498939bd239db7c54b5b2f804)
ly_associate_package(PACKAGE_NAME pyside2-5.15.2.1-py3.10-rev6-windows                  TARGETS pyside2                     PACKAGE_HASH 6c4e99af920cda5f34dc2a495c42f38381179543f062dc72a12147a8a660a681)
ly_associate_package(PACKAGE_NAME openimageio-opencolorio-2.3.17-rev4-windows           TARGETS OpenImageIO OpenColorIO OpenColorIO::Runtime OpenImageIO::Tools::Binaries OpenImageIO::Tools::PythonPlugins PACKAGE_HASH d542198653b33640268a999de542c9c5adf4a29322ed04858c730bc82eb34b09)
ly_associate_package(PACKAGE_NAME qt-5.15.2-rev7-windows                                TARGETS Qt                          PACKAGE_HASH 4343a04130657e740795e50a25ab5fe6e41100fa3c58a212c86bed612dde7775)
ly_associate_package(PACKAGE_NAME png-1.6.37-rev2-windows                               TARGETS PNG                         PACKAGE_HASH e16539a0fff26ac9ef80dd11ef0103eca91745519eacd41d41d96911c173589f)
ly_associate_package(PACKAGE_NAME libsamplerate-0.2.1-rev2-windows                      TARGETS libsamplerate               PACKAGE_HASH dcf3c11a96f212a52e2c9241abde5c364ee90b0f32fe6eeb6dcdca01d491829f)
ly_associate_package(PACKAGE_NAME OpenMesh-8.1-rev3-windows                             TARGETS OpenMesh                    PACKAGE_HASH 7a6309323ad03bfc646bd04ecc79c3711de6790e4ff5a72f83a8f5a8f496d684)
ly_associate_package(PACKAGE_NAME civetweb-1.8-rev1-windows                             TARGETS civetweb                    PACKAGE_HASH 36d0e58a59bcdb4dd70493fb1b177aa0354c945b06c30416348fd326cf323dd4)
ly_associate_package(PACKAGE_NAME OpenSSL-1.1.1o-rev1-windows                           TARGETS OpenSSL                     PACKAGE_HASH 52b9d2bc5f3e0c6e405a0f290d1904bf545acc0c73d6a52351247d917c4a06d2)
ly_associate_package(PACKAGE_NAME Crashpad-0.8.0-rev1-windows                           TARGETS Crashpad                    PACKAGE_HASH d162aa3070147bc0130a44caab02c5fe58606910252caf7f90472bd48d4e31e2)
ly_associate_package(PACKAGE_NAME zlib-1.2.11-rev5-windows                              TARGETS ZLIB                        PACKAGE_HASH 8847112429744eb11d92c44026fc5fc53caa4a06709382b5f13978f3c26c4cbd)
ly_associate_package(PACKAGE_NAME squish-ccr-deb557d-rev1-windows                       TARGETS squish-ccr                  PACKAGE_HASH 5c3d9fa491e488ccaf802304ad23b932268a2b2846e383f088779962af2bfa84)
ly_associate_package(PACKAGE_NAME astc-encoder-3.2-rev2-windows                         TARGETS astc-encoder                PACKAGE_HASH 17249bfa438afb34e21449865d9c9297471174ae0cea9b2f9def2ee206038295)
ly_associate_package(PACKAGE_NAME ISPCTexComp-36b80aa-rev1-windows                      TARGETS ISPCTexComp                 PACKAGE_HASH b6fa6ea28a2808a9a5524c72c37789c525925e435770f2d94eb2d387360fa2d0)
ly_associate_package(PACKAGE_NAME lz4-1.9.4-rev1-windows                                TARGETS lz4                         PACKAGE_HASH aa7d4e8f6424fad7fdc0304fecd83c1183886cc4c7b38ba014c85f43d036e550)
ly_associate_package(PACKAGE_NAME azslc-1.8.22-rev1-windows                             TARGETS azslc                       PACKAGE_HASH 3738f008842df5f9ed9281a16f6e4c9166ed3bd604b57d7eea751af913c60a66)
ly_associate_package(PACKAGE_NAME SQLite-3.37.2-rev1-windows        	                TARGETS SQLite                      PACKAGE_HASH c1658c8ed5cf0e45d4a5da940c6a6d770b76e0f4f57313b70d0fd306885f015e)
ly_associate_package(PACKAGE_NAME AwsIotDeviceSdkCpp-1.15.2-rev1-windows                TARGETS AwsIotDeviceSdkCpp          PACKAGE_HASH b03475a9f0f7a7e7c90619fba35f1a74fb2b8f4cd33fa07af99f2ae9e0c079dd)
ly_associate_package(PACKAGE_NAME vulkan-validationlayers-1.3.261-rev1-windows          TARGETS vulkan-validationlayers     PACKAGE_HASH 79132c6379e0d167c7a2f5a8e251861759a71d92f1fb8cc3b873df821d83ae51)
