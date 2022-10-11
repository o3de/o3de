#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# this file allows you to specify all 3p packages (provided by O3DE or the operating system) for Windows.

# shared by other platforms:
ly_associate_package(PACKAGE_NAME md5-2.0-multiplatform                                 TARGETS md5                         PACKAGE_HASH 29e52ad22c78051551f78a40c2709594f0378762ae03b417adca3f4b700affdf)
ly_associate_package(PACKAGE_NAME RapidJSON-1.1.0-rev1-multiplatform                    TARGETS RapidJSON                   PACKAGE_HASH 2f5e26ecf86c3b7a262753e7da69ac59928e78e9534361f3d00c1ad5879e4023)
ly_associate_package(PACKAGE_NAME RapidXML-1.13-rev1-multiplatform                      TARGETS RapidXML                    PACKAGE_HASH 4b7b5651e47cfd019b6b295cc17bb147b65e53073eaab4a0c0d20a37ab74a246)
ly_associate_package(PACKAGE_NAME pybind11-2.10.0-rev1-multiplatform                    TARGETS pybind11                    PACKAGE_HASH 6690acc531d4b8cd453c19b448e2fb8066b2362cbdd2af1ad5df6e0019e6c6c4)
ly_associate_package(PACKAGE_NAME cityhash-1.1-multiplatform                            TARGETS cityhash                    PACKAGE_HASH 0ace9e6f0b2438c5837510032d2d4109125845c0efd7d807f4561ec905512dd2)
ly_associate_package(PACKAGE_NAME zstd-1.35-multiplatform                               TARGETS zstd                        PACKAGE_HASH 45d466c435f1095898578eedde85acf1fd27190e7ea99aeaa9acfd2f09e12665)
ly_associate_package(PACKAGE_NAME glad-2.0.0-beta-rev2-multiplatform                    TARGETS glad                        PACKAGE_HASH ff97ee9664e97d0854b52a3734c2289329d9f2b4cd69478df6d0ca1f1c9392ee)
ly_associate_package(PACKAGE_NAME xxhash-0.7.4-rev1-multiplatform                       TARGETS xxhash                      PACKAGE_HASH e81f3e6c4065975833996dd1fcffe46c3cf0f9e3a4207ec5f4a1b564ba75861e)

# platform-specific:
ly_associate_package(PACKAGE_NAME expat-2.4.2-rev2-windows                              TARGETS expat                       PACKAGE_HASH 748d08f21f5339757059a7887e72b52d15e954c549245c638b0b05bd5961e307)
ly_associate_package(PACKAGE_NAME assimp-5.1.6-rev1-windows                             TARGETS assimplib                   PACKAGE_HASH 299d8a3c70657d74af8841650538e9d083fda9356f6782416edbec0ef5a0493e)
ly_associate_package(PACKAGE_NAME OpenEXR-3.1.3-rev4-windows                            TARGETS OpenEXR Imath               PACKAGE_HASH c850268e849171751cdaefdab1952333ac38afbb771b999e99d67f9761706d98)
ly_associate_package(PACKAGE_NAME AWSGameLiftServerSDK-3.4.2-rev1-windows               TARGETS AWSGameLiftServerSDK        PACKAGE_HASH 9d30eb265adc8b46a7f6a9ad122c2d3c8820ca16961533a3cc994734e264969a)
ly_associate_package(PACKAGE_NAME DirectXShaderCompilerDxc-1.6.2112-o3de-rev1-windows   TARGETS DirectXShaderCompilerDxc    PACKAGE_HASH fdcdc081e67abcfdc8172866258a9c36f1fd0d7b963ba5378ca01cb4fcdbf9c7)
ly_associate_package(PACKAGE_NAME SPIRVCross-2021.04.29-rev1-windows                    TARGETS SPIRVCross                  PACKAGE_HASH 7d601ea9d625b1d509d38bd132a1f433d7e895b16adab76bac6103567a7a6817)
ly_associate_package(PACKAGE_NAME tiff-4.2.0.15-rev3-windows                            TARGETS TIFF                        PACKAGE_HASH c6000a906e6d2a0816b652e93dfbeab41c9ed73cdd5a613acd53e553d0510b60)
ly_associate_package(PACKAGE_NAME freetype-2.11.1-rev1-windows                          TARGETS Freetype                    PACKAGE_HASH 861d059a5542cb8f58a5157f411eee2e78f69ac72e45117227ebe400efe49f61)
ly_associate_package(PACKAGE_NAME AWSNativeSDK-1.9.50-rev2-windows                      TARGETS AWSNativeSDK                PACKAGE_HASH 047de23fa57d33196666c22f45afc9c628bae354a6c39d774cbeee8054b2eb53)
ly_associate_package(PACKAGE_NAME Lua-5.4.4-rev1-windows                                TARGETS Lua                         PACKAGE_HASH 8ac853288712267ec9763be152a9274ce87b54728b8add97e2ba73c0fd5a0345)
ly_associate_package(PACKAGE_NAME PhysX-4.1.2.29882248-rev5-windows                     TARGETS PhysX                       PACKAGE_HASH 4e31a3e1f5bf3952d8af8e28d1a29f04167995a6362fc3a7c20c25f74bf01e23)
ly_associate_package(PACKAGE_NAME mcpp-2.7.2_az.2-rev1-windows                          TARGETS mcpp                        PACKAGE_HASH 794789aba639bfe2f4e8fcb4424d679933dd6290e523084aa0a4e287ac44acb2)
ly_associate_package(PACKAGE_NAME mikkelsen-1.0.0.4-windows                             TARGETS mikkelsen                   PACKAGE_HASH 872c4d245a1c86139aa929f2b465b63ea4ea55b04ced50309135dd4597457a4e)
ly_associate_package(PACKAGE_NAME googletest-1.8.1-rev4-windows                         TARGETS googletest                  PACKAGE_HASH 7e8f03ae8a01563124e3daa06386f25a2b311c10bb95bff05cae6c41eff83837)
ly_associate_package(PACKAGE_NAME googlebenchmark-1.5.0-rev2-windows                    TARGETS GoogleBenchmark             PACKAGE_HASH 0c94ca69ae8e7e4aab8e90032b5c82c5964410429f3dd9dbb1f9bf4fe032b1d4)
ly_associate_package(PACKAGE_NAME d3dx12-headers-rev1-windows                           TARGETS d3dx12                      PACKAGE_HASH 088c637159fba4a3e4c0cf08fb4921906fd4cca498939bd239db7c54b5b2f804)
ly_associate_package(PACKAGE_NAME pyside2-5.15.2.1-py3.10-rev3-windows                  TARGETS pyside2                     PACKAGE_HASH dd3af8dcf2b12cc84c97027438dbf43167f6e9299b30a43ba3af8c0fb2b513cf)
ly_associate_package(PACKAGE_NAME openimageio-opencolorio-2.3.17-rev2-windows           TARGETS OpenImageIO OpenColorIO OpenColorIO::Runtime OpenImageIO::Tools::Binaries OpenImageIO::Tools::PythonPlugins PACKAGE_HASH 5c208a92a23474ce9a33cf4ee4fcba4ddbc712983fefb1dcaebb65a139da4f2b)
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
ly_associate_package(PACKAGE_NAME lz4-1.9.3-vcpkg-rev4-windows                          TARGETS lz4                         PACKAGE_HASH 4ea457b833cd8cfaf8e8e06ed6df601d3e6783b606bdbc44a677f77e19e0db16)
ly_associate_package(PACKAGE_NAME azslc-1.7.35-rev1-windows                             TARGETS azslc                       PACKAGE_HASH 606aea611f2f20afcd8467ddabeecd3661e946eac3c843756c7df2871c1fb8a0)
ly_associate_package(PACKAGE_NAME SQLite-3.37.2-rev1-windows        	                TARGETS SQLite                      PACKAGE_HASH c1658c8ed5cf0e45d4a5da940c6a6d770b76e0f4f57313b70d0fd306885f015e)
ly_associate_package(PACKAGE_NAME AwsIotDeviceSdkCpp-1.15.2-rev1-windows                TARGETS AwsIotDeviceSdkCpp          PACKAGE_HASH b03475a9f0f7a7e7c90619fba35f1a74fb2b8f4cd33fa07af99f2ae9e0c079dd)
ly_associate_package(PACKAGE_NAME vulkan-validationlayers-1.2.198-rev1-windows          TARGETS vulkan-validationlayers     PACKAGE_HASH 4c617b83611f9f990b7e6ff21f2e2d22bda154591bff7e0e39610e319a3e5a53)