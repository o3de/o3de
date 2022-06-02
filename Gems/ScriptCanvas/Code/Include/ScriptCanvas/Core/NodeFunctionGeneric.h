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
    template<typename t_Func, typename t_Traits, t_Func function>
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
                    ->Version(1)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Field("Initialized", &NodeFunctionGenericMultiReturn::m_initialized)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<NodeFunctionGenericMultiReturn>(t_Traits::GetNodeName(), "Deprecated Generic Function Node")
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
