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
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>

#include <AzCore/RTTI/RTTI.h>

#include <RCExt/CoordinateSystemConverter.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            class CoordinateSystemRule
                : public AZ::SceneAPI::DataTypes::IRule
            {
            public:
                AZ_RTTI(CoordinateSystemRule, "{603207E2-4F55-4C33-9AAB-98CA75C1E351}", IRule);
                AZ_CLASS_ALLOCATOR_DECL

                enum CoordinateSystem : int
                {
                    ZUpPositiveYForward = 0,
                    ZUpNegativeYForward = 1
                };

                CoordinateSystemRule();
                ~CoordinateSystemRule() override = default;

                void UpdateCoordinateSystemConverter();
                CoordinateSystem GetTargetCoordinateSystem() const;

                AZ_INLINE const CoordinateSystemConverter& GetCoordinateSystemConverter() const         { return m_coordinateSystemConverter; }

                static void Reflect(AZ::ReflectContext* context);

            protected:
                CoordinateSystemConverter   m_coordinateSystemConverter;
                CoordinateSystem            m_targetCoordinateSystem;
            };
        }  // Rule
    }  // Pipeline
}  // EMotionFX
