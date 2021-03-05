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

#include <AzCore/base.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        //! This class extends the functionality of AnyAssetBuilder so it can 
        //! convert source asset to product asset which are different types.
        //! This should be used as base class for the source data type which can to be converted to another data type.
        class ConvertibleSource
        {
            public:
                AZ_TYPE_INFO(ConvertibleSource, "{478DDAAC-E4A4-47B1-8E83-62F239D5BB57}");

                static void Reflect(ReflectContext* context);

                virtual ~ConvertibleSource() = default;

                /// Convert the data from this class to another and output its class id and data
                virtual bool Convert(TypeId& outTypeId, AZStd::shared_ptr<void>& outData) const;
        };
    }
}
