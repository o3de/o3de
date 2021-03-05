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

#include <AzCore/Memory/Memory.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IOriginRule.h>

namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }
        namespace SceneData
        {
            class OriginRule
                : public DataTypes::IOriginRule
            {
            public:
                AZ_RTTI(OriginRule, "{90AECE4A-58D4-411C-9CDE-59B54C59354E}", DataTypes::IOriginRule);
                AZ_CLASS_ALLOCATOR_DECL

                OriginRule();
                ~OriginRule() override = default;

                const AZStd::string& GetOriginNodeName() const override;
                bool UseRootAsOrigin() const override;
                const Quaternion& GetRotation() const override;
                const Vector3& GetTranslation() const override;
                float GetScale() const override;

                void SetOriginNodeName(const AZStd::string& originNodeName);
                void SetRotation(const Quaternion& rotation);
                void SetTranslation(const Vector3& translation);
                void SetScale(float scale);

                static void Reflect(ReflectContext* context);
                
                static const AZStd::string c_defaultWorldUIString;

            protected:
                AZStd::string m_originNodeName;
                Quaternion m_rotation;
                Vector3 m_translation;
                float m_scale;
            };
        } // SceneData
    } // SceneAPI
} // AZ
