#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>

namespace AZ::SceneAPI::DataTypes
{
    class ICustomPropertyData
        : public IGraphObject
    {
    public:
        AZ_RTTI(ICustomPropertyData, "{A524B276-A4C5-496D-A4E5-C219A78DF58D}", IGraphObject);
        using PropertyMap = AZStd::unordered_map<AZStd::string, AZStd::any>;

        ~ICustomPropertyData() override = default;
        void CloneAttributesFrom([[maybe_unused]] const IGraphObject* sourceObject) override {}

        virtual PropertyMap& GetPropertyMap() = 0;
        virtual const PropertyMap& GetPropertyMap() const = 0;
    };
}
