/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/EBus/Policies.h>

#ifndef AZ_TRACK_ASSET_SCOPES
// You may manually uncomment this to enable asset tracking.
//#   define AZ_TRACK_ASSET_SCOPES
#endif

#if !defined(AZ_TRACK_ASSET_SCOPES)
// Default to enabling asset tracking when memory tracking is enabled
#   define AZ_TRACK_ASSET_SCOPES
#endif


#ifdef AZ_TRACK_ASSET_SCOPES
#define AZ_ASSET_SCOPE_VARIABLE_NAME(line)  AZ_JOIN(_az_assettracking_scope_, line)

///////////////////////////////////////////////////////////////////////////////
// Preferred macros to use at the top of a scope you want to to track asset memory for.
///////////////////////////////////////////////////////////////////////////////

// Creates a new scope with a name, usually the name of an asset being loaded. (This may be a format-string, e.g. "Foo: %s", bar.c_str())
#   define AZ_ASSET_NAMED_SCOPE(...)  AZ::Debug::AssetTracking::Scope AZ_ASSET_SCOPE_VARIABLE_NAME(__LINE__) (AZ::Debug::AssetTracking::Scope::ScopeFromAssetId(__FILE__, __LINE__, __VA_ARGS__))

// Attempts to enter an existing scope that already owns some other allocation.
#   define AZ_ASSET_ATTACH_TO_SCOPE(other)  AZ::Debug::AssetTracking::Scope AZ_ASSET_SCOPE_VARIABLE_NAME(__LINE__) (AZ::Debug::AssetTracking::Scope::ScopeFromAttachment((other), __FILE__, __LINE__))

///////////////////////////////////////////////////////////////////////////////
// Optional macros to manually enter and exit a scope. 
// It is the responsibility of the user to make sure every call to AZ_ASSET_ENTER_SCOPE_* is matched by a corresponding call to AZ_ASSET_EXIT_SCOPE.
///////////////////////////////////////////////////////////////////////////////
#   define AZ_ASSET_ENTER_SCOPE_BY_ASSET_ID(...)  AZ::Debug::AssetTracking::EnterScopeByAssetId(__FILE__, __LINE__, __VA_ARGS__)
#   define AZ_ASSET_ENTER_SCOPE_BY_ATTACHMENT(other)  AZ::Debug::AssetTracking::EnterScopeByAttachment((other), __FILE__, __LINE__)
#   define AZ_ASSET_EXIT_SCOPE  AZ::Debug::AssetTracking::ExitScope()

#else
#   define AZ_ASSET_NAMED_SCOPE(...)  (void)0
#   define AZ_ASSET_ATTACH_TO_SCOPE(other)  (void)0

#   define AZ_ASSET_ENTER_SCOPE_BY_ASSET_ID(...)  (void)0
#   define AZ_ASSET_ENTER_SCOPE_BY_ATTACHMENT(other)  (void)0
#   define AZ_ASSET_EXIT_SCOPE  (void)0

#endif

namespace AZ
{
    class ReflectContext;

    namespace Debug
    {
        class AssetTrackingImpl;
        class AssetTreeBase;
        class AssetTreeNodeBase;
        class AssetAllocationTableBase;

        class AssetTracking
        {
        public:
            AZ_TYPE_INFO(AssetTracking, "{D4335180-09A2-415A-8B50-9B734E7CE1E6}");
            AZ_CLASS_ALLOCATOR(AssetTracking, OSAllocator, 0);

            // Provide RAII method for entering and exiting scopes.
            // Generally you will want to use the macros at the top of this file rather than instantiating this object directly.
            class Scope
            {
            public:
                static Scope ScopeFromAssetId(const char* file, int line, const char* fmt, ...);
                static Scope ScopeFromAttachment(void* attachTo, const char* file, int line);

                Scope(Scope&&) = default;
                ~Scope();

            private:
                Scope();
            };

            // Generally you will want to use the macros at the top of this file rather than calling these functions directly.
            static void EnterScopeByAssetId(const char* file, int line, const char* fmt, ...);
            static void EnterScopeByAttachment(void* attachTo, const char* file, int line);
            static void ExitScope();
            static const char* GetDebugScope();

            AssetTracking(AssetTreeBase* assetTree, AssetAllocationTableBase* allocationTable);
            ~AssetTracking();

            AssetTreeNodeBase* GetCurrentThreadAsset() const;

        private:
            AZStd::unique_ptr<AssetTrackingImpl> m_impl;
        };

        // An EBus processing policy that attempts to attach to an existing scope before calling a handler.
        //
        // Use this on EBuses where you want the callees to track asset memory during their event handlers.
        // This will work so long as the callees were themselves allocated inside an existing asset scope.
        //
        // May be added to an existing EBus with the following code:
        //    using EventProcessingPolicy = Debug::AssetTrackingEventProcessingPolicy;
        //
        template<typename Parent = EBusEventProcessingPolicy>
        struct AssetTrackingEventProcessingPolicy
        {
            template<class Results, class Function, class Interface, class... InputArgs>
            static void CallResult(Results& results, Function&& func, Interface&& iface, InputArgs&&... args)
            {
                AZ_ASSET_ATTACH_TO_SCOPE(iface);
                Parent::CallResult(results, AZStd::forward<Function>(func), AZStd::forward<Interface>(iface), AZStd::forward<InputArgs>(args)...);
            }

            template<class Function, class Interface, class... InputArgs>
            static void Call(Function&& func, Interface&& iface, InputArgs&&... args)
            {
                AZ_ASSET_ATTACH_TO_SCOPE(iface);
                Parent::Call(AZStd::forward<Function>(func), AZStd::forward<Interface>(iface), AZStd::forward<InputArgs>(args)...);
            }
        };
    }

} // namespace AzFramework
