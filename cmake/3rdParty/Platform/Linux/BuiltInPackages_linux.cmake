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
ly_associate_package(PACKAGE_NAME glad-2.0.0-beta-rev2-multiplatform                TARGETS glad                        PACKAGE_HASH ff97ee9664e97d0854b52a3734c2289329d9f2b4cd69478df6d0ca1f1c9392ee)
ly_associate_package(PACKAGE_NAME lux_core-2.2-rev5-multiplatform                   TARGETS lux_core                    PACKAGE_HASH c8c13cf7bc351643e1abd294d0841b24dee60e51647dff13db7aec396ad1e0b5)
ly_associate_package(PACKAGE_NAME xxhash-0.7.4-rev1-multiplatform                   TARGETS xxhash                      PACKAGE_HASH e81f3e6c4065975833996dd1fcffe46c3cf0f9e3a4207ec5f4a1b564ba75861e)
ly_associate_package(PACKAGE_NAME PVRTexTool-4.24.0-rev4-multiplatform              TARGETS PVRTexTool                  PACKAGE_HASH d0d6da61c7557de0d2c71fc35ba56c3be49555b703f0e853d4c58225537acf1e)

# platform-specific:
ly_associate_package(PACKAGE_NAME AWSGameLiftServerSDK-3.4.1-rev1-linux             TARGETS AWSGameLiftServerSDK        PACKAGE_HASH a8149a95bd100384af6ade97e2b21a56173740d921e6c3da8188cd51554d39af)
ly_associate_package(PACKAGE_NAME freetype-2.10.4.14-linux                          TARGETS freetype                    PACKAGE_HASH 9ad246873067717962c6b780d28a5ce3cef3321b73c9aea746a039c798f52e93)
ly_associate_package(PACKAGE_NAME tiff-4.2.0.15-linux                               TARGETS tiff                        PACKAGE_HASH ae92b4d3b189c42ef644abc5cac865d1fb2eb7cb5622ec17e35642b00d1a0a76)
ly_associate_package(PACKAGE_NAME AWSNativeSDK-1.7.167-rev5-linux                   TARGETS AWSNativeSDK                PACKAGE_HASH 0101a4052d9fce83a6f5515e00f366e97b308ecb8261ad23a6e4eb4365212ab6)
ly_associate_package(PACKAGE_NAME Lua-5.3.5-rev5-linux                              TARGETS Lua                         PACKAGE_HASH 1adc812abe3dd0dbb2ca9756f81d8f0e0ba45779ac85bf1d8455b25c531a38b0)
ly_associate_package(PACKAGE_NAME PhysX-4.1.2.29882248-rev3-linux                   TARGETS PhysX                       PACKAGE_HASH a110249cbef4f266b0002c4ee9a71f59f373040cefbe6b82f1e1510c811edde6)
ly_associate_package(PACKAGE_NAME etc2comp-9cd0f9cae0-rev1-linux                    TARGETS etc2comp                    PACKAGE_HASH 9283aa5db5bb7fb90a0ddb7a9f3895317c8ebe8044943124bbb3673a41407430)
ly_associate_package(PACKAGE_NAME mcpp-2.7.2_az.1-rev1-linux                        TARGETS mcpp                        PACKAGE_HASH 0aa713f3f2c156cb2f17d9b800aed8acf9df5ab167c48b679853ecb040da9a67)
ly_associate_package(PACKAGE_NAME mikkelsen-1.0.0.4-linux                           TARGETS mikkelsen                   PACKAGE_HASH 5973b1e71a64633588eecdb5b5c06ca0081f7be97230f6ef64365cbda315b9c8)
ly_associate_package(PACKAGE_NAME googletest-1.8.1-rev4-linux                       TARGETS googletest                  PACKAGE_HASH 7b7ad330f369450c316a4c4592d17fbb4c14c731c95bd8f37757203e8c2bbc1b)
ly_associate_package(PACKAGE_NAME googlebenchmark-1.5.0-rev2-linux                  TARGETS GoogleBenchmark             PACKAGE_HASH 4038878f337fc7e0274f0230f71851b385b2e0327c495fc3dd3d1c18a807928d)
ly_associate_package(PACKAGE_NAME unwind-1.2.1-linux                                TARGETS unwind                      PACKAGE_HASH 3453265fb056e25432f611a61546a25f60388e315515ad39007b5925dd054a77)
ly_associate_package(PACKAGE_NAME qt-5.15.2-rev4-linux                              TARGETS Qt                          PACKAGE_HASH 1122e0ec19b01cb02a11fcf34dbf884bc9049ba5ff04fb692bfb09d4e5ee1e6b)
ly_associate_package(PACKAGE_NAME libsamplerate-0.2.1-rev2-linux                    TARGETS libsamplerate               PACKAGE_HASH 41643c31bc6b7d037f895f89d8d8d6369e906b92eff42b0fe05ee6a100f06261)
ly_associate_package(PACKAGE_NAME OpenSSL-1.1.1b-rev2-linux                         TARGETS OpenSSL                     PACKAGE_HASH b779426d1e9c5ddf71160d5ae2e639c3b956e0fb5e9fcaf9ce97c4526024e3bc)
ly_associate_package(PACKAGE_NAME DirectXShaderCompilerDxc-1.6.2104-o3de-rev2-linux TARGETS DirectXShaderCompilerDxc    PACKAGE_HASH 235606f98512c076a1ba84a8402ad24ac21945998abcea264e8e204678efc0ba)
