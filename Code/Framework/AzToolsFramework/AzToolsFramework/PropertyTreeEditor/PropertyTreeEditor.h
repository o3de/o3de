/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Uuid.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

namespace AzToolsFramework
{
    //! A class to easily access, enumerate and alter Edit Context properties of an object.
    //! The constructor automatically builds the InstanceDataHierarchy for the object provided
    //! and maps the property paths to the respective InstanceDataNode for easy access.
    //! Used to allow editor scripts to access and edit properties of Components, CryMaterials and more.
    class PropertyTreeEditor
    {
    public:
        AZ_RTTI(PropertyTreeEditor, "{704E727E-E941-47EE-9C70-065BC3AD66F3}")

        PropertyTreeEditor() = default; //!< Required to expose the class to Behavior Context; creates an empty object.
        explicit PropertyTreeEditor(void* pointer, AZ::TypeId typeId);
        virtual ~PropertyTreeEditor() = default;

        //! returns a list of properties in the form of 'property path'
        const AZStd::vector<AZStd::string> BuildPathsList() const;

        //! returns a list of properties in the form of 'property path (type name, [ShowChildrenOnly], [ReadOnly])'
        const AZStd::vector<AZStd::string> BuildPathsListWithTypes();

        //! returns the type of the property a 'property path' points to
        AZStd::string GetPropertyType(const AZStd::string_view propertyPath);

        //! toggles enforcement of skipping hidden node types such as ShowChildrenOnly and NotVisible
        void SetVisibleEnforcement(bool enforceVisiblity);

        using PropertyAccessOutcome = AZ::Outcome<AZStd::any, AZStd::string>;

        PropertyAccessOutcome GetProperty(const AZStd::string_view propertyPath);
        PropertyAccessOutcome SetProperty(const AZStd::string_view propertyPath, const AZStd::any& value);
        bool CompareProperty(const AZStd::string_view propertyPath, const AZStd::any& value);

        // Container API
        bool IsContainer(AZStd::string_view propertyPath) const;
        PropertyAccessOutcome GetContainerCount(AZStd::string_view propertyPath) const;
        PropertyAccessOutcome ResetContainer(AZStd::string_view propertyPath);
        PropertyAccessOutcome AppendContainerItem(AZStd::string_view propertyPath, const AZStd::any& value);
        PropertyAccessOutcome AddContainerItem(AZStd::string_view propertyPath, const AZStd::any& key, const AZStd::any& value);
        PropertyAccessOutcome RemoveContainerItem(AZStd::string_view propertyPath, const AZStd::any& key);
        PropertyAccessOutcome UpdateContainerItem(AZStd::string_view propertyPath, const AZStd::any& key, const AZStd::any& value);
        PropertyAccessOutcome GetContainerItem(AZStd::string_view propertyPath, const AZStd::any& key) const;

        // Property attributes access
        bool HasAttribute(AZStd::string_view propertyPath, AZStd::string_view attribute) const;

    private:
        //! Given a source (@fromType) and destination (@toType) rtti types it converts the
        //! value pointed by @sourceValuePtr if the rtti types are different, otherwise returns @sourceValuePtr.
        //! When @fromType != @toType then the converted value is placed in @convertedValue and the address to @convertedValue
        //! is returned.
        //! If the conversion is not supported it returns nullptr.
        const void* HandleTypeConversion(AZ::TypeId fromType, AZ::TypeId toType, const void* sourceValuePtr, AZStd::any& convertedValue);

        //! Always returns a pointer to @convertedValue if successful. Otherwise returns nullptr.
        template <typename V>
        const void* HandleTypeConversion(V fromValue, AZ::TypeId toType, AZStd::any& convertedValue);

        struct PropertyTreeEditorNode;
        AZStd::optional<PropertyTreeEditorNode*> FetchPropertyTreeEditorNode(AZStd::string_view propertyPath);
        bool IsPropertyTreeEditorNodeHidden(const PropertyTreeEditorNode& propertyTreeEditorNode) const;

        struct ChangeNotification
        {
            ChangeNotification(InstanceDataNode* node, AZ::Edit::Attribute* attribute)
                : m_attribute(attribute)
                , m_node(node)
            {
            }

            ChangeNotification() = default;

            InstanceDataNode* m_node = nullptr;
            AZ::Edit::Attribute* m_attribute = nullptr;
        };

        struct PropertyTreeEditorNode
        {
            PropertyTreeEditorNode() {}
            PropertyTreeEditorNode(AzToolsFramework::InstanceDataNode* nodePtr, AZStd::optional<AZStd::string> newName);
            PropertyTreeEditorNode(AzToolsFramework::InstanceDataNode* nodePtr, AZStd::optional<AZStd::string> newName, 
                const AZStd::string& parentPath, const AZStd::vector<ChangeNotification>& notifiers);
            PropertyTreeEditorNode(const AZ::TypeId& typeId, AzToolsFramework::InstanceDataNode* nodePtr, AZStd::optional<AZStd::string> newName);

            AZStd::optional<AZ::Attribute*> FindAttribute(AZ::Edit::AttributeId nameCrc) const;

            AZ::TypeId m_typeId = AZ::TypeId::CreateNull();
            AzToolsFramework::InstanceDataNode* m_nodePtr = nullptr;
            AZStd::optional<AZStd::string> m_newName = {};

            const AZStd::string m_parentPath;
            AZStd::vector<ChangeNotification> m_notifiers;
        };

        struct ContainerData final
        {
            ContainerData() = default;
            ~ContainerData() = default;

            const PropertyTreeEditorNode* m_propertyTreeEditorNode = nullptr;
            AZ::SerializeContext::IDataContainer* m_dataContainer = nullptr;
            const AZ::SerializeContext::ClassElement* m_valueElement = nullptr;
        };

        struct AssociatePairInfo final
        {
            AssociatePairInfo() = default;
            ~AssociatePairInfo() = default;

            const AZ::SerializeContext::ClassData* m_pairClass = nullptr;
            const AZ::SerializeContext::ClassElement* m_keyElement = nullptr;
            const AZ::SerializeContext::ClassElement* m_valueElement = nullptr;
        };

        bool SetSimpleAssetPath(const AZ::Data::AssetId& assetId, const PropertyTreeEditorNode& pteNode);
        bool SetAssetData(const AZ::Data::AssetId& assetId, const PropertyTreeEditorNode& pteNode);

        void HandleChangeNotifyAttribute(PropertyAttributeReader& reader, InstanceDataNode* node, AZStd::vector<ChangeNotification>& notifier);

        void PopulateNodeMap(AZStd::list<InstanceDataNode>& nodeList, const AZStd::string_view& previousPath = "");
        
        PropertyModificationRefreshLevel PropertyNotify(const PropertyTreeEditorNode* node, size_t optionalIndex = 0);

        AZStd::optional<ContainerData> FetchContainerData(AZStd::string_view propertyPath) const;
        AZStd::optional<AssociatePairInfo> MakeAssociatePair(const AZ::SerializeContext::ClassElement* containerElement) const;

        AZ::SerializeContext* m_serializeContext = nullptr;
        AZStd::shared_ptr<InstanceDataHierarchy> m_instanceDataHierarchy;
        AZStd::unordered_map<AZStd::string, PropertyTreeEditorNode> m_nodeMap;
        bool m_enforceVisiblity = false;
    };
}
