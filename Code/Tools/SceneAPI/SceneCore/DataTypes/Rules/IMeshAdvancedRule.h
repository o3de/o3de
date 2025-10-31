/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <AzCore/std/string/fixed_string.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            static constexpr const char* s_advancedDisabledString = "Disabled";

            class IMeshAdvancedRule
                : public IRule
            {
            public:
                AZ_RTTI(IMeshAdvancedRule, "{ADF04C44-5786-466A-AE69-63E5E3474D21}", IRule);

                virtual ~IMeshAdvancedRule() override = default;

                virtual bool Use32bitVertices() const = 0;
                virtual bool MergeMeshes() const = 0;
                virtual bool UseCustomNormals() const = 0;
                virtual const AZStd::string& GetVertexColorStreamName() const = 0;
                // Returns whether or not the vertex color stream was explicitly disabled by the user.
                //      This does guarantee a valid vertex color stream name if false.
                virtual bool IsVertexColorStreamDisabled() const = 0;
            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ
