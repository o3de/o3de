/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>

namespace AZ::SceneAPI::DataTypes
{
    //! Interface to access the stored custom property key-value node pairs defined in a source scene asset
    class ICustomPropertyData
        : public IGraphObject
    {
    public:
        AZ_RTTI(ICustomPropertyData, "{A524B276-A4C5-496D-A4E5-C219A78DF58D}", IGraphObject);

        //! Custom properties will be coming in as pair of a string key to a supported value type
        //! The supported value types are AZStd::string, bool, int32_t, int64_t, float, and double
        using PropertyMap = AZStd::unordered_map<AZStd::string, AZStd::any>;

        ~ICustomPropertyData() override = default;
        void CloneAttributesFrom([[maybe_unused]] const IGraphObject* sourceObject) override {}

        virtual PropertyMap& GetPropertyMap() = 0;
        virtual const PropertyMap& GetPropertyMap() const = 0;
    };
}
