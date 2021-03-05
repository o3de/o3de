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

# this gem uses the ASN1 package on IOS.
ly_associate_package(asn1-0.9.27-rev2-ios ASN1)

find_library(STOREKIT_FRAMEWORK StoreKit)

set(LY_BUILD_DEPENDENCIES
    PRIVATE
        ${STOREKIT_FRAMEWORK}
        3rdParty::OpenSSL
        3rdParty::ASN1
)

