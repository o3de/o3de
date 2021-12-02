#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# this gem uses the ASN1 package on IOS.
ly_associate_package(PACKAGE_NAME asn1-0.9.27-rev2-ios TARGETS ASN1 PACKAGE_HASH 314e4113930283f88fc65239470daa2aca776f0a11b67d3f09164ac72cb82c56)

find_library(STOREKIT_FRAMEWORK StoreKit)

set(LY_BUILD_DEPENDENCIES
    PRIVATE
        ${STOREKIT_FRAMEWORK}
        3rdParty::OpenSSL
        3rdParty::ASN1
)

