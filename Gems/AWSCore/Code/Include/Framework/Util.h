/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include "AzCore/StringFunc/StringFunc.h"
#include <aws/core/utils/memory/stl/AWSString.h>

namespace AWSCore
{
    namespace Util
    {
        //! An enum representing the available data provided in an AWS ARN (Amazon Resource Name)
        //! ARN formats:
        //!     arn:partition:service:region:account-id:resource-id
        //!     arn:partition:service:region:account-id:resource-type/resource-id
        //!     arn:partition:service:region:account-id:resource-type:resource-id
        //! Be aware that the ARNs for some resources omit the Region, the account ID, or both the Region and the account ID.
        //! Example of GameLift Fleet ARN:
        //!     "arn:aws:gamelift:us-west-2:<account id>:fleet/fleet-<fleet id>"
        enum ArnFormatDataIndex
        {
            Partition = 1,
            Service,
            Region,
            AccountId
        };

        //! Extracts information from an AWS ARN (Amazon Resource Name)
        //! @param awsArn An AWS ARN. ARNs are formatted as arn:partition:service:region:account-id:<resource-type/id>
        //! @param arnDataIndex The type of ARN data to extract. Possible values are Partition, Service, Region, or AccountId
        //! @return A string representing extracted ARN data. Empty string is returned if data type isn't found.
        //! Be aware that the ARNs for some resources omit the Region, the account ID, or both the Region and the account ID.
        inline AZStd::string ExtractArnData(AZStd::string_view awsArn, ArnFormatDataIndex arnDataIndex)
        {
            AZStd::vector<AZStd::string> tokenizedGameLiftArn;
            AZ::StringFunc::Tokenize(awsArn, tokenizedGameLiftArn, ":");
            if (tokenizedGameLiftArn.size() >= arnDataIndex)
            {
                return tokenizedGameLiftArn[arnDataIndex];
            }
            return "";
        }

        //! Extracts the AWS region from a given ARN (Amazon Resource Name)
        //! @param awsArn An AWS ARN. ARNs are formatted as arn:partition:service:region:account-id:<resource-type/id>
        //! @return A string representing the AWS region or an empty string if no region is found.
        inline AZStd::string ExtractRegion(AZStd::string_view awsArn)
        {
            return ExtractArnData(awsArn, ArnFormatDataIndex::Region);
        }

        inline Aws::String ToAwsString(const AZStd::string& s)
        {
            return Aws::String(s.c_str(), s.length());
        }

        inline AZStd::string ToAZString(const Aws::String& s)
        {
            return AZStd::string(s.c_str(), s.length());
        }
    }

} // namespace AWSCore
