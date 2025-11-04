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
#include <AzCore/std/smart_ptr/enable_shared_from_this.h>
#include <AzCore/std/string/string.h>

// GraphModel
#include <GraphModel/Model/Common.h>
#include <GraphModel/Model/DataType.h>

namespace GraphModel
{
    //!!! Start in Graph.h for high level GraphModel documentation !!!

    //! GraphContext provides access to client specific information and systems required by the graphmodel framework.
    //! All supported data types used by graphs in the client system must be registered with the GraphContext.
    //! All systems that use GraphModel must provide an instance of this, or a derived, class, passed to the main Graph object.
    class GraphContext : public AZStd::enable_shared_from_this<GraphContext>
    {
    public:
        AZ_CLASS_ALLOCATOR(GraphContext, AZ::SystemAllocator);
        AZ_RTTI(GraphContext, "{4CD3C171-A7AA-4B62-96BB-F09F398A73E7}");
        static void Reflect(AZ::ReflectContext* context);

        GraphContext() = default;
        GraphContext(const AZStd::string& systemName, const AZStd::string& moduleExtension, const DataTypeList& dataTypes);
        virtual ~GraphContext() = default;

        //! Returns the name of the system that is using the GraphModel framework, mostly for debug messages.
        const char* GetSystemName() const;

        //! Returns the file extension used for module files
        const char* GetModuleFileExtension() const;

        //! Creates the module graph manager used by all module nodes in this context.
        //! This is done after construction because it is optional and the module graph manager needs a reference to the graph context
        void CreateModuleGraphManager();

        //! Returns a ModuleGraphManager to support creating ModuleNodes. Subclasses can just return nullptr if this isn't needed.
        ModuleGraphManagerPtr GetModuleGraphManager() const;

        //! Returns all available data types.
        const DataTypeList& GetAllDataTypes() const;

        //! Returns a DataType object representing the given TypeId, or Invalid if it doesn't exist.
        DataTypePtr GetDataType(DataType::Enum typeEnum) const;

        //! Returns a DataType object representing the given cpp or display name, or Invalid if it doesn't exist.
        DataTypePtr GetDataType(const AZStd::string& name) const;

        //! Returns a DataType object representing the given TypeId, or Invalid if it doesn't exist.
        DataTypePtr GetDataType(const AZ::Uuid& typeId) const;

        //! Utility function to returns a DataType object representing the given template type T, or Invalid if it doesn't exist.
        template<typename T>
        DataTypePtr GetDataType() const
        {
            return GetDataType(azrtti_typeid<T>());
        }

        //! Returns a DataType object representing the given AZStd::any value, or Invalid if it doesn't exist.
        //! This data type method has a different name because if the GraphContext implementation doesn't override
        //! this, there will be a compile error for a hidden function because of subclasses implementing
        //! the templated version below
        DataTypePtr GetDataTypeForValue(const AZStd::any& value) const;

    protected:
        AZStd::string m_systemName;
        AZStd::string m_moduleExtension;
        DataTypeList m_dataTypes;
        ModuleGraphManagerPtr m_moduleGraphManager;
    };

} // namespace GraphModel
