#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

ly_add_3rdparty_target(
    NAME rapidxml HEADERONLY
    INCLUDE_DIRECTORIES
        INTERFACE Include
)

ly_create_alias(NAME RapidXML NAMESPACE 3rdParty TARGETS 3rdParty::rapidxml)
