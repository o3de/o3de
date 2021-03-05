#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

set(CLANG_PLATFORM_LIB_PATH ${BASE_PATH}/vs2015/$<IF:$<CONFIG:Debug>,debug,release>/lib)

set(CLANG_INCLUDE_DIRECTORIES
    llvm/include
    vs2015/$<IF:$<CONFIG:Debug>,debug,release>/include
)

set(CLANG_LIBS
    ${CLANG_PLATFORM_LIB_PATH}/clangFrontend.lib
    ${CLANG_PLATFORM_LIB_PATH}/clangSerialization.lib
    ${CLANG_PLATFORM_LIB_PATH}/clangDriver.lib
    ${CLANG_PLATFORM_LIB_PATH}/clangTooling.lib
    ${CLANG_PLATFORM_LIB_PATH}/clangParse.lib
    ${CLANG_PLATFORM_LIB_PATH}/clangSema.lib
    ${CLANG_PLATFORM_LIB_PATH}/clangAnalysis.lib
    ${CLANG_PLATFORM_LIB_PATH}/clangRewriteFrontend.lib
    ${CLANG_PLATFORM_LIB_PATH}/clangRewrite.lib
    ${CLANG_PLATFORM_LIB_PATH}/clangEdit.lib
    ${CLANG_PLATFORM_LIB_PATH}/clangAST.lib
    ${CLANG_PLATFORM_LIB_PATH}/clangLex.lib
    ${CLANG_PLATFORM_LIB_PATH}/clangBasic.lib
    ${CLANG_PLATFORM_LIB_PATH}/LLVMBinaryFormat.lib
    ${CLANG_PLATFORM_LIB_PATH}/LLVMDebugInfoDWARF.lib
    ${CLANG_PLATFORM_LIB_PATH}/LLVMMC.lib
    ${CLANG_PLATFORM_LIB_PATH}/LLVMOption.lib
    ${CLANG_PLATFORM_LIB_PATH}/LLVMBitReader.lib
    ${CLANG_PLATFORM_LIB_PATH}/LLVMMCParser.lib
    ${CLANG_PLATFORM_LIB_PATH}/LLVMProfileData.lib
    ${CLANG_PLATFORM_LIB_PATH}/LLVMTarget.lib
    ${CLANG_PLATFORM_LIB_PATH}/LLVMCore.lib
    ${CLANG_PLATFORM_LIB_PATH}/LLVMSupport.lib
    Version.lib
)
