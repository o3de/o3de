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
#include <AzCore/std/typetraits/add_pointer.h>
#include <ScriptCanvas/Libraries/Libraries.h>
#include <ScriptCanvas/Translation/TranslationContext.h>

#include "Node.h"
#include "Attributes.h"

 /**
  * Deprecated
  */

//! Deprecated generic function node macros
#define SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(NODE_NAME, CATEGORY, UUID, DESCRIPTION, ...)\
    SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(NODE_NAME, CATEGORY, UUID, "")

#define SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE_DEPRECATED(NODE_NAME, CATEGORY, UUID, DESCRIPTION, ...)\
    SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(NODE_NAME, CATEGORY, UUID, "")

#define SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(NODE_NAME, DEFAULT_FUNC, CATEGORY, UUID, DESCRIPTION, ...)\
    SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(NODE_NAME, CATEGORY, UUID, "")

#define SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS_DEPRECATED(NODE_NAME, DEFAULT_FUNC, CATEGORY, UUID, DESCRIPTION, ...)\
    SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(NODE_NAME, CATEGORY, UUID, "")

#define SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(NODE_NAME, CATEGORY, UUID, DESCRIPTION, ...)\
    SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(NODE_NAME, CATEGORY, UUID, "")

#define SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_DEPRECATED(NODE_NAME, CATEGORY, UUID, DESCRIPTION, ...)\
    SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(NODE_NAME, CATEGORY, UUID, "")

//! Macros to migrate deprecated generic function node to autogen function node
#define SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(NODE_NAME, CATEGORY, UUID, METHOD_NAME)\
    struct NODE_NAME##Traits\
    {\
        AZ_TYPE_INFO(NODE_NAME##Traits, UUID);\
        static const char* GetDependency() { return CATEGORY; }\
        static const char* GetNodeName() { return #NODE_NAME; };\
        static const char* GetReplacementMethodName() { return METHOD_NAME; };\
    };\
    using NODE_NAME##Node = ScriptCanvas::NodeFunctionGenericMultiReturn<AZStd::add_pointer_t<decltype(NODE_NAME)>, NODE_NAME##Traits, &NODE_NAME>;

namespace ScriptCanvas
{
    template<auto Function, typename t_Traits>
    class NodeFunctionGenericMultiReturnImpl;

    template<typename t_Func, typename t_Traits, t_Func function>
    using NodeFunctionGenericMultiReturn = NodeFunctionGenericMultiReturnImpl<function, t_Traits>;

    template<auto Function, typename t_Traits>
    inline AZ::TypeNameString GetO3deTypeName(AZ::Adl, AZStd::type_identity<NodeFunctionGenericMultiReturnImpl<Function, t_Traits>>)
    {
        using t_Func = decltype(Function);
        constexpr const char* displayName = "NodeFunctionGenericMultiReturn";
        static AZ::TypeNameString s_typeName;
        if (s_typeName.empty())
        {
            AZ::TypeNameString typeName{ displayName };
            typeName += '<';

            bool prependSeparator = false;
            for (AZStd::string_view templateParamName : { AZ::Internal::AggregateTypes<t_Func, t_Traits>::TypeName() })
            {
                typeName += prependSeparator ? AZ::Internal::TypeNameSeparator : "";
                typeName += templateParamName;
                prependSeparator = true;
            }
            typeName += '>';
            s_typeName = typeName;
        }

        return s_typeName;
    }
    template<auto Function, typename t_Traits>
    inline AZ::TypeId GetO3deTypeId(AZ::Adl, AZStd::type_identity<NodeFunctionGenericMultiReturnImpl<Function, t_Traits>>)
    {
        using t_Func = decltype(Function);
        constexpr AZ::TemplateId templateUuid{ "{DC5B1799-6C5B-4190-8D90-EF0C2D1BCE4E}" };
        static const AZ::TypeId s_typeId = templateUuid + AZ::Internal::AggregateTypes<t_Func, t_Traits>::GetCanonicalTypeId();
        return s_typeId;
    }


    template<auto Function, typename t_Traits>
    class NodeFunctionGenericMultiReturnImpl
        : public Node
    {
    public:
        using t_Func = decltype(Function);

        AZ_PUSH_DISABLE_WARNING(5046, "-Wunknown-warning-option"); // 'function' : Symbol involving type with internal linkage not defined
        AZ_RTTI_NO_TYPE_INFO_DECL();
        AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(NodeFunctionGenericMultiReturnImpl);
        AZ_COMPONENT_BASE(NodeFunctionGenericMultiReturnImpl);
        AZ_CLASS_ALLOCATOR(NodeFunctionGenericMultiReturnImpl, AZ::ComponentAllocator);
        AZ_POP_DISABLE_WARNING;

        static constexpr void DeprecatedTypeNameVisitor(
            const AZ::DeprecatedTypeNameCallback& visitCallback)
        {
            // NodeFunctionGenericMultiReturn previously restricted the typename to 128 bytes
            AZStd::array<char, 128> deprecatedName{};

            // Due to the extra set of parenthesis, the actual type name of NodeFunctionGenericMultiReturn started literally as
            // "(NodeFunctionGenericMultiReturn<t_Func, t_Traits, function>"
            AZ::Internal::AzTypeInfoSafeCat(deprecatedName.data(), deprecatedName.size(), "(NodeFunctionGenericMultiReturn<t_Func, t_Traits, function>)<");
            auto AppendDeprecatedFunctionName = [&deprecatedName](AZStd::string_view deprecatedFuncName)
            {
                // The function signature name was restricted to 64 bytes
                AZStd::array<char, 64> deprecatedFuncSignature{};
                AZ::Internal::AzTypeInfoSafeCat(deprecatedFuncSignature.data(), deprecatedFuncSignature.size(), deprecatedFuncName.data());

                // If the function type was a pointer then it a '*' needs to be appended to it.
                // But the AZ_TYPE_INFO_INTERNAL_SPECIALIZE_CV macro will only append a '*' if the function signature buffer has room for it
                if constexpr (AZStd::is_pointer_v<t_Func>)
                {
                    AZ::Internal::AzTypeInfoSafeCat(deprecatedFuncSignature.data(), deprecatedFuncSignature.size(), "*");
                }

                // Now copy the function signature to the deprecatedName buffer
                AZ::Internal::AzTypeInfoSafeCat(deprecatedName.data(), deprecatedName.size(), deprecatedFuncSignature.data());
            };
            AZ::DeprecatedTypeNameVisitor<t_Func>(AppendDeprecatedFunctionName);
            // The old AZ::Internal::AggregateTypes implementations placed a space after each argument as a separator
            AZ::Internal::AzTypeInfoSafeCat(deprecatedName.data(), deprecatedName.size(), " ");
            AZ::Internal::AzTypeInfoSafeCat(deprecatedName.data(), deprecatedName.size(), AZ::AzTypeInfo<t_Traits>::Name());
            AZ::Internal::AzTypeInfoSafeCat(deprecatedName.data(), deprecatedName.size(), " >");

            if (visitCallback)
            {
                visitCallback(deprecatedName.data());
            }
        }


        static const char* GetNodeFunctionName()
        {
            return t_Traits::GetNodeName();
        }

        static t_Func GetFunction()
        {
            return Function;
        }

        static void Reflect(AZ::ReflectContext* reflectContext)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                serializeContext->Class<NodeFunctionGenericMultiReturnImpl, Node>()
                    ->Version(1)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Field("Initialized", &NodeFunctionGenericMultiReturnImpl::m_initialized)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<NodeFunctionGenericMultiReturnImpl>(t_Traits::GetNodeName(), "Deprecated Generic Function Node")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "Deprecated Generic Function Node")
                        ->Attribute(AZ::Script::Attributes::Deprecated, true)
                        ->Attribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride, "DeprecatedNodeTitlePalette")
                        ->Attribute(AZ::Edit::Attributes::Category, "Deprecated")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
                }
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

        bool IsDeprecated() const override
        {
            return true;
        }

        ScriptCanvas::NodeReplacementConfiguration GetReplacementNodeConfiguration() const override
        {
            ScriptCanvas::NodeReplacementConfiguration replacementNode;
            // Replacement node should always be behavior context global method
            // ScriptCanvas method node uuid is E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF
            replacementNode.m_type = AZ::Uuid("E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF");
            replacementNode.m_methodName = t_Traits::GetReplacementMethodName();
            return replacementNode;
        }

    private:
        bool m_initialized = false;
    };

    AZ_RTTI_NO_TYPE_INFO_IMPL_INLINE((NodeFunctionGenericMultiReturnImpl, AZ_TYPE_INFO_AUTO, AZ_TYPE_INFO_CLASS), Node);

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
        static void AddToRegistry([[maybe_unused]] NodeRegistry& nodeRegistry)
        {
            // Deprecated, do nothing here
            AZ_Warning("ScriptCanvas", false, "NodeFunctionGeneric is deprecated, please migrate to autogen function node.");
        }

        template<typename t_Library>
        static void Reflect(AZ::BehaviorContext* behaviorContext, const char* libraryName)
        {
            auto reflection = behaviorContext->Class<t_Library>(libraryName);
            reflection->Attribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, AZ::AttributeIsValid::IfPresent)
                ->Attribute(AZ::ScriptCanvasAttributes::Internal::ImplementedAsNodeGeneric, true);
            SCRIPT_CANVAS_CALL_ON_INDEX_SEQUENCE(reflection->Method(t_Node::GetNodeFunctionName(), t_Node::GetFunction())->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List | AZ::Script::Attributes::ExcludeFlags::Documentation));
        }
    };
}
