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

set(GOOGLETEST_PACKAGE_NAME googletest)
set(GOOGLETEST_PACKAGE_VERSION 1.8.1-az.3)

ly_add_external_target(
    NAME GTest
    PACKAGE ${GOOGLETEST_PACKAGE_NAME} 
    VERSION ${GOOGLETEST_PACKAGE_VERSION}
    INCLUDE_DIRECTORIES include
    COMPILE_DEFINITIONS GTEST_HAS_EXCEPTIONS=0
)

ly_add_external_target(
    NAME GTestMain
    PACKAGE ${GOOGLETEST_PACKAGE_NAME} 
    VERSION ${GOOGLETEST_PACKAGE_VERSION}
    INCLUDE_DIRECTORIES include
)

ly_add_external_target(
    NAME GMock
    PACKAGE ${GOOGLETEST_PACKAGE_NAME}
    VERSION ${GOOGLETEST_PACKAGE_VERSION}
    INCLUDE_DIRECTORIES include
)

ly_add_external_target(
    NAME GMockMain
    PACKAGE ${GOOGLETEST_PACKAGE_NAME} 
    VERSION ${GOOGLETEST_PACKAGE_VERSION}
    INCLUDE_DIRECTORIES include
)
