/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>

namespace AZ
{
    class Vector4;
}

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            enum class TangentSpace
            {
                FromSourceScene = 0,
                MikkT           = 1,
                EMotionFX       = 2
            };

            enum class BitangentMethod
            {
                UseFromTangentSpace = 0,
                Orthogonal          = 1
            };

            class IMeshVertexTangentData
                : public IGraphObject
            {
            public:
                AZ_RTTI(IMeshVertexTangentData, "{B24084FF-09B1-4EE5-BA5B-2D392E92ECC1}", IGraphObject);

                virtual ~IMeshVertexTangentData() override = default;

                void CloneAttributesFrom([[maybe_unused]] const IGraphObject* sourceObject) override {}

                virtual size_t GetCount() const = 0;
                virtual const AZ::Vector4& GetTangent(size_t index) const = 0;
                virtual void SetTangent(size_t vertexIndex, const AZ::Vector4& tangent) = 0;
                virtual void SetTangentSetIndex(size_t setIndex) = 0;
                virtual size_t GetTangentSetIndex() const = 0;
                virtual TangentSpace GetTangentSpace() const = 0;
                virtual void SetTangentSpace(TangentSpace space) = 0;
            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ
