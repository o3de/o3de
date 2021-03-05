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
    class Quaternion;
    class Vector3;

    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IOriginRule
                : public IRule
            {
            public:
                AZ_RTTI(IOriginRule, "{9FB042DF-1C7F-4815-BC83-BF8D94F907C5}", IRule);

                virtual ~IOriginRule() override = default;

                virtual const AZStd::string& GetOriginNodeName() const = 0;
                virtual bool UseRootAsOrigin() const = 0;
                virtual const AZ::Quaternion& GetRotation() const = 0;
                virtual const AZ::Vector3& GetTranslation() const = 0;
                virtual float GetScale() const = 0;
            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ
