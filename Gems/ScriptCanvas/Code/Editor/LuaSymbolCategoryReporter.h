/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace ScriptCanvasEditor
{
    //////////////////////////////////////////////////////////////////////////
    // Reflected result rows
    //
    // Each row carries the FINAL resolved Script Canvas category path for one
    // reflected symbol (translation override -> reflection Category attribute ->
    // default bucket, exactly as the Node Palette computes it). A remote tool
    // joins these onto the symbol set it already scrapes, keyed by the identity
    // it already has: typeId for classes, name for globals and EBuses. A miss
    // means "uncategorized" and the tool keeps its flat layout for that symbol.
    //////////////////////////////////////////////////////////////////////////

    struct LuaClassCategory
    {
        AZ_TYPE_INFO(LuaClassCategory, "{A1E8C4D2-3F6B-4A19-9C2E-1B7D5E0F8A34}");
        static void Reflect(AZ::ReflectContext* context);

        AZ::Uuid      m_typeId;     // join key
        AZStd::string m_name;       // behavior class name (reference / debugging)
        AZStd::string m_category;   // final resolved category path

        AZStd::string ToString() const;
    };

    struct LuaGlobalCategory
    {
        AZ_TYPE_INFO(LuaGlobalCategory, "{B2F9D5E3-4A7C-4B2A-8D3F-2C8E6F1A9B45}");
        static void Reflect(AZ::ReflectContext* context);

        AZStd::string m_name;       // join key (global method or property name)
        AZStd::string m_category;   // final resolved category path

        AZStd::string ToString() const;
    };

    struct LuaEBusCategory
    {
        AZ_TYPE_INFO(LuaEBusCategory, "{C3A0E6F4-5B8D-4C3B-9E4A-3D9F7A2B0C56}");
        static void Reflect(AZ::ReflectContext* context);

        AZStd::string m_name;              // join key (EBus name)
        AZStd::string m_senderCategory;    // path for Event / Broadcast senders
        AZStd::string m_handlerCategory;   // path for Notification handler events

        AZStd::string ToString() const;
    };

    //////////////////////////////////////////////////////////////////////////
    // Request bus
    //////////////////////////////////////////////////////////////////////////

    class LuaSymbolCategoryReporterRequests
    {
    public:
        AZ_RTTI(LuaSymbolCategoryReporterRequests, "{D4B1F7A5-6C9E-4D4C-AF5B-4E0A8B3C1D67}");
        virtual ~LuaSymbolCategoryReporterRequests() = default;

        virtual const AZStd::vector<LuaClassCategory>& GetClassCategories() = 0;
        virtual const AZStd::vector<LuaGlobalCategory>& GetGlobalMethodCategories() = 0;
        virtual const AZStd::vector<LuaGlobalCategory>& GetGlobalPropertyCategories() = 0;
        virtual const AZStd::vector<LuaEBusCategory>& GetEBusCategories() = 0;
    };

    class LuaSymbolCategoryReporterBusTraits
        : public AZ::EBusTraits
    {
    public:
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };

    using LuaSymbolCategoryReporterRequestBus = AZ::EBus<LuaSymbolCategoryReporterRequests, LuaSymbolCategoryReporterBusTraits>;

    //////////////////////////////////////////////////////////////////////////
    // System component
    //
    // Editor-only. Iterates the BehaviorContext and, using the GraphCanvas
    // translation database, resolves the final category path for every class,
    // global and EBus, then serves the results over a behavior-reflected bus so
    // the offline Python dumper can emit lua_symbol_categories.json.
    //////////////////////////////////////////////////////////////////////////

    class LuaSymbolCategoryReporterSystemComponent
        : public AZ::Component
        , protected LuaSymbolCategoryReporterRequestBus::Handler
    {
    public:
        AZ_COMPONENT(LuaSymbolCategoryReporterSystemComponent, "{E5C2A8B6-7D0F-4E5D-B06C-5F1B9C4D2E78}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // LuaSymbolCategoryReporterRequestBus
        const AZStd::vector<LuaClassCategory>& GetClassCategories() override;
        const AZStd::vector<LuaGlobalCategory>& GetGlobalMethodCategories() override;
        const AZStd::vector<LuaGlobalCategory>& GetGlobalPropertyCategories() override;
        const AZStd::vector<LuaEBusCategory>& GetEBusCategories() override;

    private:
        // Resolves every symbol's category once, on first request, and caches it.
        void BuildCategoriesIfNeeded();

        AZStd::vector<LuaClassCategory>  m_classCategories;
        AZStd::vector<LuaGlobalCategory> m_globalMethodCategories;
        AZStd::vector<LuaGlobalCategory> m_globalPropertyCategories;
        AZStd::vector<LuaEBusCategory>   m_ebusCategories;
        bool m_built = false;
    };
}
