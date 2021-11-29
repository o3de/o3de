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
ly_associate_package(PACKAGE_NAME AWSGameLiftServerSDK-3.4.1-rev1-linux             TARGETS AWSGameLiftServerSDK        PACKAGE_HASH a8149a95bd100384af6ade97e2b21a56173740d921e6c3da8188cd51554d39af)
ly_associate_package(PACKAGE_NAME tiff-4.2.0.15-rev3-linux                          TARGETS TIFF                        PACKAGE_HASH 2377f48b2ebc2d1628d9f65186c881544c92891312abe478a20d10b85877409a)
ly_associate_package(PACKAGE_NAME freetype-2.10.4.16-linux                          TARGETS freetype                    PACKAGE_HASH 3f10c703d9001ecd2bb51a3bd003d3237c02d8f947ad0161c0252fdc54cbcf97)
ly_associate_package(PACKAGE_NAME AWSNativeSDK-1.7.167-rev6-linux                   TARGETS AWSNativeSDK                PACKAGE_HASH 490291e4c8057975c3ab86feb971b8a38871c58bac5e5d86abdd1aeb7141eec4)
ly_associate_package(PACKAGE_NAME Lua-5.3.5-rev5-linux                              TARGETS Lua                         PACKAGE_HASH 1adc812abe3dd0dbb2ca9756f81d8f0e0ba45779ac85bf1d8455b25c531a38b0)
ly_associate_package(PACKAGE_NAME PhysX-4.1.2.29882248-rev5-linux                   TARGETS PhysX                       PACKAGE_HASH fa72365df409376aef02d1763194dc91d255bdfcb4e8febcfbb64d23a3e50b96)
ly_associate_package(PACKAGE_NAME mcpp-2.7.2_az.2-rev1-linux                        TARGETS mcpp                        PACKAGE_HASH df7a998d0bc3fedf44b5bdebaf69ddad6033355b71a590e8642445ec77bc6c41)
ly_associate_package(PACKAGE_NAME mikkelsen-1.0.0.4-linux                           TARGETS mikkelsen                   PACKAGE_HASH 5973b1e71a64633588eecdb5b5c06ca0081f7be97230f6ef64365cbda315b9c8)
ly_associate_package(PACKAGE_NAME googletest-1.8.1-rev4-linux                       TARGETS googletest                  PACKAGE_HASH 7b7ad330f369450c316a4c4592d17fbb4c14c731c95bd8f37757203e8c2bbc1b)
ly_associate_package(PACKAGE_NAME googlebenchmark-1.5.0-rev2-linux                  TARGETS GoogleBenchmark             PACKAGE_HASH 4038878f337fc7e0274f0230f71851b385b2e0327c495fc3dd3d1c18a807928d)
ly_associate_package(PACKAGE_NAME unwind-1.2.1-linux                                TARGETS unwind                      PACKAGE_HASH 3453265fb056e25432f611a61546a25f60388e315515ad39007b5925dd054a77)
ly_associate_package(PACKAGE_NAME qt-5.15.2-rev6-linux                              TARGETS Qt                          PACKAGE_HASH a37bd9989f1e8fe57d94b98cbf9bd5c3caaea740e2f314e5162fa77300551531)
ly_associate_package(PACKAGE_NAME libpng-1.6.37-rev1-linux                          TARGETS libpng                      PACKAGE_HASH 896451999f1de76375599aec4b34ae0573d8d34620d9ab29cc30b8739c265ba6)
ly_associate_package(PACKAGE_NAME libsamplerate-0.2.1-rev2-linux                    TARGETS libsamplerate               PACKAGE_HASH 41643c31bc6b7d037f895f89d8d8d6369e906b92eff42b0fe05ee6a100f06261)
ly_associate_package(PACKAGE_NAME OpenMesh-8.1-rev3-linux                           TARGETS OpenMesh                    PACKAGE_HASH 805bd0b24911bb00c7f575b8c3f10d7ea16548a5014c40811894a9445f17a126)
ly_associate_package(PACKAGE_NAME OpenSSL-1.1.1b-rev2-linux                         TARGETS OpenSSL                     PACKAGE_HASH b779426d1e9c5ddf71160d5ae2e639c3b956e0fb5e9fcaf9ce97c4526024e3bc)
ly_associate_package(PACKAGE_NAME DirectXShaderCompilerDxc-1.6.2104-o3de-rev3-linux TARGETS DirectXShaderCompilerDxc    PACKAGE_HASH 88c4a359325d749bc34090b9ac466424847f3b71ba0de15045cf355c17c07099)
ly_associate_package(PACKAGE_NAME SPIRVCross-2021.04.29-rev1-linux                  TARGETS SPIRVCross                  PACKAGE_HASH 7889ee5460a688e9b910c0168b31445c0079d363affa07b25d4c8aeb608a0b80)
ly_associate_package(PACKAGE_NAME azslc-1.7.34-rev1-linux                           TARGETS azslc                       PACKAGE_HASH 6d7dc671936c34ff70d2632196107ca1b8b2b41acdd021bfbc69a9fd56215c22)
ly_associate_package(PACKAGE_NAME zlib-1.2.11-rev5-linux                            TARGETS ZLIB                        PACKAGE_HASH 9be5ea85722fc27a8645a9c8a812669d107c68e6baa2ca0740872eaeb6a8b0fc)
ly_associate_package(PACKAGE_NAME squish-ccr-deb557d-rev1-linux                     TARGETS squish-ccr                  PACKAGE_HASH 85fecafbddc6a41a27c5f59ed4a5dfb123a94cb4666782cf26e63c0a4724c530)
ly_associate_package(PACKAGE_NAME astc-encoder-3.2-rev2-linux                       TARGETS astc-encoder                PACKAGE_HASH 71549d1ca9e4d48391b92a89ea23656d3393810e6777879f6f8a9def2db1610c)
ly_associate_package(PACKAGE_NAME ISPCTexComp-36b80aa-rev1-linux                    TARGETS ISPCTexComp                 PACKAGE_HASH 065fd12abe4247dde247330313763cf816c3375c221da030bdec35024947f259)
ly_associate_package(PACKAGE_NAME lz4-1.9.3-vcpkg-rev4-linux                        TARGETS lz4                         PACKAGE_HASH 5de3dbd3e2a3537c6555d759b3c5bb98e5456cf85c74ff6d046f809b7087290d)
ly_associate_package(PACKAGE_NAME pyside2-5.15.2-rev2-linux                         TARGETS pyside2                     PACKAGE_HASH 7589c397c8224d0c3ad691ff02e1afd55d9a1f9de1967c14eb8105dd2b0c4dd1)
