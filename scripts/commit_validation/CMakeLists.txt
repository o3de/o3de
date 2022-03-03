#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_pytest(
    NAME test_commit_validation
    PATH ${CMAKE_CURRENT_LIST_DIR}/commit_validation/tests
    TEST_SUITE smoke
)
