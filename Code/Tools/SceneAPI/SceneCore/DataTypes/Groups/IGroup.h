/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/Containers/RuleContainer.h>
#include <SceneAPI/SceneCore/DataTypes/IManifestObject.h>


namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IRule;

            class IGroup
                : public IManifestObject
            {
            public:
                AZ_RTTI(IGroup, "{DE008E67-790D-4672-A73A-5CA0F31EDD2D}", IManifestObject);

                ~IGroup() override = default;

                virtual const AZStd::string& GetName() const = 0;
                virtual const Uuid& GetId() const = 0;

                virtual Containers::RuleContainer& GetRuleContainer() = 0;
                virtual const Containers::RuleContainer& GetRuleContainerConst() const = 0;
            };

        }  // DataTypes
    }  // SceneAPI
}  // AZ
