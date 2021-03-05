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
#pragma once

/*
*
* This file contains the AzCodeGenerator tag definitions for Script Canvas, it also provides the necessary 
* documentation for the usage of the different code generation tags and the intellisense helpers for
* supported tags.
*
* CodeGen in Script Canvas consists of two parts, the first part is the header, nodes that use any of the
* code generation tags in this file need to include the generated header file which will have the 
* format: <filename>.generated.h
*
* The generated header file is used to perform code injection into the class definition, see: ScriptCanvas_Node
*
* Example:
*     #include <Libraries/Core/Print.generated.h>
*
* The .cpp file will also need to include the generated code. This file will contain the generated serialization
* and reflection code for any of the tags specified in the class declaration.
*
* Example:
*     #include <Libraries/Core/Print.generated.cpp>
*
*/

#include <AzCore/Preprocessor/CodeGen.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Core/Contract.h>
#include <ScriptCanvas/Core/SlotConfigurations.h>

#if !defined(AZ_CODE_GENERATOR)

/* ----------------------------------------------------------------------------------------------------------
*
* ScriptCanvas_Node
* This tag must be included within the body of any custom Script Canvas node.It generates the necessary code to support nodes
* and customizes the serialization and reflection parameters(version, converter).
*
* Supports:
* ScriptCanvas_Node::Uuid             // REQUIRED Uuid for this node's type, it is used by the AZ_COMPONENT macro internally.
* ScriptCanvas_Node::Description      // The friendly description to display in the editor (the name will be the name of the class)
* ScriptCanvas_Node::Icon             // Attribute used by the EditContext to provide a path to an icon for the node.
* ScriptCanvas_Node::Version          // The version of the node, this ensure data serialization versioning is supported
*                                     // Optionally, the version tag supports a version converter, this can be used by 
*                                     // specifying the unqualified function name as the second argument of the Version tag.
* ScriptCanvas_Node::GraphEntryPoint  // Some nodes need to execute as soon as the graph is activated (i.e. the Start node).
*
* Example:
*     ScriptCanvas_Node(Print, ScriptCanvas_Node::Uuid("{085CBDD3-D4E0-44D4-BF68-8732E35B9DF1}")
*                              ScriptCanvas_Node::Description("Prints a string")
*                              ScriptCanvas_Node::Icon("Editor/Icons/ScriptCanvas/Print.png")
*                              ScriptCanvas_Node::Version(3, VersionConverter));
*
* ----------------------------------------------------------------------------------------------------------- */
#define ScriptCanvas_Node(ClassName, ...) AZ_JOIN(AZ_GENERATED_, ClassName)

/* ----------------------------------------------------------------------------------------------------------
*
* ScriptCanvas_In
* This tag provides a named "Input" execution slot to the node.
*
* Supports:
*     ScriptCanvas_In::Name             // The friendly name and description to display in the editor
*
* Example:
*     ScriptCanvas_In(ScriptCanvas_In::Name("Start Process", "Signals this node to begin processing."));
*
* ----------------------------------------------------------------------------------------------------------- */
#define ScriptCanvas_In(...)

/* -----------------------------------------------------------------------------------------------------------
*
* ScriptCanvas_Out
* This tag provides a named "Output" execution slot to the node.
*
* Supports:
*     ScriptCanvas_Out::Name             // The friendly name and description to display in the editor
*
* Example:
*     ScriptCanvas_Out(ScriptCanvas_Out::Name("On Finished", "Will be signaled when the operation is complete."));
*
* ---------------------------------------------------------------------------------------------------------- */
#define ScriptCanvas_Out(...) 

/* -----------------------------------------------------------------------------------------------------------
*
* ScriptCanvas_OutLatent
* This tag provides a named latent out execution slot to the node.
*
* Supports:
*     ScriptCanvas_OutLatent::Name             // The friendly name and description to display in the editor
*
* Example:
*     ScriptCanvas_OutLatent(ScriptCanvas_OutLatent::Name("On Finished", "Will be signaled when the operation is complete."));
*
* ---------------------------------------------------------------------------------------------------------- */
#define ScriptCanvas_OutLatent(...) 

/*
*----------------------------------------------------------------------------------------------------------
*
* ScriptCanvas_Property
* This tag must precede a member variable in the class that you wish to expose to Script Canvas for editing
* and scripting. By default the property will be exposed with an input and output slot, but it can be customized
* to only expose one or the other through the Input/Output attributes.
*
* Usage:
*
* ScriptCanvas_Property(<property type>, <codegen attributes>);
*
* Supports:
*     ScriptCanvas_Property::Name             // The friendly name and description to display in the editor
*     ScriptCanvas_Property::Input            // Exposes this property as an INPUT slot on the node.
*     ScriptCanvas_Property::Output           // Exposes this property as an OUTPUT slot on the node.
*     ScriptCanvas_Property::Transient        // Property will not be reflected for serialization, edit or behavior, these properties are useful when the value can only be provided by an connected node.
*
* Examples:
*
*     ScriptCanvas_Property(AZStd::string, ScriptCanvas_Property::Name("Text", "A string of characters"))
*
*     ScriptCanvas_Property(bool, ScriptCanvas_Property::Name("Is Ready", "Returns true if the entity is ready.")  ScriptCanvas_Property::Output)
*
*     ScriptCanvas_Property(bool, ScriptCanvas_Property::Name("Enable", "Set whether this functionality is enabled.") ScriptCanvas_Property::Input)
*
* ---------------------------------------------------------------------------------------------------------- */
#define ScriptCanvas_Property(...)

/*
*----------------------------------------------------------------------------------------------------------
*
* ScriptCanvas_DynamicDataSlot
*
* This tag must precede a definition of a Dynamically Typed slot.
*
* Usage:
*
* ScriptCanvas_DynamicDataSlot(<Dynamic PropertyType>, <ConnectionType>, <codegen attributes>);
* Supports:
*     ScriptCanvas_Property::Name             // The friendly name and description to display in the editor
*     ScriptCanvas_Property::DynamicGroup     // A way of grouping multiple dynamically typed slots to all have the same type.
*
*
* Examples:
*
*     ScriptCanvas_DynamicProperty(ScriptCanvas::DynamicDataType::Value, ScriptCanvas::ConnectionType::Input, ScriptCanvas_Property::Name("Value", "A generic value"))*
*     ScriptCanvas_DynamicProperty(ScriptCanvas::DynamicDataType::Any, ScriptCanvas::ConnectionType::Output, ScriptCanvas_Property::Name("Any", "A generic any"))
*     ScriptCanvas_DynamicProperty(ScriptCanvas::DynamicDataType::Container, ScriptCanvas_Property::Input, ScriptCanvas_Property::Name("Container", "A Generic Container Input"))*
*/
#define ScriptCanvas_DynamicDataSlot(DynamicDataType, ConnectionType, ...)

/*
*----------------------------------------------------------------------------------------------------------
*
* Property
* This tag provides a mechanism to reflect a property to the serialization context that does not
* need to be an editable or an input property.
*
* Examples:
* 
*     Property(bool, m_autoConnect);
*
*
* ---------------------------------------------------------------------------------------------------------- */
#define ScriptCanvas_SerializeProperty(Type, Name, ...) Type Name;

/*
*----------------------------------------------------------------------------------------------------------
*
* Property
* This is the same as ScriptCanvas_SerializeProperty, but allows a user to provide a default value for the property
*
* Examples:
*
*     ScriptCanvas_SerializePropertyWithDefaults(bool, m_autoConnect, false);
*
*
* ---------------------------------------------------------------------------------------------------------- */
#define ScriptCanvas_SerializePropertyWithDefaults(Type, Name, DefaultVal, ...) Type Name{ DefaultVal };

/*
*----------------------------------------------------------------------------------------------------------
*
* EditProperty
* This tag provides a mechanism to reflect a property to the serialization context and reflect it to
* the EditContext with EditContext attribute support.
*
* Examples:
* 
*     EditProperty(bool, m_autoConnect, EditProperty::Name("Auto Connect", "When true it will auto connect to the graph entity."));
*
* ---------------------------------------------------------------------------------------------------------- */
#define ScriptCanvas_EditProperty(Type, Name, ...) Type Name;

/*
*----------------------------------------------------------------------------------------------------------
*
* EditProperty
* This is the same tag as ScriptCanvas_EditProperty, but allows the user to provide a default value to the property
*
* Examples:
*
*     ScriptCanvas_EditPropertyWithDefaults(bool, m_autoConnect, false EditProperty::Name("Auto Connect", "When true it will auto connect to the graph entity."));
*
* ---------------------------------------------------------------------------------------------------------- */
#define ScriptCanvas_EditPropertyWithDefaults(Type, Name, DefaultVal, ...) Type Name{ DefaultVal };

/*
*----------------------------------------------------------------------------------------------------------
*
* ScriptCanvas_PropertyWithDefaults
* This is the same tag as ScriptCanvas_Property but it gives the user the ability to provide and override
* the default value for the property.
*
* Example:
*
*     ScriptCanvas_PropertyWithDefaults(AZ::Vector2, AZ::Vector2(20.f, 20.f),
*     ScriptCanvas_Property::Name("Position", "Position of the text on-screen in normalized coordinates [0-1].")
*     ScriptCanvas_Property::Input);
*
* ---------------------------------------------------------------------------------------------------------- */
#define ScriptCanvas_PropertyWithDefaults(...)

/*
*----------------------------------------------------------------------------------------------------------
*
* ScriptCanvas_Include
*
* When it is necessary for the generated file to contain a specific include.
* Note: The include directive will use the brackets syntax. (e.g. ScriptCanvas_Include("AzCore/Script/ScriptTimePoint.h")
* will produce:
* #include <AzCore/Script/ScriptTimePoint.h>
* in the generated.h file
* ---------------------------------------------------------------------------------------------------------- */
#define ScriptCanvas_Include(IncludeDeclaration, ...)

#else

// These are the tag definitions as seen by AzCodeGenerator during code traversal, they will produce the necessary 
// data for the code generation templates to produce code. See: ScriptCanvas/CodeGen/Drivers

#define ScriptCanvas_Node(ClassName, ...) AZCG_CreateArgumentAnnotation(ScriptCanvas_Class, Class_Attribute, Identifier(ClassName), __VA_ARGS__) int AZ_JOIN(m_azCodeGenInternal, __COUNTER__);

#define ScriptCanvas_In(...) AZCG_CreateArgumentAnnotation(ScriptCanvas_In, Identifier(In), __VA_ARGS__) int AZ_JOIN(m_azCodeGenInternal, __COUNTER__);
#define ScriptCanvas_Out(...) AZCG_CreateArgumentAnnotation(ScriptCanvas_Out, Identifier(Out), __VA_ARGS__) int AZ_JOIN(m_azCodeGenInternal, __COUNTER__);
#define ScriptCanvas_OutLatent(...) AZCG_CreateArgumentAnnotation(ScriptCanvas_OutLatent, Identifier(Out), __VA_ARGS__) int AZ_JOIN(m_azCodeGenInternal, __COUNTER__);

#define ScriptCanvas_Property(Type, ...) AZCG_CreateArgumentAnnotation(ScriptCanvas_Property, Identifier(Property), ValueType(Type), __VA_ARGS__) Type AZ_JOIN(m_azCodeGenInternal, __COUNTER__);
#define ScriptCanvas_PropertyWithDefaults(Type, Default, ...) AZCG_CreateArgumentAnnotation(ScriptCanvas_Property, Identifier(Property), ValueType(Type), DefaultValue(AZ_STRINGIZE(Default)), __VA_ARGS__) Type AZ_JOIN(m_azCodeGenInternal, __COUNTER__);

#define ScriptCanvas_DynamicDataSlot(DynamicDataType, ConnectionDirectionType, ...) AZCG_CreateArgumentAnnotation(ScriptCanvas_DynamicDataSlot, Identifier(DynamicDataSlot), DynamicType(DynamicDataType), ConnectionType(ConnectionDirectionType), __VA_ARGS__) Type AZ_JOIN(m_azCodeGenlInternal, __COUNTER__);

#define ScriptCanvas_SerializeProperty(Type, Name, ...) AZCG_CreateArgumentAnnotation(ScriptCanvas_SerializeProperty, SerializedProperty, __VA_ARGS__) Type Name;
#define ScriptCanvas_EditProperty(Type, Name, ...) AZCG_CreateArgumentAnnotation(ScriptCanvas_SerializeProperty, SerializedProperty, EditProperty, __VA_ARGS__) Type Name;
#define ScriptCanvas_SerializePropertyWithDefaults(Type, Name, DefaultVal, ...) AZCG_CreateArgumentAnnotation(ScriptCanvas_SerializeProperty, SerializedProperty, __VA_ARGS__) Type Name{ DefaultVal };
#define ScriptCanvas_EditPropertyWithDefaults(Type, Name, DefaultVal, ...) AZCG_CreateArgumentAnnotation(ScriptCanvas_SerializeProperty, SerializedProperty, EditProperty, __VA_ARGS__) Type Name{ DefaultVal };

#define ScriptCanvas_Include(IncludeDeclaration, ...) AZCG_CreateArgumentAnnotation(ScriptCanvas_Includes, Identifier(IncludeDeclaration), __VA_ARGS__) int AZ_JOIN(m_azCodeGenInternal, __COUNTER__);

#endif

// Intellisense helpers, the following definitions exist to provide code completion details regarding what attributes are
// supported by the different tags.

// Note: It's important to verify these definitions exist within your templates:
// {% if class.annotations.ScriptCanvas_Node is defined %}

// Tags that might be common to multiple contexts, should be aliased into those namespaces and not used directly
namespace ScriptCanvasTags
{
    struct Name
    {
        Name([[maybe_unused]] const char* name, [[maybe_unused]] const char* description) {}
    };

    struct Description
    {
        Description([[maybe_unused]] const char* description) {}
    };

    struct Uuid
    {
        Uuid([[maybe_unused]] const char* uuid) {}
    };

    struct Category
    {
        Category([[maybe_unused]] const char* category) {}
    };

    struct DisplayGroup
    {
        DisplayGroup([[maybe_unused]] const char* displayGroup) {}
    };

    struct Icon
    {
        Icon([[maybe_unused]] const char* icon) {}
    };

    struct Version
    {
        using ConverterFunction = bool(class AZ::SerializeContext& context, class AZ::SerializeContext::DataElementNode& classElement);
        Version([[maybe_unused]] unsigned int version) {}
        Version([[maybe_unused]] unsigned int version, [[maybe_unused]] ConverterFunction converter) {}
    };

    template <class EventHandlerType>
    struct EventHandler
    {
        EventHandler() = default;
    };

    namespace Edit
    {
        struct UIHandler
        {
            UIHandler([[maybe_unused]] const AZ::Crc32& uiHandler = AZ::Edit::UIHandlers::Default) {}
        };
    }

    struct EditAttributes
    {
        template <typename ...Args>
        EditAttributes(Args&&... args) {}
    };

    struct BaseClass
    {
        BaseClass(AZStd::initializer_list<const char*>) {}
    };

    // Provided a hook for reflecting dependent classes from a node.
    struct DependentReflections
    {
        DependentReflections(AZStd::initializer_list<const char*>) {}
    };

    struct Deprecated
    {
        Deprecated([[maybe_unused]] const char* details) {}
    };

    struct Contracts
    {
        explicit Contracts([[maybe_unused]] AZStd::initializer_list<ScriptCanvas::Contract> contracts) {}
    };

    struct RestrictedTypeContractTag
    {
        explicit RestrictedTypeContractTag([[maybe_unused]] AZStd::initializer_list<ScriptCanvas::Data::Type> restrictedType) {}
    };

    struct SupportsMethodContractTag
    {
        explicit SupportsMethodContractTag([[maybe_unused]] const char* methodName) {}
    };    
}

namespace ScriptCanvas_Node
{
    using ScriptCanvasTags::Name;
    using ScriptCanvasTags::Description;
    using ScriptCanvasTags::Uuid;
    using ScriptCanvasTags::Icon;
    using ScriptCanvasTags::Version;
    using ScriptCanvasTags::EventHandler;
    using ScriptCanvasTags::EditAttributes;
    using ScriptCanvasTags::Category;
    using ScriptCanvasTags::Deprecated;
    using ScriptCanvasTags::DependentReflections;
    
    struct GraphEntryPoint
    {
        GraphEntryPoint(bool) {}
    };

    // Signals whether or not the ordering of dynamically added slots on the node will change during edit time.
    //
    // Main use cases are for user input that will add/remove slots where order is desired to be maintained
    struct DynamicSlotOrdering
    {
        DynamicSlotOrdering(bool) {}
    };

}

namespace ScriptCanvas_In
{
    using ScriptCanvasTags::Name;
    using ScriptCanvasTags::DisplayGroup;
    using ScriptCanvasTags::Contracts;    
}

namespace ScriptCanvas_Out
{
    using ScriptCanvasTags::Name;
    using ScriptCanvasTags::DisplayGroup;
}

namespace ScriptCanvas_OutLatent
{
    using ScriptCanvasTags::Name;
    using ScriptCanvasTags::DisplayGroup;
}

namespace ScriptCanvas_Property
{
    using ScriptCanvasTags::Name;
    using ScriptCanvasTags::DisplayGroup;

    using ScriptCanvasTags::Edit::UIHandler;

    using AzCommon::Attributes::ChangeNotify;
    using AzCommon::Attributes::Visibility;
    using AzCommon::Attributes::AutoExpand;
    using AzCommon::Attributes::DescriptionTextOverride;
    using AzCommon::Attributes::NameLabelOverride;
    using AzCommon::Attributes::Min;
    using AzCommon::Attributes::Max;
    
    // Will produce an untyped input slot.
    using Overloaded = bool;

    // Exposes this property as an INPUT slot on the node.
    using Input = bool;

    // Exposes this property as an OUTPUT slot on the node.
    using Output = bool;

    // Transient properties will not be reflected for serialization, edit or behavior, these are properties whose 
    // value will be provided by a connected node.
    using Transient = bool;

    using OutputStorageSpec = bool;
}

namespace ScriptCanvas_DynamicDataSlot
{
    using ScriptCanvasTags::Name;
    using ScriptCanvasTags::DisplayGroup;

    using ScriptCanvasTags::Contracts;
    using ScriptCanvasTags::RestrictedTypeContractTag;
    using ScriptCanvasTags::SupportsMethodContractTag;

    using DynamicGroup = AZStd::string;
}

namespace EditProperty
{
    using ScriptCanvasTags::Name;

    using ScriptCanvasTags::Category;
    using ScriptCanvasTags::EditAttributes;    

    using AzCommon::Attributes::ChangeNotify;
    using AzCommon::Attributes::Visibility;
    using AzCommon::Attributes::AutoExpand;
    using AzCommon::Attributes::DescriptionTextOverride;
    using AzCommon::Attributes::NameLabelOverride;
    using AzCommon::Attributes::Min;
    using AzCommon::Attributes::Max;
}
