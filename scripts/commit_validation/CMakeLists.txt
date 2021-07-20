#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# this ctest makes sure that the commit validation function
# also runs its tests during commit validation!
ly_add_pytest(
    NAME test_commit_validation
    PATH ${CMAKE_CURRENT_LIST_DIR}
)

