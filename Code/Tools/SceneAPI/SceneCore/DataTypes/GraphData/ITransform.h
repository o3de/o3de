#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>
#include <SceneAPI/SceneCore/DataTypes/MatrixType.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class ITransform
                : public IGraphObject
            {
            public:
                AZ_RTTI(ITransform, "{C1A24A14-EDD4-422F-AB62-9566D744AF1B}", IGraphObject);

                virtual ~ITransform() override = default;

                void CloneAttributesFrom([[maybe_unused]] const IGraphObject* sourceObject) override {}

                virtual MatrixType& GetMatrix() = 0;
                virtual const MatrixType& GetMatrix() const = 0;
                void GetDebugOutput(AZ::SceneAPI::Utilities::DebugOutput& output) const override
                {
                    output.Write("Matrix", GetMatrix());
                }
            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ
