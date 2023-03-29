#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# QtForPythonSystemComponent uses a try catch block
if(MSVC)
    set(LY_COMPILE_OPTIONS PRIVATE /EHsc)
else()
    set(LY_COMPILE_OPTIONS
    PRIVATE
        -fexceptions
    )
endif()
