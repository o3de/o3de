/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace LmbrCentral::GenericAssetBuilder
{
    //! "Registration" represents a registered type for the Generic Asset Builder to build.
    //! You can make any number of these in response to the callback.
    struct GenericAssetBuilderRegistration
    {
        AZStd::string m_pattern; //! The pattern to use to identify it, like "*.myasset"
        AZ::Uuid m_outputAssetTypeId; //! The typeid to assign it in the asset database.
                                      //!  This should match the typeid (the guid) of the asset type, ie,
                                      //!  the typeid of the template parameter T inside AZ::Data::Asset<T>
    };
} // namespace LmbrCentral::GenericAssetBuilderAPI
