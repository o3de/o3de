/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/ScriptCanvas/ScriptCanvasAttributes.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <Editor/LuaSymbolCategoryReporter.h>
#include <Editor/Translation/TranslationHelper.h>   // pulls GraphCanvas TranslationBus + AssetContext keys

namespace ScriptCanvasEditor
{
    //////////////////////////////////////////////////////////////////////////
    // Category resolution - a faithful, UI-free copy of the Node Palette's
    // per-kind rules (NodePaletteModel.cpp). Keep these in step if that file
    // changes. Each rule is: translation override -> reflection Category
    // attribute -> default bucket, with the same leaf/append behavior per kind.
    //////////////////////////////////////////////////////////////////////////

    namespace
    {
        // Default buckets - must match NodePaletteModel's constants exactly.
        constexpr char DefaultClassMethodCategory[]    = "Class Methods";
        constexpr char DefaultGlobalMethodCategory[]   = "Global Methods";
        constexpr char DefaultGlobalConstantCategory[] = "Global Constants";
        constexpr char DefaultEbusHandlerCategory[]    = "Event Handlers";
        constexpr char DefaultEbusEventCategory[]      = "Events";

        // Reads the reflection Category attribute (path form, e.g. "Gameplay/Camera").
        // Empty when unspecified - the correct "uncategorized" signal.
        AZStd::string GetCategoryAttribute(const AZ::AttributeArray& attributes, const AZ::BehaviorContext& behaviorContext)
        {
            AZStd::string retVal;
            if (AZ::Attribute* categoryAttribute = AZ::FindAttribute(AZ::Script::Attributes::Category, attributes))
            {
                AZ::AttributeReader(nullptr, categoryAttribute).Read<AZStd::string>(retVal, behaviorContext);
            }
            return retVal;
        }

        // Looks up the "<context>.<name>.details" translation entry, the tier-1 override source.
        GraphCanvas::TranslationRequests::Details GetTranslationDetails(const char* assetContext, const AZStd::string& name)
        {
            GraphCanvas::TranslationKey key;
            key << assetContext << name << "details";

            GraphCanvas::TranslationRequests::Details details;
            GraphCanvas::TranslationRequestBus::BroadcastResult(details, &GraphCanvas::TranslationRequests::GetDetails, key, details);
            return details;
        }

        // Classes: translation ?: attribute ?: (prettyName | "Class Methods"), then "/" + leaf.
        AZStd::string ResolveClassCategory(const AZStd::string& className, const AZ::BehaviorClass& behaviorClass, const AZ::BehaviorContext& behaviorContext)
        {
            const GraphCanvas::TranslationRequests::Details details =
                GetTranslationDetails(TranslationHelper::AssetContext::BehaviorClassContext, behaviorClass.m_name);

            AZStd::string categoryPath = details.m_category;
            if (categoryPath.empty())
            {
                categoryPath = GetCategoryAttribute(behaviorClass.m_attributes, behaviorContext);
            }

            AZStd::string classNamePretty(className);
            if (AZ::Attribute* prettyNameAttribute = AZ::FindAttribute(AZ::ScriptCanvasAttributes::PrettyName, behaviorClass.m_attributes))
            {
                AZ::AttributeReader(nullptr, prettyNameAttribute).Read<AZStd::string>(classNamePretty, behaviorContext);
            }

            if (categoryPath.empty())
            {
                categoryPath = classNamePretty.empty() ? DefaultClassMethodCategory : classNamePretty;
            }

            categoryPath.append("/");
            categoryPath.append(details.m_name.empty() ? classNamePretty : details.m_name);
            return categoryPath;
        }

        // Global methods: translation ?: attribute ?: "Global Methods". No leaf.
        AZStd::string ResolveGlobalMethodCategory(const AZ::BehaviorMethod& behaviorMethod, const AZ::BehaviorContext& behaviorContext)
        {
            const GraphCanvas::TranslationRequests::Details details =
                GetTranslationDetails(TranslationHelper::AssetContext::BehaviorGlobalMethodContext, behaviorMethod.m_name);

            if (!details.m_category.empty())
            {
                return details.m_category;
            }

            const AZStd::string categoryPath = GetCategoryAttribute(behaviorMethod.m_attributes, behaviorContext);
            return categoryPath.empty() ? DefaultGlobalMethodCategory : categoryPath;
        }

        // Global constants: translation ?: "Global Constants". Note: no attribute fallback,
        // matching the Node Palette (RegisterGlobalConstant).
        AZStd::string ResolveGlobalPropertyCategory(const AZStd::string& propertyName)
        {
            AZStd::string name = propertyName;
            AZ::StringFunc::Replace(name, "::Getter", "");
            AZ::StringFunc::Replace(name, "::Setter", "");

            const GraphCanvas::TranslationRequests::Details details =
                GetTranslationDetails(TranslationHelper::AssetContext::BehaviorGlobalPropertyContext, name);

            return details.m_category.empty() ? DefaultGlobalConstantCategory : details.m_category;
        }

        // Shared EBus rule (sender + handler differ only by context key and default bucket).
        AZStd::string ResolveEBusCategory(const AZ::BehaviorEBus& behaviorEbus, const AZ::BehaviorContext& behaviorContext,
            const char* assetContext, const char* defaultCategory)
        {
            const GraphCanvas::TranslationRequests::Details details = GetTranslationDetails(assetContext, behaviorEbus.m_name);

            AZStd::string categoryPath = details.m_category.empty()
                ? GetCategoryAttribute(behaviorEbus.m_attributes, behaviorContext)
                : details.m_category;

            if (!categoryPath.empty())
            {
                categoryPath.append("/");
            }
            else
            {
                categoryPath = AZStd::string::format("%s/", defaultCategory);
            }

            if (!details.m_name.empty())
            {
                categoryPath.append(details.m_name);
            }
            else if (categoryPath.contains(defaultCategory))
            {
                categoryPath.append(behaviorEbus.m_name);
            }

            return categoryPath;
        }
    } // namespace

    //////////////////////////////////////////////////////////////////////////
    // Reflected result rows
    //////////////////////////////////////////////////////////////////////////

    AZStd::string LuaClassCategory::ToString() const
    {
        return AZStd::string::format("%s [%s] -> %s", m_name.c_str(), m_typeId.ToString<AZStd::string>().c_str(), m_category.c_str());
    }

    void LuaClassCategory::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<LuaClassCategory>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "script")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("typeId", BehaviorValueProperty(&LuaClassCategory::m_typeId))
                ->Property("name", BehaviorValueProperty(&LuaClassCategory::m_name))
                ->Property("category", BehaviorValueProperty(&LuaClassCategory::m_category))
                ->Method("ToString", &LuaClassCategory::ToString)
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;
        }
    }

    AZStd::string LuaGlobalCategory::ToString() const
    {
        return AZStd::string::format("%s -> %s", m_name.c_str(), m_category.c_str());
    }

    void LuaGlobalCategory::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<LuaGlobalCategory>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "script")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("name", BehaviorValueProperty(&LuaGlobalCategory::m_name))
                ->Property("category", BehaviorValueProperty(&LuaGlobalCategory::m_category))
                ->Method("ToString", &LuaGlobalCategory::ToString)
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;
        }
    }

    AZStd::string LuaEBusCategory::ToString() const
    {
        return AZStd::string::format("%s -> sender[%s] handler[%s]", m_name.c_str(), m_senderCategory.c_str(), m_handlerCategory.c_str());
    }

    void LuaEBusCategory::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<LuaEBusCategory>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "script")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("name", BehaviorValueProperty(&LuaEBusCategory::m_name))
                ->Property("senderCategory", BehaviorValueProperty(&LuaEBusCategory::m_senderCategory))
                ->Property("handlerCategory", BehaviorValueProperty(&LuaEBusCategory::m_handlerCategory))
                ->Method("ToString", &LuaEBusCategory::ToString)
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // System component
    //////////////////////////////////////////////////////////////////////////

    void LuaSymbolCategoryReporterSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        LuaClassCategory::Reflect(context);
        LuaGlobalCategory::Reflect(context);
        LuaEBusCategory::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<LuaSymbolCategoryReporterSystemComponent, AZ::Component>()
                ->Version(0);

            serializeContext->RegisterGenericType<AZStd::vector<LuaClassCategory>>();
            serializeContext->RegisterGenericType<AZStd::vector<LuaGlobalCategory>>();
            serializeContext->RegisterGenericType<AZStd::vector<LuaEBusCategory>>();
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<LuaSymbolCategoryReporterRequestBus>("LuaSymbolCategoryReporterBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "script")
                ->Event("GetClassCategories", &LuaSymbolCategoryReporterRequests::GetClassCategories)
                ->Event("GetGlobalMethodCategories", &LuaSymbolCategoryReporterRequests::GetGlobalMethodCategories)
                ->Event("GetGlobalPropertyCategories", &LuaSymbolCategoryReporterRequests::GetGlobalPropertyCategories)
                ->Event("GetEBusCategories", &LuaSymbolCategoryReporterRequests::GetEBusCategories)
                ;
        }
    }

    void LuaSymbolCategoryReporterSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("LuaSymbolCategoryReporterService"));
    }

    void LuaSymbolCategoryReporterSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("LuaSymbolCategoryReporterService"));
    }

    void LuaSymbolCategoryReporterSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        // The BehaviorContext and translation database are queried lazily on first request, by which
        // point the editor is fully loaded; no hard service dependency is required.
    }

    void LuaSymbolCategoryReporterSystemComponent::Activate()
    {
        LuaSymbolCategoryReporterRequestBus::Handler::BusConnect();
    }

    void LuaSymbolCategoryReporterSystemComponent::Deactivate()
    {
        LuaSymbolCategoryReporterRequestBus::Handler::BusDisconnect();
    }

    void LuaSymbolCategoryReporterSystemComponent::BuildCategoriesIfNeeded()
    {
        if (m_built)
        {
            return;
        }

        AZ::BehaviorContext* behaviorContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        if (!behaviorContext)
        {
            return;
        }

        for (const auto& [className, behaviorClass] : behaviorContext->m_classes)
        {
            if (!behaviorClass)
            {
                continue;
            }

            LuaClassCategory row;
            row.m_typeId = behaviorClass->m_typeId;
            row.m_name = className;
            row.m_category = ResolveClassCategory(className, *behaviorClass, *behaviorContext);
            m_classCategories.push_back(AZStd::move(row));
        }

        for (const auto& [methodName, behaviorMethod] : behaviorContext->m_methods)
        {
            if (!behaviorMethod)
            {
                continue;
            }

            LuaGlobalCategory row;
            row.m_name = methodName;
            row.m_category = ResolveGlobalMethodCategory(*behaviorMethod, *behaviorContext);
            m_globalMethodCategories.push_back(AZStd::move(row));
        }

        for (const auto& [propertyName, behaviorProperty] : behaviorContext->m_properties)
        {
            if (!behaviorProperty)
            {
                continue;
            }

            LuaGlobalCategory row;
            row.m_name = propertyName;
            row.m_category = ResolveGlobalPropertyCategory(propertyName);
            m_globalPropertyCategories.push_back(AZStd::move(row));
        }

        for (const auto& [ebusName, behaviorEbus] : behaviorContext->m_ebuses)
        {
            if (!behaviorEbus)
            {
                continue;
            }

            LuaEBusCategory row;
            row.m_name = ebusName;
            row.m_senderCategory = ResolveEBusCategory(*behaviorEbus, *behaviorContext, TranslationHelper::AssetContext::EBusSenderContext, DefaultEbusEventCategory);
            row.m_handlerCategory = ResolveEBusCategory(*behaviorEbus, *behaviorContext, TranslationHelper::AssetContext::EBusHandlerContext, DefaultEbusHandlerCategory);
            m_ebusCategories.push_back(AZStd::move(row));
        }

        m_built = true;
    }

    const AZStd::vector<LuaClassCategory>& LuaSymbolCategoryReporterSystemComponent::GetClassCategories()
    {
        BuildCategoriesIfNeeded();
        return m_classCategories;
    }

    const AZStd::vector<LuaGlobalCategory>& LuaSymbolCategoryReporterSystemComponent::GetGlobalMethodCategories()
    {
        BuildCategoriesIfNeeded();
        return m_globalMethodCategories;
    }

    const AZStd::vector<LuaGlobalCategory>& LuaSymbolCategoryReporterSystemComponent::GetGlobalPropertyCategories()
    {
        BuildCategoriesIfNeeded();
        return m_globalPropertyCategories;
    }

    const AZStd::vector<LuaEBusCategory>& LuaSymbolCategoryReporterSystemComponent::GetEBusCategories()
    {
        BuildCategoriesIfNeeded();
        return m_ebusCategories;
    }
}
