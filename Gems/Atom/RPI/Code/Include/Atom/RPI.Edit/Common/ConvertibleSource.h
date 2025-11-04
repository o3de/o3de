/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Edit/Configuration.h>
#include <AzCore/base.h>
#include <AzCore/RTTI/TypeInfoSimple.h>

namespace AZStd
{
    template<class T>
    class shared_ptr;
}
namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! This class extends the functionality of AnyAssetBuilder so it can
        //! convert source asset to product asset which are different types.
        //! This should be used as base class for the source data type which can to be converted to another data type.
        class ATOM_RPI_EDIT_API ConvertibleSource
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
