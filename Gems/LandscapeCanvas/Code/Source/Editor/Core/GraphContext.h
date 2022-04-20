/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// GraphModel
#include <GraphModel/Model/GraphContext.h>

// Landscape Canvas
#include <Editor/Core/DataTypes.h>

namespace LandscapeCanvas
{
    class GraphContext : public GraphModel::GraphContext
    {
    public:
        static void SetInstance(AZStd::shared_ptr<GraphContext> graphContext);
        static AZStd::shared_ptr<GraphContext> GetInstance();

        GraphContext();
        virtual ~GraphContext() = default;

        //! Overridden for custom handling of invalid entity IDs
        GraphModel::DataTypePtr GetDataTypeForValue(const AZStd::any& value) const override;

    private:
        static AZStd::shared_ptr<GraphContext> s_instance;
    };
} // namespace LandscapeCanvas
