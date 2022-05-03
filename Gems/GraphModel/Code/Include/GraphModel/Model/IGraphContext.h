/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// AZ
#include <AzCore/std/containers/vector.h>

// GraphModel
#include <GraphModel/Model/DataType.h>

namespace GraphModel
{

    //!!! Start in Graph.h for high level GraphModel documentation !!!

    //! IGraphContext provides an interface to client system specific features for the GraphModel framework to use.
    //! All systems that use GraphModel must provide an implementation of this interface, passed to the main Graph object.
    class IGraphContext
    {
    public:
        using DataTypeList = AZStd::vector<DataTypePtr>;
        
        virtual ~IGraphContext() = default;

        //! Returns the name of the system that is using the GraphModel framework, mostly for debug messages.
        virtual const char* GetSystemName() const = 0;

        //! Returns the file extension used for module files
        virtual const char* GetModuleFileExtension() const = 0;

        //! Returns a ModuleGraphManager to support creating ModuleNodes. Subclasses can just return nullptr if this isn't needed.
        virtual ModuleGraphManagerPtr GetModuleGraphManager() const = 0;

        //! Returns all available data types.
        virtual const DataTypeList& GetAllDataTypes() const = 0;

        //! Returns a DataType object representing the given TypeId, or Invalid if it doesn't exist.
        virtual DataTypePtr GetDataType(AZ::Uuid typeId) const = 0;

        //! Returns a DataType object representing the given AZStd::any value, or Invalid if it doesn't exist.
        //! This data type method has a different name because if the GraphContext implementation doesn't override
        //! this, there will be a compile error for a hidden function because of subclasses implementing
        //! the templated version below
        virtual DataTypePtr GetDataTypeForValue(const AZStd::any& value) const { return GetDataType(value.type()); }

        //! Returns a DataType object representing the given TypeId, or Invalid if it doesn't exist.
        virtual DataTypePtr GetDataType(DataType::Enum typeEnum) const = 0;

        //! Utility function to returns a DataType object representing the given template type T, or Invalid if it doesn't exist.
        template<typename T>
        DataTypePtr GetDataType() const { return GetDataType(azrtti_typeid<T>()); }
    };
    
} // namespace GraphModel




