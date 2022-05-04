/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/std/smart_ptr/make_shared.h>

// Graph Model
#include <GraphModel/Model/Graph.h>
#include <GraphModel/Model/GraphContext.h>
#include <GraphModel/Model/Module/ModuleGraphManager.h>

namespace GraphModel
{
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

    const AZStd::vector<DataTypePtr>& GraphContext::GetAllDataTypes() const
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

    DataTypePtr GraphContext::GetDataType(AZ::Uuid typeId) const
    {
        for (DataTypePtr dataType : m_dataTypes)
        {
            if (dataType->GetTypeUuid() == typeId)
            {
                return dataType;
            }
        }

        return {};
    }

    DataTypePtr GraphContext::GetDataTypeForValue(const AZStd::any& value) const
    {
        return GetDataType(value.type());
    }
} // namespace GraphModel
