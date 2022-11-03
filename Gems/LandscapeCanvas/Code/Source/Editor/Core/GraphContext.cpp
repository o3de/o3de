/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzCore/IO/Path/Path.h>
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
        const AZStd::string cppName = "AZ::EntityId";
        m_dataTypes.push_back(AZStd::make_shared<GraphModel::DataType>(LandscapeCanvasDataTypeEnum::Bounds, BoundsTypeId, AZStd::any(AZ::EntityId()), "Bounds", cppName));
        m_dataTypes.push_back(AZStd::make_shared<GraphModel::DataType>(LandscapeCanvasDataTypeEnum::Gradient, GradientTypeId, AZStd::any(AZ::EntityId()), "Gradient", cppName));
        m_dataTypes.push_back(AZStd::make_shared<GraphModel::DataType>(LandscapeCanvasDataTypeEnum::Area, AreaTypeId, AZStd::any(AZ::EntityId()), "Area", cppName));
        m_dataTypes.push_back(AZStd::make_shared<GraphModel::DataType>(LandscapeCanvasDataTypeEnum::Path, PathTypeId, AZStd::any(AZ::IO::Path()), "Path", "AZ::IO::Path"));

        // Construct basic data types
        const AZ::Uuid stringTypeUuid = azrtti_typeid<AZStd::string>();
        m_dataTypes.push_back(AZStd::make_shared<GraphModel::DataType>(LandscapeCanvasDataTypeEnum::String, stringTypeUuid, AZStd::any(AZStd::string("")), "String", "AZStd::string"));
    }
} // namespace LandscapeCanvas
