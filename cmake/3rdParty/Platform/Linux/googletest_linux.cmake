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

set(GOOGLETEST_LIB_PATH ${BASE_PATH}/lib/Linux-libstdcxx/$<IF:$<CONFIG:Debug>,Debug,Release>)

set(GOOGLETEST_GTEST_LIBS ${GOOGLETEST_LIB_PATH}/lib$<IF:$<CONFIG:Debug>,gtestd,gtest>.a)
set(GOOGLETEST_GTESTMAIN_LIBS ${GOOGLETEST_LIB_PATH}/lib$<IF:$<CONFIG:Debug>,gtest_maind,gtest_main>.a)
set(GOOGLETEST_GMOCK_LIBS ${GOOGLETEST_LIB_PATH}/lib$<IF:$<CONFIG:Debug>,gmockd,gmock>.a)
set(GOOGLETEST_GMOCKMAIN_LIBS ${GOOGLETEST_LIB_PATH}/lib$<IF:$<CONFIG:Debug>,gmockd,gmock>.a)

set(GOOGLETEST_COMPILE_DEFINITIONS GTEST_HAS_TR1_TUPLE=0)
