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

set(CLANG_PLATFORM_LIB_PATH ${BASE_PATH}/xcode/$<IF:$<CONFIG:Debug>,debug,release>/lib)

set(CLANG_INCLUDE_DIRECTORIES
    llvm/include
    xcode/$<IF:$<CONFIG:Debug>,debug,release>/include
)

set(CLANG_LIBS
    ${CLANG_PLATFORM_LIB_PATH}/libclangFrontend.a
    ${CLANG_PLATFORM_LIB_PATH}/libclangSerialization.a
    ${CLANG_PLATFORM_LIB_PATH}/libclangDriver.a
    ${CLANG_PLATFORM_LIB_PATH}/libclangTooling.a
    ${CLANG_PLATFORM_LIB_PATH}/libclangParse.a
    ${CLANG_PLATFORM_LIB_PATH}/libclangSema.a
    ${CLANG_PLATFORM_LIB_PATH}/libclangAnalysis.a
    ${CLANG_PLATFORM_LIB_PATH}/libclangRewriteFrontend.a
    ${CLANG_PLATFORM_LIB_PATH}/libclangRewrite.a
    ${CLANG_PLATFORM_LIB_PATH}/libclangEdit.a
    ${CLANG_PLATFORM_LIB_PATH}/libclangAST.a
    ${CLANG_PLATFORM_LIB_PATH}/libclangASTMatchers.a
    ${CLANG_PLATFORM_LIB_PATH}/libclangLex.a
    ${CLANG_PLATFORM_LIB_PATH}/libclangBasic.a
    ${CLANG_PLATFORM_LIB_PATH}/libLLVMCore.a
    ${CLANG_PLATFORM_LIB_PATH}/libLLVMBinaryFormat.a
    ${CLANG_PLATFORM_LIB_PATH}/libLLVMDebugInfoDWARF.a
    ${CLANG_PLATFORM_LIB_PATH}/libLLVMMC.a
    ${CLANG_PLATFORM_LIB_PATH}/libLLVMOption.a
    ${CLANG_PLATFORM_LIB_PATH}/libLLVMBitReader.a
    ${CLANG_PLATFORM_LIB_PATH}/libLLVMMCParser.a
    ${CLANG_PLATFORM_LIB_PATH}/libLLVMProfileData.a
    ${CLANG_PLATFORM_LIB_PATH}/libLLVMTarget.a
    ${CLANG_PLATFORM_LIB_PATH}/libLLVMSupport.a

    ${CLANG_PLATFORM_LIB_PATH}/libLLVMDemangle.a
    ${CLANG_PLATFORM_LIB_PATH}/libLLVMSupport.a
    ${CLANG_PLATFORM_LIB_PATH}/libLLVMCore.a

)
