/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
