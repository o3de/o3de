/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_SCRIPT_SYSTEM_BUS_H
#define AZCORE_SCRIPT_SYSTEM_BUS_H

#include <AzCore/EBus/EBus.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Script/ScriptContext.h>

struct lua_State;

// #define SCRIPT_SYSTEM_SCRIPT_LOADED_DIAGNOSTICS_ENABLED

#if defined(SCRIPT_SYSTEM_SCRIPT_LOADED_DIAGNOSTICS_ENABLED)
#define SCRIPT_SYSTEM_SCRIPT_STATUS(window, msg, ...) AZ_TracePrintf(window, msg, __VA_ARGS__);
#else
#define SCRIPT_SYSTEM_SCRIPT_STATUS(window, msg, ...)
#endif

namespace AZ
{
    using InMemoryScriptModules = AZStd::vector<AZStd::pair<AZStd::string, AZ::Data::Asset<AZ::ScriptAsset>>>;

    struct ScriptLoadResult
    {
        enum class Status
        {
            Failed,
            Initial,
            OnStack,
        };

        Status status = Status::Failed;
        lua_State* lua = nullptr;
    };

    constexpr const char* k_scriptLoadBinary = "b";
    constexpr const char* k_scriptLoadBinaryOrText = "bt";
    constexpr const char* k_scriptLoadRawText = "t";

    /**
     * Event for communication with the component main application. There can
     * be only one application at a time. This is why this but is set to support
     * only one client/listener.
     */
    class ScriptSystemRequests
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::recursive_mutex;

        virtual ~ScriptSystemRequests() {}

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides - application is a singleton
        static const AZ::EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;  // we sort components on m_initOrder
        //////////////////////////////////////////////////////////////////////////

        /**
         *  Add a context with based on user created context, the system will not delete such context.  
         *
         * \param garbageCollectorStep you can pass -1 to use the internal system default step, otherwise you can use any value >=1
         */
        virtual ScriptContext* AddContext(ScriptContext* context, int garbageCollectorStep) = 0;
        /// Add a context with an ID, the system will handle the ownership of the context (delete it when removed and the system deactivates)
        virtual ScriptContext* AddContextWithId(ScriptContextId id) = 0;

        /// Removes a script context (without calling delete on it)
        virtual bool RemoveContext(ScriptContext* context) = 0;
        /// Removes and deletes a script context.
        virtual bool RemoveContextWithId(ScriptContextId id) = 0;

        /// Returns the script context that has been registered with the app, if there is one.
        virtual ScriptContext* GetContext(ScriptContextId id) = 0;

        /// Full GC will be performed
        virtual void GarbageCollect() = 0;

        /// Step GC 
        virtual void GarbageCollectStep(int numberOfSteps) = 0;

        /**
         * Load script asset into the a context.
         * If the load succeeds, the script table will be on top of the stack
         *
         * \param asset     script asset
         * \param id        the id of the context to load the script in
         *
         * \return whether or not the asset load succeeded
         */
        virtual bool Load(const Data::Asset<ScriptAsset>& asset, const char* mode, ScriptContextId id) = 0;
        virtual ScriptLoadResult LoadAndGetNativeContext(const Data::Asset<ScriptAsset>& asset, const char* mode, ScriptContextId id) = 0;

        /**
         * Clear the script asset cached in ScriptSystemComponent.
         */
        virtual void ClearAssetReferences(Data::AssetId assetBaseId) = 0;

        virtual void RestoreDefaultRequireHook(ScriptContextId id) = 0;

        virtual void UseInMemoryRequireHook(const InMemoryScriptModules& modules, ScriptContextId id) = 0;
    };

    using ScriptSystemRequestBus = AZ::EBus<ScriptSystemRequests>;
}

#endif // AZCORE_SCRIPT_SYSTEM_BUS_H
#pragma once
