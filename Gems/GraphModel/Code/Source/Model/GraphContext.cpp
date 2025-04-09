/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/std/smart_ptr/make_shared.h>

// Graph Model
#include <GraphModel/Model/Graph.h>
#include <GraphModel/Model/GraphContext.h>
#include <GraphModel/Model/Module/ModuleGraphManager.h>

namespace GraphModel
{
    void GraphContext::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GraphContext>()
                ->Version(0)
                ;

            serializeContext->RegisterGenericType<GraphContextPtr>();
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<GraphContext>("GraphModelGraphContext")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "editor.graph")
                ->Method("GetSystemName", &GraphContext::GetSystemName)
                ->Method("GetModuleFileExtension", &GraphContext::GetModuleFileExtension)
                //->Method("GetAllDataTypes", &GraphContext::GetAllDataTypes) // GHI-15121 Unbinding until asserts with AP behavior context reflection are resolved
                ->Method("GetDataTypeByEnum", static_cast<DataTypePtr (GraphContext::*)(DataType::Enum) const>(&GraphContext::GetDataType))
                ->Method("GetDataTypeByName", static_cast<DataTypePtr (GraphContext::*)(const AZStd::string&) const>(&GraphContext::GetDataType))
                ->Method("GetDataTypeByUuid", static_cast<DataTypePtr (GraphContext::*)(const AZ::Uuid&) const>(&GraphContext::GetDataType))
                ->Method("GetDataTypeForValue", &GraphContext::GetDataTypeForValue)
                ;
        }
    }

    GraphContext::GraphContext(const AZStd::string& systemName, const AZStd::string& moduleExtension, const DataTypeList& dataTypes)
        : m_systemName(systemName)
        , m_moduleExtension(moduleExtension)
        , m_dataTypes(dataTypes)
    {
    }

    const char* GraphContext::GetSystemName() const
    {
        return m_systemName.c_str();
    }

    const char* GraphContext::GetModuleFileExtension() const
    {
        return m_moduleExtension.c_str();
    }

    void GraphContext::CreateModuleGraphManager()
    {
        if (!m_moduleGraphManager)
        {
            m_moduleGraphManager = AZStd::make_shared<ModuleGraphManager>(shared_from_this());
        }
    }

    ModuleGraphManagerPtr GraphContext::GetModuleGraphManager() const
    {
        return m_moduleGraphManager;
    }

    const DataTypeList& GraphContext::GetAllDataTypes() const
    {
        return m_dataTypes;
    }

    DataTypePtr GraphContext::GetDataType(DataType::Enum typeEnum) const
    {
        for (DataTypePtr dataType : m_dataTypes)
        {
            if (dataType->GetTypeEnum() == typeEnum)
            {
                return dataType;
            }
        }

        return {};
    }

    DataTypePtr GraphContext::GetDataType(const AZStd::string& name) const
    {
        for (DataTypePtr dataType : m_dataTypes)
        {
            if (AZ::StringFunc::Equal(dataType->GetCppName(), name) || AZ::StringFunc::Equal(dataType->GetDisplayName(), name))
            {
                return dataType;
            }
        }

        return {};
    }

    DataTypePtr GraphContext::GetDataType(const AZ::Uuid& typeId) const
    {
        for (DataTypePtr dataType : m_dataTypes)
        {
            if (dataType->IsSupportedType(typeId))
            {
                return dataType;
            }
        }

        return {};
    }

    DataTypePtr GraphContext::GetDataTypeForValue(const AZStd::any& value) const
    {
        for (DataTypePtr dataType : m_dataTypes)
        {
            if (dataType->IsSupportedValue(value))
            {
                return dataType;
            }
        }

        return {};
    }
} // namespace GraphModel
