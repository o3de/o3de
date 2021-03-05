#pragma once

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

#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            const static AZStd::string s_advancedDisabledString = "Disabled";
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
