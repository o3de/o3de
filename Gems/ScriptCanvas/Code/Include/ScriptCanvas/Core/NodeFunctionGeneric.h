/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/tuple.h>
#include <AzCore/std/typetraits/add_pointer.h>
#include <AzCore/std/typetraits/function_traits.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <ScriptCanvas/Libraries/Libraries.h>
#include <ScriptCanvas/Translation/TranslationContext.h>

#include "Node.h"
#include "Attributes.h"

/**
 * NodeFunctionGeneric.h
 * 
 * This file makes it really easy to take a single function and make into a ScriptCanvas node
 * with all of the necessary plumbing, by using a macro, and adding the result to a node registry.
 * 
 * Use SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE for a function of any arity that returns [0, N] 
 * arguments, wrapped in a tuple.
 * 
 * The macros will turn the function name into a ScriptCanvas node with name of the function
 * with "Node" appended to it.
 * 
 * \note As much as possible, it best to wrap functions that use 'native' ScriptCanvas types, 
 * and to pass them in/out by value.
 
 * You will need to add the nodes to the registry like any other node, and get a component description
 * from it, in order to have it show up in the editor, etc.
 * 
 * It is preferable to use this method for any node that provides ScriptCanvas-only functionality.
 * If you are creating a node that represents functionality that would be useful in Lua, or any other
 * client of BehaviorContext, it may be better to expose your functionality to BehaviorContext, unless 
 * performance in ScriptCanvas is an issue. This method will almost certainly provide faster run-time 
 * performance than a node that calls into BehaviorContext.
 * 
 * A good faith effort to support reference return types has been made. Pointers and references, even in
 * tuples, are supported. However, if your input or return values is T** or T*&, it won't work, and there
 * are no plans to support them. If your tuple return value is made up of references remember to return it with
 * std::forward_as_tuple, and not std::make_tuple.
 * 
 * \see MathGenerics.h and Math.cpp for example usage of the macros and generic registrar defined below.
 *  
 */

// this defines helps provide type safe static asserts in the results of the macros below
#define SCRIPT_CANVAS_FUNCTION_VAR_ARGS(...) (AZStd::tuple_size<decltype(AZStd::make_tuple(__VA_ARGS__))>::value)

namespace ScriptCanvas
{
    namespace Internal
    {
        template<class T, typename = AZStd::void_t<>>
        struct extended_tuple_size : AZStd::integral_constant<size_t, 1> {};

        template<class T>
        struct extended_tuple_size<T, AZStd::enable_if_t<IsTupleLike<T>::value>> : AZStd::tuple_size<T>{};

        template<>
        struct extended_tuple_size<void, AZStd::void_t<>>: AZStd::integral_constant<size_t, 0> {};
    }
}

#define SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE_WITH_DEFAULTS(NODE_NAME, DEFAULT_FUNC, CATEGORY, UUID, ISDEPRECATED, DESCRIPTION, ...)\
    struct NODE_NAME##Traits\
    {\
        AZ_TYPE_INFO(NODE_NAME##Traits, UUID);\
        using FunctionTraits = AZStd::function_traits< decltype(&NODE_NAME) >;\
        using ResultType = FunctionTraits::result_type;\
        static const size_t s_numArgs = FunctionTraits::arity;\
        static const size_t s_numNames = SCRIPT_CANVAS_FUNCTION_VAR_ARGS(__VA_ARGS__);\
        /*static const size_t s_numResults = ScriptCanvas::Internal::extended_tuple_size<ResultType>::value;*/\
        \
        static const char* GetArgName(size_t i)\
        {\
            return GetName(i).data();\
        }\
        \
        static const char* GetResultName(size_t i)\
        {\
            AZStd::string_view result = GetName(i + s_numArgs);\
            return !result.empty() ? result.data() : "Result";\
        }\
        \
        static const char* GetDependency() { return CATEGORY; }\
        static const char* GetCategory() { if (IsDeprecated()) return "Deprecated"; else return CATEGORY; };\
        static const char* GetDescription() { return DESCRIPTION; };\
        static const char* GetNodeName() { return #NODE_NAME; };\
        static bool IsDeprecated() { return ISDEPRECATED; };\
        \
    private:\
        static AZStd::string_view GetName(size_t i)\
        {\
            AZ_PUSH_DISABLE_WARNING(4296, "-Wunknown-warning-option")\
            static_assert(s_numArgs <= s_numNames, "Number of arguments is greater than number of names in " #NODE_NAME );\
            AZ_POP_DISABLE_WARNING\
            /*static_assert(s_numResults <= s_numNames, "Number of results is greater than number of names in " #NODE_NAME );*/\
            /*static_assert((s_numResults + s_numArgs) == s_numNames, "Argument name count + result name count != name count in " #NODE_NAME );*/\
            static const AZStd::array<AZStd::string_view, s_numNames> s_names = {{ __VA_ARGS__ }};\
            return i < s_names.size() ? s_names[i] : "";\
        }\
    };\
    using NODE_NAME##Node = ScriptCanvas::NodeFunctionGenericMultiReturn<AZStd::add_pointer_t<decltype(NODE_NAME)>, NODE_NAME##Traits, &NODE_NAME, AZStd::add_pointer_t<decltype(DEFAULT_FUNC)>, &DEFAULT_FUNC>;

#define SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(NODE_NAME, CATEGORY, UUID, DESCRIPTION, ...)\
    SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE_WITH_DEFAULTS(NODE_NAME, ScriptCanvas::NoDefaultArguments, CATEGORY, UUID, false, DESCRIPTION, __VA_ARGS__)

#define SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE_DEPRECATED(NODE_NAME, CATEGORY, UUID, DESCRIPTION, ...)\
    SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE_WITH_DEFAULTS(NODE_NAME, ScriptCanvas::NoDefaultArguments, CATEGORY, UUID, true, DESCRIPTION, __VA_ARGS__)

#define SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(NODE_NAME, DEFAULT_FUNC, CATEGORY, UUID, DESCRIPTION, ...)\
    SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE_WITH_DEFAULTS(NODE_NAME, DEFAULT_FUNC, CATEGORY, UUID, false, DESCRIPTION, __VA_ARGS__)

#define SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS_DEPRECATED(NODE_NAME, DEFAULT_FUNC, CATEGORY, UUID, DESCRIPTION, ...)\
    SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE_WITH_DEFAULTS(NODE_NAME, DEFAULT_FUNC, CATEGORY, UUID, true, DESCRIPTION, __VA_ARGS__)

#define SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(NODE_NAME, CATEGORY, UUID, DESCRIPTION, ...)\
    SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE_WITH_DEFAULTS(NODE_NAME, ScriptCanvas::NoDefaultArguments, CATEGORY, UUID, false, DESCRIPTION, __VA_ARGS__)

#define SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(NODE_NAME, CATEGORY, UUID, DESCRIPTION, ...)\
    SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE_WITH_DEFAULTS(NODE_NAME, ScriptCanvas::NoDefaultArguments, CATEGORY, UUID, true, DESCRIPTION, __VA_ARGS__)

namespace ScriptCanvas
{
    template<size_t... inputDatumIndices>
    struct SetDefaultValuesByIndex
    {
        template<typename... t_Args>
        AZ_INLINE static void _(Node& node, t_Args&&... args)
        {
            Help(node, AZStd::make_index_sequence<sizeof...(t_Args)>(), AZStd::forward<t_Args>(args)...);
        }

    private:
        template<AZStd::size_t... Is, typename... t_Args>
        AZ_INLINE static void Help(Node& node, AZStd::index_sequence<Is...>, t_Args&&... args)
        {
            static int indices[] = { inputDatumIndices... };
            static_assert(sizeof...(Is) == AZ_ARRAY_SIZE(indices), "size of default values doesn't match input datum indices for them");
            [[maybe_unused]] std::initializer_list<int> dummy = { (MoreHelp(node, indices[Is], AZStd::forward<t_Args>(args)), 0)... };
        }

        template<typename ArgType>
        AZ_INLINE static void MoreHelp(Node& node, size_t datumIndex, ArgType&& arg)
        {
            ModifiableDatumView datumView;
            node.FindModifiableDatumViewByIndex(datumIndex, datumView);

            datumView.template SetAs<AZStd::remove_cvref_t<ArgType>>(AZStd::forward<ArgType>(arg));
        }
    };

    // a no-op for generic function nodes that have no overrides for default input
    AZ_INLINE void NoDefaultArguments(Node&) {}

    template<typename t_Func, typename t_Traits, t_Func function, typename t_DefaultFunc, t_DefaultFunc defaultsFunction>
    class NodeFunctionGeneric
        : public Node
    {
    public:
        // This class has been deprecated for NodeFunctionGenericMultiReturn
        NodeFunctionGeneric() = delete;
        AZ_RTTI(((NodeFunctionGeneric<t_Func, t_Traits, function, t_DefaultFunc, defaultsFunction>), "{19E4AABE-1730-402C-A020-FC1006BC7F7B}", t_Func, t_Traits, t_DefaultFunc), Node);
        AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(NodeFunctionGeneric);
        AZ_COMPONENT_BASE(NodeFunctionGeneric, Node);
    };

    template<typename t_Func, typename t_Traits, t_Func function, typename t_DefaultFunc, t_DefaultFunc defaultsFunction>
    class NodeFunctionGenericMultiReturn
        : public Node
    {
    public:
    AZ_PUSH_DISABLE_WARNING(5046, "-Wunknown-warning-option") // 'function' : Symbol involving type with internal linkage not defined
        AZ_RTTI(((NodeFunctionGenericMultiReturn<t_Func, t_Traits, function>), "{DC5B1799-6C5B-4190-8D90-EF0C2D1BCE4E}", t_Func, t_Traits), Node);
        AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(NodeFunctionGenericMultiReturn);
        AZ_COMPONENT_BASE(NodeFunctionGenericMultiReturn, Node);
    AZ_POP_DISABLE_WARNING


        static const char* GetNodeFunctionName()
        {
            return t_Traits::GetNodeName();
        }

        static t_Func GetFunction()
        {
            return function;
        }

        static void Reflect(AZ::ReflectContext* reflectContext)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                serializeContext->Class<NodeFunctionGenericMultiReturn, Node>()
                    ->Version(1, &VersionConverter)
                    ->Attribute(AZ::Script::Attributes::Deprecated, t_Traits::IsDeprecated())
                    ->Field("Initialized", &NodeFunctionGenericMultiReturn::m_initialized)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<NodeFunctionGenericMultiReturn>(t_Traits::GetNodeName(), t_Traits::GetDescription())
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, t_Traits::GetDescription())
                        ->Attribute(AZ::Script::Attributes::Deprecated, t_Traits::IsDeprecated())
                        ->Attribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride, t_Traits::IsDeprecated() ? "DeprecatedNodeTitlePalette" : "")
                        ->Attribute(AZ::Edit::Attributes::Category, t_Traits::GetCategory())
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
                }

                // NodeFunctionGeneric class has been deprecated in terms of the this class
                serializeContext->ClassDeprecate("NodeFunctionGeneric", azrtti_typeid<NodeFunctionGeneric<t_Func, t_Traits, function, t_DefaultFunc, defaultsFunction>>(), &ConvertOldNodeGeneric);
                // Need to calculate the Typeid for the old NodeFunctionGeneric class as if it contains no templated types which are pointers
                AZ::Uuid genericTypeIdPointerRemoved = AZ::Uuid{ "{19E4AABE-1730-402C-A020-FC1006BC7F7B}" } + AZ::Internal::AggregateTypes<t_Func, t_Traits, t_DefaultFunc>::template Uuid<AZ::PointerRemovedTypeIdTag>();
                serializeContext->ClassDeprecate("NodeFunctionGenericTemplate", genericTypeIdPointerRemoved, &ConvertOldNodeGeneric);
                // NodeFunctionGenericMultiReturn class used to use the same typeid for pointer and not-pointer types for the function parameters
                // i.e, void Func(AZ::Entity*) and void Func2(AZ::Entity&) are the same typeid
                AZ::Uuid genericMultiReturnV1TypeId = AZ::Uuid{ "{DC5B1799-6C5B-4190-8D90-EF0C2D1BCE4E}" } + AZ::Internal::AggregateTypes<t_Func, t_Traits>::template Uuid<AZ::PointerRemovedTypeIdTag>();
                serializeContext->ClassDeprecate("NodeFunctionGenericMultiReturnV1", genericMultiReturnV1TypeId, &ConvertOldNodeGeneric);
            }
        }

        AZ::Outcome<DependencyReport, void> GetDependencies() const override
        {
            return AZ::Success(DependencyReport::NativeLibrary(t_Traits::GetDependency()));
        }

        AZ::Outcome<AZStd::string, void> GetFunctionCallName(const Slot*) const override
        { 
            return AZ::Success(AZStd::string(GetNodeFunctionName()));
        }

        AZ::Outcome<Grammar::LexicalScope, void> GetFunctionCallLexicalScope(const Slot* /*slot*/) const override
        {
            Grammar::LexicalScope scope;
            scope.m_type = Grammar::LexicalScopeType::Namespace;
            scope.m_namespaces.push_back(Translation::Context::GetCategoryLibraryName(t_Traits::GetDependency()));
            return AZ::Success(scope);
        }

    protected:

        template<typename ArgType, size_t Index>
        void CreateDataSlot(const ConnectionType& connectionType)
        {
            DataSlotConfiguration slotConfiguration;

            slotConfiguration.m_name = AZStd::string::format("%s: %s", Data::Traits<ArgType>::GetName().data(), t_Traits::GetArgName(Index));
            slotConfiguration.ConfigureDatum(AZStd::move(Datum(Data::FromAZType(Data::Traits<ArgType>::GetAZType()), Datum::eOriginality::Copy)));

            slotConfiguration.SetConnectionType(connectionType);
            AddSlot(slotConfiguration);
        }

        template<typename... t_Args, AZStd::size_t... Is>
        void AddInputDatumSlotHelper(AZStd::Internal::pack_traits_arg_sequence<t_Args...>, AZStd::index_sequence<Is...>)
        {
            static_assert(sizeof...(t_Args) == sizeof...(Is), "Argument size mismatch in NodeFunctionGenericMultiReturn");
            static_assert(sizeof...(t_Args) == t_Traits::s_numArgs, "Number of arguments does not match number of argument names in NodeFunctionGenericMultiReturn");

            SCRIPT_CANVAS_CALL_ON_INDEX_SEQUENCE(
                (CreateDataSlot<t_Args, Is>(ConnectionType::Input))
            );
        } 

        void ConfigureSlots() override
        {
            {
                ExecutionSlotConfiguration slotConfiguration("In", ConnectionType::Input);
                AddSlot(slotConfiguration);
            }

            {
                ExecutionSlotConfiguration slotConfiguration("Out", ConnectionType::Output);
                AddSlot(slotConfiguration);
            }
            
            AddInputDatumSlotHelper(typename AZStd::function_traits<t_Func>::arg_sequence{}, AZStd::make_index_sequence<AZStd::function_traits<t_Func>::arity>{});

            if (!m_initialized)
            {
                m_initialized = true;
                defaultsFunction(*this);
            }

            MultipleOutputInvoker<t_Func, function, t_Traits>::Add(*this);
        }

        static bool VersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement);
        static bool ConvertOldNodeGeneric(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement);

    private:

        bool m_initialized = false;
    };

#define SCRIPT_CANVAS_GENERICS_TO_VM(GenericClass, ReflectClass, BehaviorContext, CategoryName)\
    GenericClass::Reflect<ReflectClass>(BehaviorContext, ScriptCanvas::Translation::Context::GetCategoryLibraryName(CategoryName).c_str());

    template<typename... t_Node>
    class RegistrarGeneric
    {
    public:
        static void AddDescriptors(AZStd::vector<AZ::ComponentDescriptor*>& descriptors)
        {
            SCRIPT_CANVAS_CALL_ON_INDEX_SEQUENCE(descriptors.push_back(t_Node::CreateDescriptor()));
        }

        template<typename t_NodeGroup>
        static void AddToRegistry(NodeRegistry& nodeRegistry)
        {
            auto& nodes = nodeRegistry.m_nodeMap[azrtti_typeid<t_NodeGroup>()];
            SCRIPT_CANVAS_CALL_ON_INDEX_SEQUENCE(nodes.push_back({ azrtti_typeid<t_Node>(), AZ::AzTypeInfo<t_Node>::Name() }));
        }

        template<typename t_Library>
        static void Reflect(AZ::BehaviorContext* behaviorContext, const char* libraryName)
        {
            auto reflection = behaviorContext->Class<t_Library>(libraryName);
            reflection->Attribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, AZ::AttributeIsValid::IfPresent)->Attribute(AZ::ScriptCanvasAttributes::Internal::ImplementedAsNodeGeneric, true);
            SCRIPT_CANVAS_CALL_ON_INDEX_SEQUENCE(reflection->Method(t_Node::GetNodeFunctionName(), t_Node::GetFunction())->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List | AZ::Script::Attributes::ExcludeFlags::Documentation));
        }
    };

    template<typename t_Func, typename t_Traits, t_Func function, typename t_DefaultFunc, t_DefaultFunc defaultsFunction>
    bool NodeFunctionGenericMultiReturn<t_Func, t_Traits, function, t_DefaultFunc, defaultsFunction>::ConvertOldNodeGeneric(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
    {
        int nodeElementIndex = rootElement.FindElement(AZ_CRC("BaseClass1", 0xd4925735));
        if (nodeElementIndex == -1)
        {
            AZ_Error("Script Canvas", false, "Unable to find base class node element on deprecated class %s", rootElement.GetNameString());
            return false;
        }

        // The DataElementNode is being copied purposefully in this statement to clone the data
        AZ::SerializeContext::DataElementNode baseNodeElement = rootElement.GetSubElement(nodeElementIndex);
        if (!rootElement.Convert(serializeContext, azrtti_typeid<NodeFunctionGenericMultiReturn>()))
        {
            AZ_Error("Script Canvas", false, "Unable to convert deprecated class %s to class %s", rootElement.GetNameString(), RTTI_TypeName());
            return false;
        }

        if (rootElement.AddElement(baseNodeElement) == -1)
        {
            AZ_Error("Script Canvas", false, "Unable to add base class node element to %s", RTTI_TypeName());
            return false;
        }

        return true;
    }

    template<typename t_Func, typename t_Traits, t_Func function, typename t_DefaultFunc, t_DefaultFunc defaultsFunction>
    bool NodeFunctionGenericMultiReturn<t_Func, t_Traits, function, t_DefaultFunc, defaultsFunction>::VersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 1)
        {
            rootElement.AddElementWithData(serializeContext, "Initialized", true);
        }

        return true;
    }

}
