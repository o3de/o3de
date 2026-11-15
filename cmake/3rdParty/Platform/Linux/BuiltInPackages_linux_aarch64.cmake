#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# this file allows you to specify all 3p packages (provided by O3DE or the operating system) for Linux.

ly_associate_package(PACKAGE_NAME qt-6.11.2-rev1-linux-aarch64                               TARGETS Qt                          PACKAGE_HASH b62b478b9df69335d66b80373de55bd0db1c56b8301b216bf179637dad60e96a)
ly_associate_package(PACKAGE_NAME openimageio-opencolorio-2.3.17-rev2-linux-aarch64          TARGETS OpenImageIO OpenColorIO OpenColorIO::Runtime OpenImageIO::Tools::Binaries OpenImageIO::Tools::PythonPlugins PACKAGE_HASH 2bc6a43f60c8206b2606a65738e0fcf3b3b17e0db16089404d8389d337c85ad6)
ly_associate_package(PACKAGE_NAME OpenEXR-3.1.3-rev4-linux-aarch64                           TARGETS OpenEXR Imath               PACKAGE_HASH c9a81050f0d550ab03d2f5801e2f67f9f02747c26f4b39647e9919278585ad6a)
ly_associate_package(PACKAGE_NAME OpenSSL-1.1.1t-rev1-linux-aarch64                          TARGETS OpenSSL                     PACKAGE_HASH f32721bec9c82d1bd7fb244d78d5dc4e2a47e7b808bb36027236ad377e241ea5)
ly_associate_package(PACKAGE_NAME DirectXShaderCompilerDxc-1.8.2505.1-o3de-rev3-linux-aarch64 TARGETS DirectXShaderCompilerDxc    PACKAGE_HASH 7cb521963ae5f4fcd2c488c0ddf8cddc2aeebe3dd37e48a528d87cb541b2f511)
ly_associate_package(PACKAGE_NAME ISPCTexComp-36b80aa-rev2-linux-aarch64                     TARGETS ISPCTexComp                 PACKAGE_HASH c29aafa32f13839a394424cf674b5cdb323fab22bcca43c38b43adfe13fc415c)
ly_associate_package(PACKAGE_NAME vulkan-validationlayers-1.2.198-rev1-linux-aarch64         TARGETS vulkan-validationlayers     PACKAGE_HASH e67a15a95e14397ccdffd70d17f61079e5720fea22b0d21e135497312419a23f)
ly_associate_package(PACKAGE_NAME pyside6-6.11.2-py3.10-rev1-linux-aarch64                   TARGETS pyside6                     PACKAGE_HASH f0211df6ad1222af1cf17b4f0a9a6a0bcfe5d722ba178778fd31359b0175ac5a)
