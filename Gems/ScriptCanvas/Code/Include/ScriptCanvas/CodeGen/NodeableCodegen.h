/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Preprocessor/CodeGen.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#define SCRIPTCANVAS_NODE(ClassName) SCRIPTCANVAS_NODE_##ClassName

/* ----------------------------------------------------------------------------------------------------------
*
* BaseDefinition
* This tag must be included within the body of any custom nodeable class. It generates nodeable code only and it
* should be used as a base class only.
*
* Note: This tag does not generate a node class, so it will be hidden during edit time.
*
* Example:
*   BaseDefinition(BaseHelloWorld, "Base Hello World", "My BaseHelloWorld.")
*
* ----------------------------------------------------------------------------------------------------------- */
#define BaseDefinition(ClassName, Name, Description, ...) AZ_JOIN(AZ_GENERATED_, ClassName)

/* ----------------------------------------------------------------------------------------------------------
*
* NodeDefinition
* This tag must be included within the body of any custom nodeable class. It generates the necessary code to support nodes
* and customizes the serialization and reflection parameters(version, converter).
*
* Example:
*   NodeDefinition(HelloWorld, "Hello World", "My HelloWorld Node.")
*   NodeDefinition(HelloWorld, "Hello World", "My HelloWorld Node.",
*                  NodeTags::Icon("Icons/ScriptCanvas/HelloWorld.png")
*                  NodeTags::Version(3, VersionConverter))
*
* ----------------------------------------------------------------------------------------------------------- */
#define NodeDefinition(ClassName, Name, Description, ...) AZ_JOIN(AZ_GENERATED_, ClassName)

/*  ----------------------------------------------------------------------------------------------------------
* InputMethod
* Using InputMethod on a method will create execution in&out slots that is invoked
* automatically. It will also allow the automatic generation of input or output data
* slots according to the method's signature.
*
* Example
*   InputMethod("Do Something", "My DoSomething Function.")
*   InputMethod("Do Something", "My DoSomething Function.")
*   DataInput(int, "DoSomething:Arg", 0, "My DoSomething argument.")
*
* ----------------------------------------------------------------------------------------------------------- */
#define InputMethod(Name, Description, ...)

/*  ----------------------------------------------------------------------------------------------------------
* BranchMethod
* Using BranchMethod on a method will create execution input&output slots that is invoked
* automatically. It will also allow the automatic generation of input data
* slots according to the method's signature. BranchMethod should not be used on method
* having return type.
*
* Coupled with macro ExecutionOutput to generate branch out execution slots.
*
* Example
*   BranchMethod("Branches", "My Branches Function.")
*   ExecutionOutput("Branch1", "My Branch1 Function.", SlotTags::BranchOf("Branches"))
*   ExecutionOutput("Branch2", "My Branch2 Function.", SlotTags::BranchOf("Branches"))
*
* ----------------------------------------------------------------------------------------------------------- */
#define BranchMethod(Name, Description, ...)

/*  ----------------------------------------------------------------------------------------------------------
* OnInputChangeMethod
* Using OnInputChangeMethod on a method will create one data input slot that is invoked
* automatically, and it should be used only with one input method.
*
* Example
*   OnInputChangeMethod("MyInputChangeMethod", "My OnInputChange Function.")
*   DataInput(int, "MyInputChangeMethod:Arg", 0, "My MyInputChangeMethod argument.", SlotTags::DisplayGroup("MyInputChangeMethod"))
*
* ----------------------------------------------------------------------------------------------------------- */
#define OnInputChangeMethod(Name, Description, ...)

/*
*----------------------------------------------------------------------------------------------------------
*
* ExecutionInput
* This is a shorthand macro to easily create an execution input slot.
*
* Examples:
*   ExecutionInput("Start Process", "Signals this node to begin processing.")
*
* ---------------------------------------------------------------------------------------------------------- */
#define ExecutionInput(Name, Description, ...)

/*
*----------------------------------------------------------------------------------------------------------
*
* ExecutionOutput
* This is a shorthand macro to easily create an execution output slot.
*
* Examples:
*   ExecutionOutput("On Start Process", "Output of start process execution.");
*
* ---------------------------------------------------------------------------------------------------------- */
#define ExecutionOutput(Name, Description, ...)

/*
*----------------------------------------------------------------------------------------------------------
*
* ExecutionLatentOutput
* Similar to ExecutionOutput however it is used to make it explicit that the output slot will be latent,
* this means that the node maintains state and may not signal this slot immediately.
*
* Example:
*   ExecutionLatentOutput("On Finished", "Will be signaled when the operation is complete.");
*
* ---------------------------------------------------------------------------------------------------------- */
#define ExecutionLatentOutput(Name, Description, ...)

/*
*----------------------------------------------------------------------------------------------------------
*
* Data
* Provides shorthand for exposing data to serialize context and edit context,
* mainly used with SlotTags::PropertyReference for property data.
*
* Example:
*   int m_data = 1;
*   PropertyData(int, "My Data", "My Serialized Data.", SlotTags::PropertyReference(m_data));
*
* ---------------------------------------------------------------------------------------------------------- */
#define PropertyData(Type, Name, Description, ...)

/*
*----------------------------------------------------------------------------------------------------------
*
* DataInput
* Provides shorthand for creating an input data slot. 
*
* Coupled with macro InputMethod/BranchMethod/OnInputChangeMethod to give parameter editor definition
*
* Example:
*   InputMethod("Do Something", "My DoSomething Function.")
*   DataInput(int, "DoSomething:Arg", 0, "My DoSomething argument.")
*
* ---------------------------------------------------------------------------------------------------------- */
#define DataInput(Type, Name, DefaultVal, Description, ...)

/*
*----------------------------------------------------------------------------------------------------------
*
* DataOutput
* Provides shorthand for creating an output data slot.
*
* Coupled with macro InputMethod/BranchMethod/OnInputChangeMethod to give result editor definition
*
* Example:
*   InputMethod("Do Something", "My DoSomething Function.")
*   DataOutput(int, "DoSomething:Result", 0, "My DoSomething result.")
*
* ---------------------------------------------------------------------------------------------------------- */
#define DataOutput(Type, Name, Description, ...)

/*
*----------------------------------------------------------------------------------------------------------
*
* DynamicValueDataInput
* Provides shorthand for creating an input dynamic value data slot.
*
* Examples:
*     DynamicValueDataInput("ValueData", "A generic value data.")
*
* ---------------------------------------------------------------------------------------------------------- */
#define DynamicValueDataInput(Name, Description, ...)

/*
*----------------------------------------------------------------------------------------------------------
*
* DynamicValueDataOutput
* Provides shorthand for creating an output dynamic value data slot.
*
* Examples:
*     DynamicValueDataOutput("ValueData", "A generic value data.")
*
* ---------------------------------------------------------------------------------------------------------- */
#define DynamicValueDataOutput(Name, Description, ...)

/*
*----------------------------------------------------------------------------------------------------------
*
* DynamicContainerDataInput
* Provides shorthand for creating an input dynamic container data slot.
*
* Coupled with macro ExecutionInput/ExecutionOutput/ExecutionLatentOutput to generate dynamic data input slot
*
* Examples:
*     DynamicContainerDataInput("ContainerData", "A generic container data.")
*
* ---------------------------------------------------------------------------------------------------------- */
#define DynamicContainerDataInput(Name, Description, ...)

/*
*----------------------------------------------------------------------------------------------------------
*
* DynamicContainerDataOutput
* Provides shorthand for creating an output dynamic container data slot.
*
* Coupled with macro ExecutionInput/ExecutionOutput/ExecutionLatentOutput to generate dynamic data output slot
*
* Examples:
*     DynamicContainerDataOutput("ContainerData", "A generic container data.")
*
* ---------------------------------------------------------------------------------------------------------- */
#define DynamicContainerDataOutput(Name, Description, ...)

/*
*----------------------------------------------------------------------------------------------------------
*
* DynamicAnyDataInput
* Provides shorthand for creating an input dynamic any data slot.
*
* Coupled with macro ExecutionInput/ExecutionOutput/ExecutionLatentOutput to generate dynamic data input slot
*
* Examples:
*     DynamicAnyDataInput("AnyData", "A generic any data.")
*
* ---------------------------------------------------------------------------------------------------------- */
#define DynamicAnyDataInput(Name, Description, ...)

/*
*----------------------------------------------------------------------------------------------------------
*
* DynamicAnyDataOutput
* Provides shorthand for creating an output dynamic any data slot.
*
* Coupled with macro ExecutionInput/ExecutionOutput/ExecutionLatentOutput to generate dynamic data output slot
*
* Examples:
*     DynamicAnyDataOutput("AnyData", "A generic any data.")
*
* ---------------------------------------------------------------------------------------------------------- */
#define DynamicAnyDataOutput(Name, Description, ...)

// Intellisense helpers, the following definitions exist to provide code completion details regarding what attributes are
// supported by the different tags.

// Revisited common tags, we should be able to remove NodeableCodegen eventually
namespace NodeableCodegen
{
    namespace ScriptCanvasTags
    {
        using OverrideName = const char*;
        using Uuid = const char*;
        using Category = const char*;
        using Icon = const char*;
        using Deprecated = const char*;

        struct Version
        {
            using ConverterFunction = bool(class AZ::SerializeContext& context, class AZ::SerializeContext::DataElementNode& classElement);
            Version(unsigned int /*version*/) {}
            Version(unsigned int /*version*/, ConverterFunction /*converter*/) {}
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
            EditAttributes(Args&& ... args) {}
        };

        struct BaseClass
        {
            BaseClass(AZStd::initializer_list<const char*>) {}
        };

        //struct Contracts
        //{
        //    explicit Contracts(AZStd::initializer_list<ScriptCanvas::Contract>) {}
        //};

        //struct RestrictedTypeContractTag
        //{
        //    explicit RestrictedTypeContractTag(AZStd::initializer_list<ScriptCanvas::Data::Type>) {}
        //};

        struct SupportsMethodContractTag
        {
            explicit SupportsMethodContractTag(const char*) {}
        };
    }
}

namespace NodeTags
{
    using NodeableCodegen::ScriptCanvasTags::OverrideName;
    using NodeableCodegen::ScriptCanvasTags::Uuid;
    using NodeableCodegen::ScriptCanvasTags::Version;
    using NodeableCodegen::ScriptCanvasTags::Icon;
    using NodeableCodegen::ScriptCanvasTags::EditAttributes;
    using NodeableCodegen::ScriptCanvasTags::Category;
    using NodeableCodegen::ScriptCanvasTags::Deprecated;

    using GraphEntryPoint = bool;
}

namespace SlotTags
{
    using NodeableCodegen::ScriptCanvasTags::OverrideName;
    //using NodeableCodegen::ScriptCanvasTags::Contracts;

    // Data specific
    using DisplayGroup = const char*;

    // PropertyData specific
    using PropertyReference = const char*;
    using PropertyInterface = const char*;

    // ExecutionSlot specific
    using BranchOf = const char*;

    // EditContext specific
    using NodeableCodegen::ScriptCanvasTags::EditAttributes;
    using NodeableCodegen::ScriptCanvasTags::Edit::UIHandler;
    using AzCommon::Attributes::ChangeNotify;
    using AzCommon::Attributes::Visibility;
    using AzCommon::Attributes::AutoExpand;
    using AzCommon::Attributes::DescriptionTextOverride;
    using AzCommon::Attributes::NameLabelOverride;
    using AzCommon::Attributes::Min;
    using AzCommon::Attributes::Max;

    // DynamicData specific
    //using NodeableCodegen::ScriptCanvasTags::RestrictedTypeContractTag;
    using NodeableCodegen::ScriptCanvasTags::SupportsMethodContractTag;
    using DynamicGroup = const char*;
}
