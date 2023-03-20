/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>

namespace AZ
{
    class ScriptTimePoint;

    /**
     * Script system component. It will manage all script contexts and provide script asset handler.
     * You must have one of this components on you system entity (or any) to enable script support
     * using the Hex facility. You are NOT REQUIRED to use it. You can use ScriptContext directly and
     * manage it anyway you like, but you will be provide ScriptAsset management too.
     */
    class ScriptSystemComponent
        : public Component
        , public ScriptSystemRequestBus::Handler
        , public SystemTickBus::Handler
        , public Data::AssetHandler
        , public AssetTypeInfoBus::Handler
        , protected Data::AssetBus::MultiHandler
    {
    public:
        AZ_COMPONENT(ScriptSystemComponent, "{EE57B2C2-4CF4-4CC1-9BA0-88F3BB298B2B}", Component)

        ScriptSystemComponent();
        ~ScriptSystemComponent() override;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // Component base
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Script System Bus
        ScriptContext*  AddContext(ScriptContext* context, int garbageCollectorStep = -1) override;
        /// Add a context with an ID, the system will handle the ownership of the context (delete it when removed and the system deactivates)
        ScriptContext*  AddContextWithId(ScriptContextId id) override;

        /// Removes a script context (without calling delete on it)
        bool RemoveContext(ScriptContext* context) override;
        /// Removes and deletes a script context.
        bool RemoveContextWithId(ScriptContextId id) override;
        /// Returns the script context that has been registered with the app, if there is one.
        ScriptContext* GetContext(ScriptContextId id = ScriptContextIds::DefaultScriptContextId) override;

        void GarbageCollect() override;
        void GarbageCollectStep(int numberOfSteps) override;

        bool Load(const Data::Asset<ScriptAsset>& asset, const char* mode, ScriptContextId id) override;
        ScriptLoadResult LoadAndGetNativeContext(const Data::Asset<ScriptAsset>& asset, const char* mode, ScriptContextId id) override;

        void ClearAssetReferences(Data::AssetId assetBaseId) override;

        void RestoreDefaultRequireHook(ScriptContextId id = ScriptContextIds::DefaultScriptContextId) override;
        void UseInMemoryRequireHook(const InMemoryScriptModules& modules, ScriptContextId id = ScriptContextIds::DefaultScriptContextId) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // SystemTickBus
        void OnSystemTick() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AssetHandler
        /// Called by the asset database to create a new asset. No loading should during this call
        Data::AssetPtr CreateAsset(const Data::AssetId& id, const Data::AssetType& type) override;
        /// Called by the asset database to perform actual asset load
        Data::AssetHandler::LoadResult LoadAssetData(
            const Data::Asset<Data::AssetData>& asset,
            AZStd::shared_ptr<Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
        /// Called by the asset database when an asset should be deleted.
        void DestroyAsset(Data::AssetPtr ptr) override;
        /// Return handled asset types
        void GetHandledAssetTypes(AZStd::vector<Data::AssetType>& assetTypes) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////////////////////////
        // AZ::AssetTypeInfoBus::Handler
        AZ::Data::AssetType GetAssetType() const override;
        const char* GetAssetTypeDisplayName() const override;
        const char* GetGroup() const override;
        const char* GetBrowserIcon() const override;
        AZ::Uuid GetComponentTypeId() const override;
        void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) override;
        bool CanCreateComponent(const AZ::Data::AssetId& assetId) const override;
        //////////////////////////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////////////////////////
        // AssetBus
        /// Called before an asset reload has started.
        void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;
        //////////////////////////////////////////////////////////////////////////////////////////////
        void LoadReadyAsset(Data::Asset<Data::AssetData> asset);

        /// \ref ComponentDescriptor::GetProvidedServices
        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
        /// \ref ComponentDescriptor::GetIncompatibleServices
        static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);
        /// \ref ComponentDescriptor::GetDependentServices
        static void GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent);
        /// \red ComponentDescriptor::Reflect
        static void Reflect(ReflectContext* reflection);

        struct LoadedScriptInfo
        {
            AZStd::unordered_set<AZStd::string> m_scriptNames;  ///< Aliases of the script table
            Data::Asset<ScriptAsset>            m_scriptAsset;  ///< Asset of the script required (may be empty if asset not require()'d)
            int                                 m_tableReference = -2; //< The reference to the table returned by the script (default -2 == LUA_NOREF)
        };
        int m_defaultGarbageCollectorSteps;

        struct ContextContainer
        {
            ScriptContext* m_context = nullptr;
            bool m_isOwner = true;
            int m_garbageCollectorSteps = 0;
            AZStd::unordered_map<Uuid, LoadedScriptInfo> m_loadedScripts;
            AZStd::unordered_map<Uuid, Data::Asset<ScriptAsset>> m_trackedScripts;
            AZStd::recursive_mutex m_loadedScriptsMutex;

            ContextContainer() = default;
            ContextContainer(const ContextContainer&) = delete;
            ContextContainer(ContextContainer&& rhs)
            {
                *this = AZStd::move(rhs);
            }

            ContextContainer& operator=(const ContextContainer&) = delete;
            ContextContainer& operator=(ContextContainer&& rhs)
            {
                m_context = rhs.m_context;
                m_isOwner = rhs.m_isOwner;
                m_garbageCollectorSteps = rhs.m_garbageCollectorSteps;

                {
                    AZStd::lock_guard<AZStd::recursive_mutex> myLock(m_loadedScriptsMutex);
                    AZStd::lock_guard<AZStd::recursive_mutex> rhsLock(rhs.m_loadedScriptsMutex);

                    m_loadedScripts.swap(rhs.m_loadedScripts);
                }
                return *this;
            }
        };

        ContextContainer*       GetContextContainer(ScriptContextId id);

        /// Default require hook installed on new contexts, looks for a compiled asset in the asset system corresponding to the module path and name.
        /// If found, loads the module if not done previously, leaves it on the stack, otherwise pushes string error.
        /// Additionally connects to the script id to reload the script if the script changes
        int DefaultRequireHook(lua_State* l, ScriptContext* context, const char* module);

        /// Custom require hook looks for a compiled asset by name of module that exists in memory only, specifically in m_inMemoryModules.
        /// Leaves the module or an error string on the stack.
        /// These modules are unloaded and deleted when RestoreDefaultRequireHook is called;
        int InMemoryRequireHook(lua_State* l, ScriptContext* context, const char* module);

        InMemoryScriptModules m_inMemoryModules;

        AZStd::vector<ContextContainer> m_contexts;
    };
}
