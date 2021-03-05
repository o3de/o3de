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

// AZ
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/smart_ptr/make_shared.h>

// Graph Model
#include <GraphModel/Model/Module/ModuleGraphManager.h>

// Landscape Canvas
#include <Editor/Core/GraphContext.h>
#include <Editor/Core/Core.h>

namespace LandscapeCanvas
{
    /*static*/ AZStd::shared_ptr<GraphContext> GraphContext::s_instance;

    /*static*/ void GraphContext::SetInstance(AZStd::shared_ptr<GraphContext> graphContext)
    {
        s_instance = graphContext;

        if (s_instance)
        {
            s_instance->Init();
        }
    }

    /*static*/ AZStd::shared_ptr<GraphContext> GraphContext::GetInstance()
    {
        return s_instance;
    }

    GraphContext::GraphContext()
    {
        // Construct our custom data types
        const AZ::EntityId invalidEntity;
        const AZStd::any defaultValue(invalidEntity);
        const AZStd::string cppName = "AZ::EntityId";
        m_dataTypes.push_back(AZStd::make_shared<LandscapeCanvasDataType>(LandscapeCanvasDataTypeEnum::InvalidEntity, InvalidEntityTypeId, defaultValue, "InvalidEntity", cppName));
        m_dataTypes.push_back(AZStd::make_shared<LandscapeCanvasDataType>(LandscapeCanvasDataTypeEnum::Bounds, BoundsTypeId, defaultValue, "Bounds", cppName));
        m_dataTypes.push_back(AZStd::make_shared<LandscapeCanvasDataType>(LandscapeCanvasDataTypeEnum::Gradient, GradientTypeId, defaultValue, "Gradient", cppName));
        m_dataTypes.push_back(AZStd::make_shared<LandscapeCanvasDataType>(LandscapeCanvasDataTypeEnum::Area, AreaTypeId, defaultValue, "Area", cppName));

        // Construct basic data types
        const AZ::Uuid stringTypeUuid = azrtti_typeid<AZStd::string>();
        m_dataTypes.push_back(AZStd::make_shared<LandscapeCanvasDataType>(LandscapeCanvasDataTypeEnum::String, stringTypeUuid, AZStd::any(AZStd::string("")), "String", "AZStd::string"));
    }

    void GraphContext::Init()
    {
        if (!m_moduleGraphManager)
        {
            m_moduleGraphManager = AZStd::make_shared<GraphModel::ModuleGraphManager>(shared_from_this());
        }
    }

    const char* GraphContext::GetSystemName() const
    {
        return SYSTEM_NAME;
    }

    const char* GraphContext::GetModuleFileExtension() const
    {
        return MODULE_FILE_EXTENSION;
    }

    GraphModel::DataTypePtr GraphContext::GetDataType(AZ::Uuid typeId) const
    {
        for (GraphModel::DataTypePtr dataType : m_dataTypes)
        {
            if (dataType->GetTypeUuid() == typeId)
            {
                return dataType;
            }
        }

        return AZStd::make_shared<LandscapeCanvasDataType>(); // LandscapeCanvasDataType is Invalid
    }

    GraphModel::DataTypePtr GraphContext::GetDataTypeForValue(const AZStd::any& value) const
    {
        // If the value is an AZ::EntityId::InvalidEntityId return our special
        // case node data type to handle the default values.
        const AZ::Uuid type = value.type();
        if (type == azrtti_typeid<AZ::EntityId>())
        {
            auto entityId = AZStd::any_cast<AZ::EntityId>(value);
            if (!entityId.IsValid())
            {
                return GetDataType(LandscapeCanvasDataTypeEnum::InvalidEntity);
            }
        }

        return GraphModel::IGraphContext::GetDataTypeForValue(value);
    }

    GraphModel::DataTypePtr GraphContext::GetDataType(GraphModel::DataType::Enum typeEnum) const
    {
        if (0 <= typeEnum && typeEnum < m_dataTypes.size())
        {
            return m_dataTypes[typeEnum];
        }
        else
        {
            return AZStd::make_shared<LandscapeCanvasDataType>(); // LandscapeCanvasDataType is Invalid
        }
    }

    const AZStd::vector<GraphModel::DataTypePtr>& GraphContext::GetAllDataTypes() const
    {
        return m_dataTypes;
    }

    GraphModel::ModuleGraphManagerPtr GraphContext::GetModuleGraphManager() const
    {
        return m_moduleGraphManager;
    }
} // namespace LandscapeCanvas
