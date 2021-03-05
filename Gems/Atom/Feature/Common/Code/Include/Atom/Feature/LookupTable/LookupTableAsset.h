/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <Atom/RPI.Reflect/Pass/PassAsset.h>
#include <AzCore/Asset/AssetCommon.h>

namespace AZ
{
    class ReflectContext;

    namespace Render
    {
        //! A descriptor used to configure the DisplayMapper
        struct LookupTableAsset final
        {
            AZ_TYPE_INFO(LookupTableAsset, "{15B38E83-CCEC-4561-BFA5-0A367FDBCF9E}");
            static void Reflect(AZ::ReflectContext* context);

            //! Lut image name
            AZStd::string m_name;

            AZStd::vector<AZ::u64> m_intervals;
            AZStd::vector<AZ::u64> m_values;
        };

    } // namespace RPI
} // namespace AZ
