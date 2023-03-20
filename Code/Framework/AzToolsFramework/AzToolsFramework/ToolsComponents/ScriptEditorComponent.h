/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Script/ScriptProperty.h>
#include <AzFramework/Script/ScriptComponent.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorAssetReference.h>

namespace AzToolsFramework
{
    namespace Components
    {
        /**
        *
        */
        class ScriptEditorComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , private AZ::Data::AssetBus::Handler
        {
        public:
            AZ_EDITOR_COMPONENT(ScriptEditorComponent, "{b5fc8679-fa2a-4c7c-ac42-dcc279ea613a}")

            static void Reflect(AZ::ReflectContext* context);
            static bool DoComponentsMatch(const ScriptEditorComponent* thisComponent, const ScriptEditorComponent* otherComponent);

            ScriptEditorComponent() = default;
            ~ScriptEditorComponent() override;

            //////////////////////////////////////////////////////////////////////////
            // Component
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Editor Component
            void BuildGameEntity(AZ::Entity* gameEntity) override;
            void SetPrimaryAsset(const AZ::Data::AssetId& /*assetId*/) override;
            //////////////////////////////////////////////////////////////////////////

            const AZ::Data::Asset<AZ::ScriptAsset>& GetScript() const       { return m_scriptComponent.GetScript(); }
            void SetScript(const AZ::Data::Asset<AZ::ScriptAsset>& script);

            /// Resets the property to it's default value. (TODO)
            //void ResetProperty(const char* name);

            //////////////////////////////////////////////////////////////////////////
            // Data::AssetEvents
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            //////////////////////////////////////////////////////////////////////////

            void LaunchLuaEditor(const AZ::Data::AssetId&, const AZ::Data::AssetType&);
            // make sure internal script (m_scriptComponent.m_script) is set before loading
            void LoadScript();

        protected:
            ScriptEditorComponent(const ScriptEditorComponent&) = delete;
            struct ElementInfo
            {
                AZ::Uuid m_uuid;                    // Type uuid for the class field that should use this edit data.
                AZ::Edit::ElementData m_editData;   // Edit metadata (name, description, attribs, etc).
                bool m_isAttributeOwner;            // True if this ElementInfo owns the internal attributes. We can use a single
                                                    // ElementInfo for more than one class field, but only one owns the Attributes.
                float m_sortOrder; // Sort order of the property as defined by using the "order" attribute, by default the order is FLT_MAX which means alphabetical sort will be used
            };

            void LoadProperties();
            void LoadProperties(AZ::ScriptDataContext& sdc, AzFramework::ScriptPropertyGroup& group);
            void RemovedOldProperties(AzFramework::ScriptPropertyGroup& group);
            void SortProperties(AzFramework::ScriptPropertyGroup& group);

            bool LoadAttribute(AZ::ScriptDataContext& sdc, int valueIndex, const char* name, AZ::Edit::ElementData& ed, AZ::ScriptProperty* prop);
            bool LoadDefaultAsset(AZ::ScriptDataContext& sdc, int valueIndex, const char* name, AzFramework::ScriptPropertyGroup& group, ElementInfo& elementInfo);
            bool LoadDefaultEntityRef(AZ::ScriptDataContext& sdc, int valueIndex, const char* name, AzFramework::ScriptPropertyGroup& group, ElementInfo& elementInfo);

            void ClearDataElements();
            AZ::u32 ScriptHasChanged();

            bool LoadEnumValuesDouble(AZ::ScriptDataContext& sdc, int valueIndex, AZ::Edit::ElementData& ed);
            bool LoadEnumValuesString(AZ::ScriptDataContext& sdc, int valueIndex, AZ::Edit::ElementData& ed);

            const AZ::Edit::ElementData* GetDataElement(const void* element, const AZ::Uuid& typeUuid) const;

            static const AZ::Edit::ElementData* GetScriptPropertyEditData(const void* handlerPtr, const void* elementPtr, const AZ::Uuid& elementType);

            ////////////////////////////////////////////////////////////////////////////
            const char* CacheString(const char* str);
            AZStd::unordered_map<const void*, AZStd::string> m_cachedStrings; ///<- TODO Make editor global as we can chase them across multiple areas
            ////////////////////////////////////////////////////////////////////////////

            AZStd::unordered_map<const void*, ElementInfo> m_dataElements;

            AzFramework::ScriptComponent m_scriptComponent;
            AZ::Data::Asset<AZ::ScriptAsset> m_scriptAsset;

            AZStd::string m_customName;
        };
    } // namespace Component
} // namespace AzToolsFramework
