#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# DEPRECATED: The contents of AtomCore have been relocated into AzCore.
# This target now exists only as a set of forwarding-header shims that redirect to their <AzCore/...> counterparts,
# plus an interface dependency on AZ::AzCore so existing consumers of AZ::AtomCore keep resolving.
ly_add_target(
    NAME AtomCore HEADERONLY
    NAMESPACE AZ
    FILES_CMAKE
        AtomCore/atomcore_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            .
    BUILD_DEPENDENCIES
        INTERFACE
            AZ::AzCore
)
