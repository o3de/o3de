#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# shared by other platforms:
ly_associate_package(PACKAGE_NAME ilmbase-2.3.0-rev4-multiplatform                  TARGETS ilmbase                     PACKAGE_HASH 97547fdf1fbc4d81b8ccf382261f8c25514ed3b3c4f8fd493f0a4fa873bba348)
ly_associate_package(PACKAGE_NAME assimp-5.0.1-rev11-multiplatform                  TARGETS assimplib                   PACKAGE_HASH 1a9113788b893ef4a2ee63ac01eb71b981a92894a5a51175703fa225f5804dec)
ly_associate_package(PACKAGE_NAME md5-2.0-multiplatform                             TARGETS md5                         PACKAGE_HASH 29e52ad22c78051551f78a40c2709594f0378762ae03b417adca3f4b700affdf)
ly_associate_package(PACKAGE_NAME RapidJSON-1.1.0-rev1-multiplatform                TARGETS RapidJSON                   PACKAGE_HASH 2f5e26ecf86c3b7a262753e7da69ac59928e78e9534361f3d00c1ad5879e4023)
ly_associate_package(PACKAGE_NAME RapidXML-1.13-rev1-multiplatform                  TARGETS RapidXML                    PACKAGE_HASH 4b7b5651e47cfd019b6b295cc17bb147b65e53073eaab4a0c0d20a37ab74a246)
ly_associate_package(PACKAGE_NAME pybind11-2.4.3-rev2-multiplatform                 TARGETS pybind11                    PACKAGE_HASH d8012f907b6c54ac990b899a0788280857e7c93a9595405a28114b48c354eb1b)
ly_associate_package(PACKAGE_NAME cityhash-1.1-multiplatform                        TARGETS cityhash                    PACKAGE_HASH 0ace9e6f0b2438c5837510032d2d4109125845c0efd7d807f4561ec905512dd2)
ly_associate_package(PACKAGE_NAME expat-2.1.0-multiplatform                         TARGETS expat                       PACKAGE_HASH 452256acd1fd699cef24162575b3524fccfb712f5321c83f1df1ce878de5b418)
ly_associate_package(PACKAGE_NAME zstd-1.35-multiplatform                           TARGETS zstd                        PACKAGE_HASH 45d466c435f1095898578eedde85acf1fd27190e7ea99aeaa9acfd2f09e12665)
ly_associate_package(PACKAGE_NAME SQLite-3.32.2-rev3-multiplatform                  TARGETS SQLite                      PACKAGE_HASH dd4d3de6cbb4ce3d15fc504ba0ae0587e515dc89a25228037035fc0aef4831f4)
ly_associate_package(PACKAGE_NAME glad-2.0.0-beta-rev2-multiplatform                TARGETS glad                        PACKAGE_HASH ff97ee9664e97d0854b52a3734c2289329d9f2b4cd69478df6d0ca1f1c9392ee)
ly_associate_package(PACKAGE_NAME lux_core-2.2-rev5-multiplatform                   TARGETS lux_core                    PACKAGE_HASH c8c13cf7bc351643e1abd294d0841b24dee60e51647dff13db7aec396ad1e0b5)
ly_associate_package(PACKAGE_NAME xxhash-0.7.4-rev1-multiplatform                   TARGETS xxhash                      PACKAGE_HASH e81f3e6c4065975833996dd1fcffe46c3cf0f9e3a4207ec5f4a1b564ba75861e)

# platform-specific:
ly_associate_package(PACKAGE_NAME DirectXShaderCompilerDxc-1.6.2104-o3de-rev3-mac   TARGETS DirectXShaderCompilerDxc    PACKAGE_HASH 3f77367dbb0342136ec4ebbd44bc1fedf7198089a0f83c5631248530769b2be6)
ly_associate_package(PACKAGE_NAME SPIRVCross-2021.04.29-rev1-mac                    TARGETS SPIRVCross                  PACKAGE_HASH 78c6376ed2fd195b9b1f5fb2b56e5267a32c3aa21fb399e905308de470eb4515)
ly_associate_package(PACKAGE_NAME tiff-4.2.0.15-rev3-mac                            TARGETS TIFF                        PACKAGE_HASH c2615ccdadcc0e1d6c5ed61e5965c4d3a82193d206591b79b805c3b3ff35a4bf)
ly_associate_package(PACKAGE_NAME freetype-2.10.4.16-mac                            TARGETS freetype                    PACKAGE_HASH f159b346ac3251fb29cb8dd5f805c99b0015ed7fdb3887f656945ca701a61d0d)
ly_associate_package(PACKAGE_NAME AWSNativeSDK-1.7.167-rev5-mac                     TARGETS AWSNativeSDK                PACKAGE_HASH ffb890bd9cf23afb429b9214ad9bac1bf04696f07a0ebb93c42058c482ab2f01)
ly_associate_package(PACKAGE_NAME Lua-5.3.5-rev6-mac                                TARGETS Lua                         PACKAGE_HASH b9079fd35634774c9269028447562c6b712dbc83b9c64975c095fd423ff04c08)
ly_associate_package(PACKAGE_NAME PhysX-4.1.2.29882248-rev5-mac                     TARGETS PhysX                       PACKAGE_HASH 83940b3876115db82cd8ffcb9e902278e75846d6ad94a41e135b155cee1ee186)
ly_associate_package(PACKAGE_NAME mcpp-2.7.2_az.2-rev1-mac                          TARGETS mcpp                        PACKAGE_HASH be9558905c9c49179ef3d7d84f0a5472415acdf7fe2d76eb060d9431723ddf2e)
ly_associate_package(PACKAGE_NAME mikkelsen-1.0.0.4-mac                             TARGETS mikkelsen                   PACKAGE_HASH 83af99ca8bee123684ad254263add556f0cf49486c0b3e32e6d303535714e505)
ly_associate_package(PACKAGE_NAME googletest-1.8.1-rev4-mac                         TARGETS googletest                  PACKAGE_HASH cbf020d5ef976c5db8b6e894c6c63151ade85ed98e7c502729dd20172acae5a8)
ly_associate_package(PACKAGE_NAME googlebenchmark-1.5.0-rev2-mac                    TARGETS GoogleBenchmark             PACKAGE_HASH ad25de0146769c91e179953d845de2bec8ed4a691f973f47e3eb37639381f665)
ly_associate_package(PACKAGE_NAME OpenMesh-8.1-rev3-mac                             TARGETS OpenMesh                    PACKAGE_HASH af92db02a25c1f7e1741ec898f49d81d52631e00336bf9bddd1e191590063c2f)
ly_associate_package(PACKAGE_NAME OpenSSL-1.1.1b-rev1-mac                           TARGETS OpenSSL                     PACKAGE_HASH 28adc1c0616ac0482b2a9d7b4a3a3635a1020e87b163f8aba687c501cf35f96c)
ly_associate_package(PACKAGE_NAME qt-5.15.2-rev5-mac                                TARGETS Qt                          PACKAGE_HASH 9d25918351898b308ded3e9e571fff6f26311b2071aeafd00dd5b249fdf53f7e)
ly_associate_package(PACKAGE_NAME libpng-1.6.37-mac                                 TARGETS libpng                      PACKAGE_HASH 1ad76cd038ccc1f288f83c5fe2859a0f35c5154e1fe7658e1230cc428d318a8b)
ly_associate_package(PACKAGE_NAME libsamplerate-0.2.1-rev2-mac                      TARGETS libsamplerate               PACKAGE_HASH b912af40c0ac197af9c43d85004395ba92a6a859a24b7eacd920fed5854a97fe)
ly_associate_package(PACKAGE_NAME zlib-1.2.11-rev5-mac                              TARGETS ZLIB                        PACKAGE_HASH b6fea9c79b8bf106d4703b67fecaa133f832ad28696c2ceef45fb5f20013c096)
ly_associate_package(PACKAGE_NAME squish-ccr-deb557d-rev1-mac                       TARGETS squish-ccr                  PACKAGE_HASH 155bfbfa17c19a9cd2ef025de14c5db598f4290045d5b0d83ab58cb345089a77)
ly_associate_package(PACKAGE_NAME astc-encoder-3.2-rev2-mac                         TARGETS astc-encoder                PACKAGE_HASH 06f129d26995845824f1fb906a5135b2c71d44d66c768342af85fa28a175906f)
ly_associate_package(PACKAGE_NAME ISPCTexComp-36b80aa-rev1-mac                      TARGETS ISPCTexComp                 PACKAGE_HASH 8a4e93277b8face6ea2fd57c6d017bdb55643ed3d6387110bc5f6b3b884dd169)
ly_associate_package(PACKAGE_NAME lz4-1.9.3-vcpkg-rev4-mac                          TARGETS lz4                         PACKAGE_HASH 891ff630bf34f7ab1d8eaee2ea0a8f1fca89dbdc63fca41ee592703dd488a73b)
ly_associate_package(PACKAGE_NAME azslc-1.7.34-rev1-mac                             TARGETS azslc                       PACKAGE_HASH a9d81946b42ffa55c0d14d6a9249b3340e59a8fb8835e7a96c31df80f14723bc)
