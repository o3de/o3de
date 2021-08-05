/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// AZ
#include <AzCore/std/smart_ptr/enable_shared_from_this.h>

// GraphModel
#include <GraphModel/Model/IGraphContext.h>

// Landscape Canvas
#include <Editor/Core/DataTypes.h>

namespace LandscapeCanvas
{
    class GraphContext : public GraphModel::IGraphContext, public AZStd::enable_shared_from_this<GraphContext>
    {
    public:
        static void SetInstance(AZStd::shared_ptr<GraphContext> graphContext);
        static AZStd::shared_ptr<GraphContext> GetInstance();

        GraphContext();
        virtual ~GraphContext() = default;

        const char* GetSystemName() const override;
        const char* GetModuleFileExtension() const override;
        const DataTypeList& GetAllDataTypes() const override;
        GraphModel::DataTypePtr GetDataType(AZ::Uuid typeId) const override;
        GraphModel::DataTypePtr GetDataTypeForValue(const AZStd::any& value) const override;
        GraphModel::DataTypePtr GetDataType(GraphModel::DataType::Enum typeEnum) const override;

        template<typename T>
        GraphModel::DataTypePtr GetDataType() const { return IGraphContext::GetDataType<T>(); }

        GraphModel::ModuleGraphManagerPtr GetModuleGraphManager() const override;

    private:
        //! Performs initialization that can't be done in the constructor
        void Init();

        static AZStd::shared_ptr<GraphContext> s_instance;

        DataTypeList m_dataTypes;
        GraphModel::ModuleGraphManagerPtr m_moduleGraphManager;
    };
} // namespace LandscapeCanvas
