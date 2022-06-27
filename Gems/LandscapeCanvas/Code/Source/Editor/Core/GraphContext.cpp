/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            s_instance->CreateModuleGraphManager();
        }
    }

    /*static*/ AZStd::shared_ptr<GraphContext> GraphContext::GetInstance()
    {
        return s_instance;
    }

    GraphContext::GraphContext()
        : GraphModel::GraphContext(SYSTEM_NAME, MODULE_FILE_EXTENSION, {})
    {
        // Construct our custom data types
        const AZ::EntityId invalidEntity;
        const AZStd::any defaultValue(invalidEntity);
        const AZStd::string cppName = "AZ::EntityId";
        auto validEntityIdValidator = [](const AZStd::any& value)
        {
            return value.is<AZ::EntityId>() && AZStd::any_cast<AZ::EntityId>(value).IsValid();
        };
        auto invalidEntityIdValidator = [](const AZStd::any& value)
        {
            return value.empty() || (value.is<AZ::EntityId>() && !AZStd::any_cast<AZ::EntityId>(value).IsValid());
        };

        m_dataTypes.push_back(AZStd::make_shared<GraphModel::DataType>(LandscapeCanvasDataTypeEnum::InvalidEntity, InvalidEntityTypeId, defaultValue, "InvalidEntity", cppName, invalidEntityIdValidator));
        m_dataTypes.push_back(AZStd::make_shared<GraphModel::DataType>(LandscapeCanvasDataTypeEnum::Bounds, BoundsTypeId, defaultValue, "Bounds", cppName, validEntityIdValidator));
        m_dataTypes.push_back(AZStd::make_shared<GraphModel::DataType>(LandscapeCanvasDataTypeEnum::Gradient, GradientTypeId, defaultValue, "Gradient", cppName, validEntityIdValidator));
        m_dataTypes.push_back(AZStd::make_shared<GraphModel::DataType>(LandscapeCanvasDataTypeEnum::Area, AreaTypeId, defaultValue, "Area", cppName, validEntityIdValidator));

        // Construct basic data types
        const AZ::Uuid stringTypeUuid = azrtti_typeid<AZStd::string>();
        m_dataTypes.push_back(AZStd::make_shared<GraphModel::DataType>(LandscapeCanvasDataTypeEnum::String, stringTypeUuid, AZStd::any(AZStd::string("")), "String", "AZStd::string"));
    }
} // namespace LandscapeCanvas
