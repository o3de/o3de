/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Asset/AssetCommon.h>

namespace PythonAssetBuilder
{
    //! A request bus to help produce Open 3D Engine asset data
    class PythonBuilderRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //! Creates an AZ::Entity populated with Editor components and a name
        virtual AZ::Outcome<AZ::EntityId, AZStd::string> CreateEditorEntity(const AZStd::string& name) = 0;

        //! Writes out a .SLICE file with a given list of entities; optionally can be set to dynamic
        virtual AZ::Outcome<AZ::Data::AssetType, AZStd::string> WriteSliceFile(
            AZStd::string_view filename,
            AZStd::vector<AZ::EntityId> entityList,
            bool makeDynamic) = 0;
    };

    using PythonBuilderRequestBus = AZ::EBus<PythonBuilderRequests>;
}
