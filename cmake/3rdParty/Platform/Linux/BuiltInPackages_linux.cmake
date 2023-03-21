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

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")

    ly_associate_package(PACKAGE_NAME cityhash-1.1-multiplatform                        TARGETS cityhash                    PACKAGE_HASH 0ace9e6f0b2438c5837510032d2d4109125845c0efd7d807f4561ec905512dd2)

    # platform-specific:
    ly_associate_package(PACKAGE_NAME expat-2.4.2-rev2-linux                            TARGETS expat                       PACKAGE_HASH 755369a919e744b9b3f835d1acc684f02e43987832ad4a1c0b6bbf884e6cd45b)
    ly_associate_package(PACKAGE_NAME assimp-5.2.5-rev1-linux                           TARGETS assimplib                   PACKAGE_HASH 67bd3625078b63b40ae397ef7a3e589a6f77e95d3318c97dd7075e3e22a638cd)
    ly_associate_package(PACKAGE_NAME AWSGameLiftServerSDK-3.4.2-rev1-linux             TARGETS AWSGameLiftServerSDK        PACKAGE_HASH 875a8ee45ab5948b10eedfd9057b14db7f01c4b31820f8f998eb6dee1c05a176)
    ly_associate_package(PACKAGE_NAME tiff-4.2.0.15-rev3-linux                          TARGETS TIFF                        PACKAGE_HASH 2377f48b2ebc2d1628d9f65186c881544c92891312abe478a20d10b85877409a)
    ly_associate_package(PACKAGE_NAME freetype-2.11.1-rev1-linux                        TARGETS Freetype                    PACKAGE_HASH 28bbb850590507eff85154604787881ead6780e6eeee9e71ed09cd1d48d85983)
    ly_associate_package(PACKAGE_NAME Lua-5.4.4-rev1-linux                              TARGETS Lua                         PACKAGE_HASH d582362c3ef90e1ef175a874abda2265839ffc2e40778fa293f10b443b4697ac)
    ly_associate_package(PACKAGE_NAME mcpp-2.7.2_az.2-rev1-linux                        TARGETS mcpp                        PACKAGE_HASH df7a998d0bc3fedf44b5bdebaf69ddad6033355b71a590e8642445ec77bc6c41)
    ly_associate_package(PACKAGE_NAME mikkelsen-1.0.0.4-linux                           TARGETS mikkelsen                   PACKAGE_HASH 5973b1e71a64633588eecdb5b5c06ca0081f7be97230f6ef64365cbda315b9c8)
    ly_associate_package(PACKAGE_NAME googletest-1.8.1-rev4-linux                       TARGETS googletest                  PACKAGE_HASH 7b7ad330f369450c316a4c4592d17fbb4c14c731c95bd8f37757203e8c2bbc1b)
    ly_associate_package(PACKAGE_NAME googlebenchmark-1.7.0-rev1-linux                  TARGETS GoogleBenchmark             PACKAGE_HASH 230e1881e31490820f0bd2059df4741455b52809ac73367e278e1e821ac89c9b)
    ly_associate_package(PACKAGE_NAME qt-5.15.2-rev8-linux                              TARGETS Qt                          PACKAGE_HASH 613d6a404b305ce0e715c57c936dc00318fb9f0d2d3f6609f8454c198f993095)
    ly_associate_package(PACKAGE_NAME png-1.6.37-rev2-linux                             TARGETS PNG                         PACKAGE_HASH 5c82945a1648905a5c4c5cee30dfb53a01618da1bf58d489610636c7ade5adf5)
    ly_associate_package(PACKAGE_NAME libsamplerate-0.2.1-rev2-linux                    TARGETS libsamplerate               PACKAGE_HASH 41643c31bc6b7d037f895f89d8d8d6369e906b92eff42b0fe05ee6a100f06261)
    ly_associate_package(PACKAGE_NAME openimageio-opencolorio-2.3.17-rev2-linux         TARGETS OpenImageIO OpenColorIO OpenColorIO::Runtime OpenImageIO::Tools::Binaries OpenImageIO::Tools::PythonPlugins PACKAGE_HASH c8a9f1d9d6c9f8c3defdbc3761ba391d175b1cb62a70473183af1eaeaef70c36)
    ly_associate_package(PACKAGE_NAME OpenMesh-8.1-rev3-linux                           TARGETS OpenMesh                    PACKAGE_HASH 805bd0b24911bb00c7f575b8c3f10d7ea16548a5014c40811894a9445f17a126)
    ly_associate_package(PACKAGE_NAME OpenEXR-3.1.3-rev4-linux                          TARGETS OpenEXR Imath               PACKAGE_HASH fcbac68cfb4e3b694580bc3741443e111aced5f08fde21a92e0c768e8803c7af)
    ly_associate_package(PACKAGE_NAME DirectXShaderCompilerDxc-1.6.2112-o3de-rev1-linux TARGETS DirectXShaderCompilerDxc    PACKAGE_HASH ac9f98e0e3b07fde0f9bbe1e6daa386da37699819cde06dcc8d3235421f6e977)
    ly_associate_package(PACKAGE_NAME SPIRVCross-2021.04.29-rev1-linux                  TARGETS SPIRVCross                  PACKAGE_HASH 7889ee5460a688e9b910c0168b31445c0079d363affa07b25d4c8aeb608a0b80)
    ly_associate_package(PACKAGE_NAME azslc-1.8.15-rev2-linux                           TARGETS azslc                       PACKAGE_HASH a29b9fd5123f2fe9dc99040180c6e15e5c72f0cf72da63671e6602f80c0349a5)
    ly_associate_package(PACKAGE_NAME zlib-1.2.11-rev5-linux                            TARGETS ZLIB                        PACKAGE_HASH 9be5ea85722fc27a8645a9c8a812669d107c68e6baa2ca0740872eaeb6a8b0fc)
    ly_associate_package(PACKAGE_NAME squish-ccr-deb557d-rev1-linux                     TARGETS squish-ccr                  PACKAGE_HASH 85fecafbddc6a41a27c5f59ed4a5dfb123a94cb4666782cf26e63c0a4724c530)
    ly_associate_package(PACKAGE_NAME astc-encoder-3.2-rev2-linux                       TARGETS astc-encoder                PACKAGE_HASH 71549d1ca9e4d48391b92a89ea23656d3393810e6777879f6f8a9def2db1610c)
    ly_associate_package(PACKAGE_NAME ISPCTexComp-36b80aa-rev1-linux                    TARGETS ISPCTexComp                 PACKAGE_HASH 065fd12abe4247dde247330313763cf816c3375c221da030bdec35024947f259)
    ly_associate_package(PACKAGE_NAME lz4-1.9.3-vcpkg-rev4-linux                        TARGETS lz4                         PACKAGE_HASH 5de3dbd3e2a3537c6555d759b3c5bb98e5456cf85c74ff6d046f809b7087290d)
    ly_associate_package(PACKAGE_NAME pyside2-5.15.2.1-py3.10-rev6-linux                TARGETS pyside2                     PACKAGE_HASH 0e39a7f775e87516bf241acec2fbc437ed6b1fd2b99282d2490e0df7882ec567)
    ly_associate_package(PACKAGE_NAME SQLite-3.37.2-rev1-linux                          TARGETS SQLite                      PACKAGE_HASH bee80d6c6db3e312c1f4f089c90894436ea9c9b74d67256d8c1fb00d4d81fe46)
    ly_associate_package(PACKAGE_NAME AwsIotDeviceSdkCpp-1.15.2-rev1-linux              TARGETS AwsIotDeviceSdkCpp          PACKAGE_HASH 83fc1711404d3e5b2faabb1134e97cc92b748d8b87ff4ea99599d8c750b8eff0)
    ly_associate_package(PACKAGE_NAME vulkan-validationlayers-1.2.198-rev1-linux        TARGETS vulkan-validationlayers     PACKAGE_HASH 9195c7959695bcbcd1bc1dc5c425c14639a759733b3abe2ffa87eb3915b12c71)

    set(AZ_USE_PHYSX5 OFF CACHE BOOL "When ON PhysX Gem will use PhysX 5 SDK")
    if(AZ_USE_PHYSX5)
        ly_associate_package(PACKAGE_NAME PhysX-5.1.1-rev1-linux                            TARGETS PhysX                       PACKAGE_HASH 343e3123882f53748c938c41f4d1c9ab6ec171e4f970ccd9263c5ab191110410)
    else()
        ly_associate_package(PACKAGE_NAME PhysX-4.1.2.29882248-rev5-linux                   TARGETS PhysX                       PACKAGE_HASH fa72365df409376aef02d1763194dc91d255bdfcb4e8febcfbb64d23a3e50b96)
    endif()

    # Certain packages are built against OpenSSL, so we must associate the proper ones based on their OpenSSL (Major) versions
    if ("${OPENSSL_VERSION}" STREQUAL "")
        message(FATAL_ERROR "OpenSSL not detected. The OpenSSL dev package is required for O3DE")
    elseif ("${OPENSSL_VERSION}" VERSION_LESS "3.0.0")
        ly_associate_package(PACKAGE_NAME AWSNativeSDK-1.9.50-rev2-linux-openssl-1          TARGETS AWSNativeSDK                PACKAGE_HASH d4489e9970dadcab52e1db17d47242c2a66478e51c5f1434f9143eeaff5c3223)
    else()
        ly_associate_package(PACKAGE_NAME AWSNativeSDK-1.9.50-rev2-linux-openssl-3          TARGETS AWSNativeSDK                PACKAGE_HASH 9b9b5124791fb2f59b7362a95ed997944aff6cc928b9dede916e8968a09f23ff)
    endif()

elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")

    # platform-specific:
    ly_associate_package(PACKAGE_NAME expat-2.4.2-rev2-linux-aarch64                             TARGETS expat                       PACKAGE_HASH 934a535c1492d11906789d7ddf105b1a530cf8d8fb126063ffde16c5caeb0179)
    ly_associate_package(PACKAGE_NAME assimp-5.2.5-rev1-linux-aarch64                            TARGETS assimplib                   PACKAGE_HASH 0e497e129f9868088c81891e87b778894c12486b039a5b7bd8a47267275b640f)
    ly_associate_package(PACKAGE_NAME AWSGameLiftServerSDK-3.4.2-rev1-linux-aarch64              TARGETS AWSGameLiftServerSDK        PACKAGE_HASH f4ac3f8e6e9e0fce66bb3a486d0e36078d3f3d37204a9e3e95031348a021865f)
    ly_associate_package(PACKAGE_NAME cityhash-1.1-rev1-linux-aarch64                            TARGETS cityhash                    PACKAGE_HASH c4fafa13add81c6ca03338462af78eabbdea917de68c599f11c4a36b0982cec2)
    ly_associate_package(PACKAGE_NAME tiff-4.2.0.15-rev3-linux-aarch64                           TARGETS TIFF                        PACKAGE_HASH 429461014b21a530dcad597c2d91072ae39d937a04b7bbbf5c34491c41767f7f)
    ly_associate_package(PACKAGE_NAME freetype-2.11.1-rev1-linux-aarch64                         TARGETS Freetype                    PACKAGE_HASH b4e3069acdcdae2f977108679d0986fb57371b9a7d4a3a496ab16909feabcba6)
    ly_associate_package(PACKAGE_NAME Lua-5.4.4-rev1-linux-aarch64                               TARGETS Lua                         PACKAGE_HASH 4d30067fc494ac27acd72b0bf18099c19c0a44ac9bd46b23db66ad780e72374a)
    ly_associate_package(PACKAGE_NAME mcpp-2.7.2_az.1-rev1-linux-aarch64                         TARGETS mcpp                        PACKAGE_HASH 817d31b94d1217b6e47bd5357b3a07a79ab6aa93452c65ff56831d0590c5169d)
    ly_associate_package(PACKAGE_NAME mikkelsen-1.0.0.4-linux-aarch64                            TARGETS mikkelsen                   PACKAGE_HASH 62f3f316c971239a2b86d7c47a68fee9be744de3a4f9b00533b32f33a4764f8b)
    ly_associate_package(PACKAGE_NAME googletest-1.8.1-rev4-linux-aarch64                        TARGETS googletest                  PACKAGE_HASH 01e236a9b5992da2494227ce23ba2f9aa6ea73a232aeceb3606fbf41954dc6d0)
    ly_associate_package(PACKAGE_NAME googlebenchmark-1.7.0-rev1-linux-aarch64                   TARGETS GoogleBenchmark             PACKAGE_HASH 06fbfeaba2aeae20197da631019e52105dc1f69e702151a76c6aba2c27c03acb)
    ly_associate_package(PACKAGE_NAME qt-5.15.2-rev8-linux-aarch64                               TARGETS Qt                          PACKAGE_HASH c437ee1c7a4fe84002352a2f8ed230c822a13dcc80735a4fecf3b3af6e34bb63)
    ly_associate_package(PACKAGE_NAME png-1.6.37-rev2-linux-aarch64                              TARGETS PNG                         PACKAGE_HASH fcf646c1b1b4163000efdb56d7c8f086b6ce0a520da5c8d3ffce4e1329ae798a)
    ly_associate_package(PACKAGE_NAME libsamplerate-0.2.1-rev2-linux-aarch64                     TARGETS libsamplerate               PACKAGE_HASH 751484da1527432cd19263909f69164d67b25644f87ec1d4ec974a343defacea)
    ly_associate_package(PACKAGE_NAME openimageio-opencolorio-2.3.17-rev2-linux-aarch64          TARGETS OpenImageIO OpenColorIO OpenColorIO::Runtime OpenImageIO::Tools::Binaries OpenImageIO::Tools::PythonPlugins PACKAGE_HASH 2bc6a43f60c8206b2606a65738e0fcf3b3b17e0db16089404d8389d337c85ad6)
    ly_associate_package(PACKAGE_NAME OpenMesh-8.1-rev3-linux-aarch64                            TARGETS OpenMesh                    PACKAGE_HASH 0d53d215c4b2185879e3b27d1a4bdf61a53bcdb059eae30377ea4573bcd9ebc1)
    ly_associate_package(PACKAGE_NAME OpenEXR-3.1.3-rev4-linux-aarch64                           TARGETS OpenEXR Imath               PACKAGE_HASH c9a81050f0d550ab03d2f5801e2f67f9f02747c26f4b39647e9919278585ad6a)
    ly_associate_package(PACKAGE_NAME DirectXShaderCompilerDxc-1.6.2112-o3de-rev1-linux-aarch64  TARGETS DirectXShaderCompilerDxc PACKAGE_HASH 13175d23c17c4d510e609d857b103922157293c3635ae03441245562e056c262)
    ly_associate_package(PACKAGE_NAME SPIRVCross-2021.04.29-rev1-linux-aarch64                   TARGETS SPIRVCross                  PACKAGE_HASH d50be7e605d632d15be3c61b651ecca99e9274a6f8f448ee0d72470f383c1689)
    ly_associate_package(PACKAGE_NAME azslc-1.8.9-rev2-linux-aarch64                             TARGETS azslc                       PACKAGE_HASH e94e87d6d92ec57b6f2778825631a7f08f9796edbeb9c61f89e666d516b66ead)
    ly_associate_package(PACKAGE_NAME zlib-1.2.11-rev5-linux-aarch64                             TARGETS ZLIB                        PACKAGE_HASH ce9d1ed2883d77ffc69c7982c078595c1f89ca55ec19d89fe7e6beb05f774775)
    ly_associate_package(PACKAGE_NAME squish-ccr-deb557d-rev1-linux-aarch64                      TARGETS squish-ccr                  PACKAGE_HASH d3e54df2defff9f9254085acbf7c61dfda56f72ad10d34e1dd3b5d1bd2b8129f)
    ly_associate_package(PACKAGE_NAME astc-encoder-3.2-rev3-linux-aarch64                        TARGETS astc-encoder                PACKAGE_HASH 60ef2a8adc15767dc263860e1e3befc2f3acea26987442a7e80783f1b2158c73)
    ly_associate_package(PACKAGE_NAME ISPCTexComp-36b80aa-rev2-linux-aarch64                     TARGETS ISPCTexComp                 PACKAGE_HASH c29aafa32f13839a394424cf674b5cdb323fab22bcca43c38b43adfe13fc415c)
    ly_associate_package(PACKAGE_NAME lz4-1.9.3-vcpkg-rev4-linux-aarch64                         TARGETS lz4                         PACKAGE_HASH 0dcfc418f8d2e84e162d8c78a9cdde85339cfa99f7066642f066253f808582b5)
    ly_associate_package(PACKAGE_NAME pyside2-5.15.2.1-py3.10-rev4-linux-aarch64                 TARGETS pyside2                     PACKAGE_HASH cc54c4783a645003a74e6a276c75b64b3eaee39f576423f4ebce0130621e2916)
    ly_associate_package(PACKAGE_NAME SQLite-3.37.2-rev1-linux-aarch64                           TARGETS SQLite                      PACKAGE_HASH 5cc1fd9294af72514eba60509414e58f1a268996940be31d0ab6919383f05118)
    ly_associate_package(PACKAGE_NAME AwsIotDeviceSdkCpp-1.15.2-rev1-linux-aarch64               TARGETS AwsIotDeviceSdkCpp          PACKAGE_HASH 0bac80fc09094c4fd89a845af57ebe4ef86ff8d46e92a448c6986f9880f9ee62)
    ly_associate_package(PACKAGE_NAME vulkan-validationlayers-1.2.198-rev1-linux-aarch64         TARGETS vulkan-validationlayers     PACKAGE_HASH e67a15a95e14397ccdffd70d17f61079e5720fea22b0d21e135497312419a23f)

    set(AZ_USE_PHYSX5 OFF CACHE BOOL "When ON PhysX Gem will use PhysX 5 SDK")
    if(AZ_USE_PHYSX5)
        ly_associate_package(PACKAGE_NAME PhysX-5.1.1-rev1-linux-aarch64                         TARGETS PhysX                       PACKAGE_HASH ba14e96fc4b85d5ef5dcf8c8ee58fd37ab92cfef790872d67c9a389aeca24c19)
    else()
        ly_associate_package(PACKAGE_NAME PhysX-4.1.2.29882248-rev5-linux-aarch64                TARGETS PhysX                       PACKAGE_HASH 7fa00d7d4f7532cf068246d4e424ce319529dfa09fb759d251676f2c59f6d50c)
    endif()

    # Certain packages are built against OpenSSL, so we must associate the proper ones based on their OpenSSL (Major) versions
    if ("${OPENSSL_VERSION}" STREQUAL "")
        message(FATAL_ERROR "OpenSSL not detected. The OpenSSL dev package is required for O3DE")
    elseif ("${OPENSSL_VERSION}" VERSION_LESS "3.0.0")
        ly_associate_package(PACKAGE_NAME AWSNativeSDK-1.9.50-rev2-linux-openssl-1-aarch64       TARGETS AWSNativeSDK                PACKAGE_HASH db83fb53946602495e586dfb1ef2dd52018d38d36e0f12c8baea3d1d87518e77)
    else()
        ly_associate_package(PACKAGE_NAME AWSNativeSDK-1.9.50-rev2-linux-openssl-3-aarch64       TARGETS AWSNativeSDK                PACKAGE_HASH 963d97090bf1a9347a4c8d512c393ff34961d3faba4eaa3a2d00e26b54ff36d4)
    endif()
else()
    message(FATAL_ERROR "Unsupport linux architecture ${CMAKE_SYSTEM_PROCESSOR}")
endif()


