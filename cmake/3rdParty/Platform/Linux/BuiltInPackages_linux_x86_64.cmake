#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# this file allows you to specify all 3p packages (provided by O3DE or the operating system) for Linux.

ly_associate_package(PACKAGE_NAME qt-6.11.2-rev1-linux                              TARGETS Qt                          PACKAGE_HASH a2b06973f03f6ded97aaf3bbd71cdd972ba424e91439373e99c0a3f998fcff78)
ly_associate_package(PACKAGE_NAME openimageio-opencolorio-2.3.17-rev2-linux         TARGETS OpenImageIO OpenColorIO OpenColorIO::Runtime OpenImageIO::Tools::Binaries OpenImageIO::Tools::PythonPlugins PACKAGE_HASH c8a9f1d9d6c9f8c3defdbc3761ba391d175b1cb62a70473183af1eaeaef70c36)
ly_associate_package(PACKAGE_NAME OpenEXR-3.1.3-rev4-linux                          TARGETS OpenEXR Imath               PACKAGE_HASH fcbac68cfb4e3b694580bc3741443e111aced5f08fde21a92e0c768e8803c7af)
ly_associate_package(PACKAGE_NAME OpenSSL-1.1.1t-rev1-linux                         TARGETS OpenSSL                     PACKAGE_HASH 63aea898b7afe8faccd0c7261e62d2f8b7b870f678a4520d5be81e5815542b39)
ly_associate_package(PACKAGE_NAME DirectXShaderCompilerDxc-1.8.2505.1-o3de-rev3-linux TARGETS DirectXShaderCompilerDxc    PACKAGE_HASH 396929d1fa237c49973244f8406a87d84430f653cb1a95b929a8b884f4967a75)
ly_associate_package(PACKAGE_NAME ISPCTexComp-36b80aa-rev1-linux                    TARGETS ISPCTexComp                 PACKAGE_HASH 065fd12abe4247dde247330313763cf816c3375c221da030bdec35024947f259)
ly_associate_package(PACKAGE_NAME pyside6-6.11.2-py3.10-rev1-linux                  TARGETS pyside6                     PACKAGE_HASH fd2c2065e95a94459fc8da68bb2460c2473148313833b49ef8ba9bb55be5b35b)
ly_associate_package(PACKAGE_NAME vulkan-validationlayers-1.2.198-rev1-linux        TARGETS vulkan-validationlayers     PACKAGE_HASH 9195c7959695bcbcd1bc1dc5c425c14639a759733b3abe2ffa87eb3915b12c71)
