/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <memory>
#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/IManifestObject.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IRule
                : public IManifestObject
            {
            public:
                AZ_RTTI(IRule, "{81267F8B-3963-423B-9FF7-D276D82CD110}", IManifestObject);

                virtual ~IRule() override = default;

                virtual bool ModifyTooltip(AZStd::string& /*tooltip*/) {return false; }
            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ
