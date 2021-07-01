#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>
#include <SceneAPI/SceneCore/DataTypes/MatrixType.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IBoneData
                : public IGraphObject
            {
            public:
                AZ_RTTI(IBoneData, "{9DC2BC55-BF0A-4849-9367-2138340768DE}", IGraphObject);

                virtual ~IBoneData() override = default;

                void CloneAttributesFrom([[maybe_unused]] const IGraphObject* sourceObject) override {}

                virtual const MatrixType& GetWorldTransform() const = 0;

                void GetDebugOutput(AZ::SceneAPI::Utilities::DebugOutput& output) const override
                {
                    output.Write("WorldTransform", GetWorldTransform());
                }
            };
        } // DataTypes
    } // SceneAPI
} // AZ
