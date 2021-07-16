#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Vector3.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>
#include <SceneAPI/SceneCore/DataTypes/MatrixType.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IAnimationData
                : public IGraphObject
            {
            public:
                AZ_RTTI(IAnimationData, "{62B0571C-6EFF-42FA-902A-85AC744E04F2}", IGraphObject);

                virtual ~IAnimationData() override = default;

                void CloneAttributesFrom([[maybe_unused]] const IGraphObject* sourceObject) override {}

                virtual size_t GetKeyFrameCount() const = 0;
                virtual const MatrixType& GetKeyFrame(size_t index) const = 0;
                virtual double GetTimeStepBetweenFrames() const = 0;
            };

            class IBlendShapeAnimationData
                : public IGraphObject
            {
            public:
                AZ_RTTI(IBlendShapeAnimationData, "{CD2004EB-8B88-42B2-A539-079A557C98C9}", IGraphObject);

                virtual ~IBlendShapeAnimationData() override = default;

                void CloneAttributesFrom([[maybe_unused]] const IGraphObject* sourceObject) override {}

                virtual const char* GetBlendShapeName() const = 0;
                virtual size_t GetKeyFrameCount() const = 0;
                virtual double GetKeyFrame(size_t index) const = 0;
                virtual double GetTimeStepBetweenFrames() const = 0;
            };
        }
    }
}
