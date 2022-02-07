/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "InstanceDataHierarchy.h"
#include <AzCore/std/bind/bind.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/sort.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/Serialization/DynamicSerializableField.h>
#include <AzCore/Script/ScriptProperty.h>
#include <AzCore/Math/Crc.h>

#include <AzCore/Serialization/EditContextConstants.inl>
#include "ComponentEditor.hxx"
#include "PropertyEditorAPI.h"

namespace
{
    AZ::Edit::Attribute* FindAttributeInNode(const AzToolsFramework::InstanceDataNode* node, AZ::Edit::AttributeId attribId)
    {
        if (!node)
        {
            return nullptr;
        }

        AZ::Edit::Attribute* attr{};
        const AZ::Edit::ElementData* elementEditData = node->GetElementEditMetadata();
        if (elementEditData)
        {
            attr = elementEditData->FindAttribute(attribId);
        }

        // Attempt to look up the attribute on the node reflected class data.
        // This look up is done via AZ::SerializeContext::ClassData -> AZ::Edit::ClassData -> EditorData element
        if (!attr)
        {
            if (const AZ::SerializeContext::ClassData* classData = node->GetClassMetadata())
            {
                if (const auto* editClassData = classData->m_editData)
                {
                    if (const auto* classEditorData = editClassData->FindElementData(AZ::Edit::ClassElements::EditorData))
                    {
                        attr = classEditorData->FindAttribute(attribId);
                    }
                }
            }
        } 

        return attr;
    }

    template<typename ParameterType, typename ... Args> 
    ParameterType GetValueFromAttributeWithParams(const ParameterType& defaultValue, const AzToolsFramework::InstanceDataNode* node, AZ::Edit::Attribute* attribute, Args&& ... params)
    {
        if (!node || !attribute)
        {
            return defaultValue;
        }

        // Read the value from the attribute found.
        ParameterType value;
        for (size_t instIndex = 0; instIndex < node->GetNumInstances(); ++instIndex)
        {
            AzToolsFramework::PropertyAttributeReader reader(node->GetInstance(instIndex), attribute);
            if (reader.Read<ParameterType>(value, AZStd::forward<Args>(params) ...))
            {
                return value;
            }
        }

        return defaultValue;
    }

    AZStd::string GetIndexedStringFromAttribute(AzToolsFramework::InstanceDataNode* parentNode, AzToolsFramework::InstanceDataNode* attributeNode, AZ::Edit::AttributeId attribId, int siblingIndex)
    {
        AZ::Edit::Attribute* attribute = FindAttributeInNode(attributeNode, attribId);
        return GetValueFromAttributeWithParams<AZStd::string>("", parentNode, attribute, siblingIndex);
    }

    AZStd::string GetStringFromAttribute(const AzToolsFramework::InstanceDataNode* node, AZ::Edit::AttributeId attribId)
    {
        AZ::Edit::Attribute* attribute = FindAttributeInNode(node, attribId);
        return GetValueFromAttributeWithParams<AZStd::string>("", node, attribute);
    }

    AZStd::string GetDisplayLabel(AzToolsFramework::InstanceDataNode* node, int siblingIndex = 0)
    {
        if (!node)
        {
            return "";
        }

        AZStd::string label;

        // We want to check to see if a name override was provided by our first real
        // non-container parent.
        //
        // 'real' basically means something that is actually going to display.
        //
        // If we don't have a parent override, then we can take a look at applying out internal name override.        
        AzToolsFramework::InstanceDataNode* parentNode = node->GetParent();

        while (parentNode)
        {
            bool nonContainerNodeFound = !parentNode->GetClassMetadata() || !parentNode->GetClassMetadata()->m_container;

            if (nonContainerNodeFound)
            {
                const bool isSlicePushUI = false;
                AzToolsFramework::NodeDisplayVisibility visibility = CalculateNodeDisplayVisibility((*parentNode), isSlicePushUI);

                nonContainerNodeFound = (visibility == AzToolsFramework::NodeDisplayVisibility::Visible);
            }

            label = GetStringFromAttribute(parentNode, AZ::Edit::Attributes::ChildNameLabelOverride);
            if (!label.empty() || nonContainerNodeFound)
            {
                break;
            }

            parentNode = parentNode->GetParent();
        }

        // If our parent isn't controlling us. Fall back to whatever we want to be called
        if (label.empty())
        {
            label = GetStringFromAttribute(node, AZ::Edit::Attributes::NameLabelOverride);
        }

        if (label.empty())
        {
            parentNode = node;
            do
            {
                parentNode = parentNode->GetParent();
            } 
            while (parentNode && parentNode->GetClassMetadata() && parentNode->GetClassMetadata()->m_container);

            // trying to get per-item name provided by real parent
            label = GetIndexedStringFromAttribute(parentNode, node->GetParent(), AZ::Edit::Attributes::IndexedChildNameLabelOverride, siblingIndex);
        }

        return label;
    }
}

namespace AzToolsFramework
{
    //-----------------------------------------------------------------------------
    // InstanceDataNode
    //-----------------------------------------------------------------------------

    //! Read the value of the node.
    //! Clones the node's value and returns its address into valuePtr. ValuePtr will be overridden.
    bool InstanceDataNode::ReadRaw(void*& valuePtr, AZ::TypeId valueType)
    {
        void* firstInstanceCast = (GetSerializeContext()->DownCast(FirstInstance(), GetClassMetadata()->m_typeId, valueType));
        if (!firstInstanceCast)
        {
            AZ_Error("InstanceDataHierarchy", false, "Could not downcast from the value typeid %s to the instance typeid %s required.",
                GetClassMetadata()->m_typeId.ToString<AZStd::string>().c_str(),
                valueType.ToString<AZStd::string>().c_str());
            return false;
        }

        valuePtr = firstInstanceCast;

        // check all instance values are the same
        return std::all_of(m_instances.begin(), m_instances.end(), [this, valueType, firstInstanceCast](void* instance)
        {
            void* instanceCast = (GetSerializeContext()->DownCast(instance, GetClassMetadata()->m_typeId, valueType));
            
            if (!instanceCast)
            {
                AZ_Error("InstanceDataHierarchy", false, "Could not downcast from the value typeid %s to the instance typeid %s required.",
                    GetClassMetadata()->m_typeId.ToString<AZStd::string>().c_str(),
                    valueType.ToString<AZStd::string>().c_str());
                return false;
            }

            return (GetClassMetadata()->m_serializer && GetClassMetadata()->m_serializer->CompareValueData(instanceCast, firstInstanceCast)) ||
                    (!GetClassMetadata()->m_serializer && instanceCast == firstInstanceCast);
        });
    }

    //! Write the value into the node.
    void InstanceDataNode::WriteRaw(const void* valuePtr, AZ::TypeId valueType)
    {
        for (void* instance : m_instances)
        {
            // If type does not match, bail
            if (valueType != GetClassMetadata()->m_typeId)
            {
                AZ_Error("InstanceDataHierarchy", false, "Could not downcast from the value typeid %s to the instance typeid %s required.",
                    GetClassMetadata()->m_typeId.ToString<AZStd::string>().c_str(),
                    valueType.ToString<AZStd::string>().c_str());
                continue;
            }
            
            if (valueType == GetClassMetadata()->m_typeId)
            {
                GetSerializeContext()->CloneObjectInplace(instance, valuePtr, GetClassMetadata()->m_typeId);
            }
        }
    }

    void* InstanceDataNode::GetInstance(size_t idx) const
    {
        void* ptr = m_instances[idx];
        if (m_classElement)
        {
            ptr = AZ::Utils::ResolvePointer(ptr, *m_classElement, *m_context);
        }
        return ptr;
    }

    //-----------------------------------------------------------------------------
    void** InstanceDataNode::GetInstanceAddress(size_t idx) const
    {
        AZ_Assert(m_classElement && (m_classElement->m_flags & AZ::SerializeContext::ClassElement::FLG_POINTER), "You can not call GetInstanceAddress() on a node that is not of a pointer type!");
        return reinterpret_cast<void**>(m_instances[idx]);
    }

    //-----------------------------------------------------------------------------
    bool InstanceDataNode::CreateContainerElement(const SelectClassCallback& selectClass, const FillDataClassCallback& fillData)
    {
        AZ::SerializeContext::IDataContainer* container = m_classData->m_container;
        AZ_Assert(container, "This node is NOT a container node!");
        const AZ::SerializeContext::ClassElement* containerClassElement = container->GetElement(container->GetDefaultElementNameCrc());

        AZ_Assert(containerClassElement != nullptr, "We should have a valid default element in the container, otherwise we don't know what elements to make!");
        if (!containerClassElement)
        {
            return false;
        }

        if (containerClassElement->m_flags & AZ::SerializeContext::ClassElement::FLG_POINTER)
        {
            // TEMP until it's safe to pass 0 as type id
            const AZ::Uuid& baseTypeId = containerClassElement->m_azRtti ? containerClassElement->m_azRtti->GetTypeId() : AZ::AzTypeInfo<int>::Uuid();
            // ask the GUI and use to create one (if there is choice)
            const AZ::SerializeContext::ClassData* classData = selectClass(containerClassElement->m_typeId, baseTypeId, m_context);

            if (classData && classData->m_factory)
            {
                for (size_t i = 0; i < m_instances.size(); ++i)
                {
                    // reserve entry in the container
                    void* dataAddress = container->ReserveElement(GetInstance(i), containerClassElement);
                    // create entry
                    void* newDataAddress = classData->m_factory->Create(classData->m_name);

                    AZ_Assert(newDataAddress, "Faliled to create new element for the continer!");
                    // cast to base type (if needed)
                    void* basePtr = m_context->DownCast(newDataAddress, classData->m_typeId, containerClassElement->m_typeId, classData->m_azRtti, containerClassElement->m_azRtti);
                    AZ_Assert(basePtr != nullptr, "Can't cast container element %s to %s, make sure classes are registered in the system and not generics!", classData->m_name, containerClassElement->m_name);
                    *reinterpret_cast<void**>(dataAddress) = basePtr; // store the pointer in the class
                    /// Store the element in the container
                    container->StoreElement(GetInstance(i), dataAddress);
                }
                return true;
            }
        }
        else if (containerClassElement->m_typeId == AZ::SerializeTypeInfo<AZ::DynamicSerializableField>::GetUuid())
        {
            // Dynamic serializable fields are capable of wrapping any type. Each one within a container can technically contain
            // an entirely different type from the others. We're going to assume that we're getting here via 
            // ScriptPropertyGenericClassArray and that it strictly uses one type. 

            const AZ::SerializeContext::ClassData* classData = m_context->FindClassData(AZ::SerializeTypeInfo<AZ::DynamicSerializableField>::GetUuid());
            const AZ::Edit::ElementData* element = m_parent->m_classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
            if (element)
            {
                // Grab the AttributeMemberFunction used to get the Uuid type of the element wrapped by the DynamicSerializableField
                AZ::Edit::Attribute* assetTypeAttribute = element->FindAttribute(AZ::Edit::Attributes::DynamicElementType);
                if (assetTypeAttribute)
                {
                    //Invoke the function we just grabbed and pull the class data based on that Uuid
                    AZ_Assert(m_parent->GetNumInstances() <= 1, "ScriptPropertyGenericClassArray should not have more than one DynamicSerializableField vector");
                    AZ::AttributeReader elementTypeIdReader(m_parent->GetInstance(0), assetTypeAttribute);
                    AZ::Uuid dynamicClassUuid;
                    if (elementTypeIdReader.Read<AZ::Uuid>(dynamicClassUuid))
                    {
                        const AZ::SerializeContext::ClassData* dynamicClassData = m_context->FindClassData(dynamicClassUuid);

                        //Construct a new element based on the Uuid we just grabbed and wrap it in a DynamicSerializeableField for storage
                        if (classData && classData->m_factory &&
                            dynamicClassData && dynamicClassData->m_factory)
                        {
                            for (size_t i = 0; i < m_instances.size(); ++i)
                            {
                                // Reserve entry in the container
                                void* dataAddress = container->ReserveElement(GetInstance(i), containerClassElement);

                                // Create DynamicSerializeableField entry
                                void* newDataAddress = classData->m_factory->Create(classData->m_name);
                                AZ_Assert(newDataAddress, "Faliled to create new element for the continer!");

                                // Create dynamic element and populate entry with it
                                AZ::DynamicSerializableField* dynamicFieldDesc = reinterpret_cast<AZ::DynamicSerializableField*>(newDataAddress);
                                void* newDynamicData = dynamicClassData->m_factory->Create(dynamicClassData->m_name);
                                dynamicFieldDesc->m_data = newDynamicData;
                                dynamicFieldDesc->m_typeId = dynamicClassData->m_typeId;

                                /// Store the entry in the container
                                *reinterpret_cast<AZ::DynamicSerializableField*>(dataAddress) = *dynamicFieldDesc;
                                container->StoreElement(GetInstance(i), dataAddress);
                            }

                            return true;
                        }
                    }
                }
            }
        }
        else
        {
            for (size_t i = 0; i < m_instances.size(); ++i)
            {
                // check capacity of container before attempting to reserve an element
                if (container->IsFixedCapacity() && !container->IsSmartPointer() && container->Size(GetInstance(i)) >= container->Capacity(GetInstance(i)))
                {
                    AZ_Warning("Serializer", false, "Cannot add additional entries to the container as it is at its capacity of %zu", container->Capacity(GetInstance(i)));
                    return false;
                }
                // reserve entry in the container
                void* dataAddress = container->ReserveElement(GetInstance(i), containerClassElement);
                bool isAssociative = false;
                ReadAttribute(AZ::Edit::Attributes::ShowAsKeyValuePairs, isAssociative, true);
                bool noDefaultData = isAssociative || (containerClassElement->m_flags & AZ::SerializeContext::ClassElement::FLG_NO_DEFAULT_VALUE) != 0;

                if (!dataAddress || !fillData(dataAddress, containerClassElement, noDefaultData, m_context) && noDefaultData) // fill default data
                {
                    return false;
                }

                /// Store the element in the container
                container->StoreElement(GetInstance(i), dataAddress);
            }

            return true;
        }

        return false;
    }

    bool InstanceDataNode::ChildMatchesAddress(const InstanceDataNode::Address& elementAddress) const
    {
        if (elementAddress.empty())
        {
            return false;
        }

        for (const InstanceDataNode& child : m_children)
        {
            if (elementAddress == child.ComputeAddress())
            {
                return true;
            }
            
            if (child.ChildMatchesAddress(elementAddress))
            {
                return true;
            }
        }

        return false;
    }

    //-----------------------------------------------------------------------------
    void InstanceDataNode::MarkNewVersusComparison()
    {
        m_comparisonFlags =  static_cast<AZ::u32>(ComparisonFlags::New);
    }

    //-----------------------------------------------------------------------------
    void InstanceDataNode::MarkDifferentVersusComparison()
    {
        m_comparisonFlags =  static_cast<AZ::u32>(ComparisonFlags::Differs);
    }
    
    //-----------------------------------------------------------------------------
    void InstanceDataNode::MarkRemovedVersusComparison()
    {
        m_comparisonFlags &= ~static_cast<AZ::u32>(ComparisonFlags::New);
        m_comparisonFlags =  static_cast<AZ::u32>(ComparisonFlags::Removed);
    }

    //-----------------------------------------------------------------------------
    void InstanceDataNode::ClearComparisonData()
    {
        m_comparisonFlags = static_cast<AZ::u32>(ComparisonFlags::None);
        m_comparisonNode = nullptr;
    }

    //-----------------------------------------------------------------------------
    bool InstanceDataNode::IsNewVersusComparison() const
    {
        return 0 != (m_comparisonFlags & static_cast<AZ::u32>(ComparisonFlags::New));
    }

    //-----------------------------------------------------------------------------
    bool InstanceDataNode::IsDifferentVersusComparison() const
    {
        return 0 != (m_comparisonFlags & static_cast<AZ::u32>(ComparisonFlags::Differs));
    }

    //-----------------------------------------------------------------------------
    bool InstanceDataNode::IsRemovedVersusComparison() const
    {
        return 0 != (m_comparisonFlags & static_cast<AZ::u32>(ComparisonFlags::Removed));
    }

    //-----------------------------------------------------------------------------
    const InstanceDataNode* InstanceDataNode::GetComparisonNode() const
    {
        return m_comparisonNode;
    }

    bool InstanceDataNode::HasChangesVersusComparison(bool includeChildren) const
    {
        if (m_comparisonFlags)
        {
            return true;
        }
        
        if (includeChildren)
        {
            for (auto child : m_children)
            {
                if (child.HasChangesVersusComparison(includeChildren))
                {
                    return true;
                }
            }
        }

        return false;
    }

    //-----------------------------------------------------------------------------
    InstanceDataNode::Address InstanceDataNode::ComputeAddress() const
    {
        Address addressStack;
        addressStack.reserve(32);

        addressStack.push_back(m_identifier);

        InstanceDataNode* parent = m_parent;
        while (parent)
        {
            addressStack.push_back(parent->m_identifier);
            parent = parent->m_parent;
        }

        return addressStack;
    }

    AZ::Edit::Attribute* InstanceDataNode::FindAttribute(AZ::Edit::AttributeId nameCrc) const
    {
        AZ::Edit::Attribute* attribute = nullptr;

        // Edit Data > Class Element > Class Data in terms of specificity to what we're editing, so prioritize their attributes accordingly
        if (m_elementEditData) 
        {
            attribute = m_elementEditData->FindAttribute(nameCrc); 
        }
        if (!attribute && m_classElement)
        {
            attribute = m_classElement->FindAttribute(nameCrc);
        }
        if (!attribute && m_classData)
        {
            attribute = m_classData->FindAttribute(nameCrc);
        }

        return attribute;
    }

    //-----------------------------------------------------------------------------
    // InstanceDataHierarchy
    //-----------------------------------------------------------------------------

    InstanceDataHierarchy::InstanceDataHierarchy()
        : m_curParentNode(nullptr)
        , m_isMerging(false)
        , m_nodeDiscarded(false)
        , m_valueComparisonFunction(DefaultValueComparisonFunction)
    {
    }

    //-----------------------------------------------------------------------------
    void InstanceDataHierarchy::AddRootInstance(void* instance, const AZ::Uuid& classId)
    {
        m_rootInstances.emplace_back(instance, classId);
    }

    //-----------------------------------------------------------------------------
    void InstanceDataHierarchy::AddComparisonInstance(void* instance, const AZ::Uuid& classId)
    {
        m_comparisonInstances.emplace_back(instance, classId);
    }

    //-----------------------------------------------------------------------------
    bool InstanceDataHierarchy::ContainsRootInstance(const void* instance) const
    {
        for (InstanceDataArray::const_iterator it = m_rootInstances.begin(); it != m_rootInstances.end(); ++it)
        {
            if (it->m_instance == instance)
            {
                return true;
            }
        }
        return false;
    }

    //-----------------------------------------------------------------------------
    void InstanceDataHierarchy::SetBuildFlags(AZ::u8 flags)
    {
        m_buildFlags = flags;
    }

    void InstanceDataHierarchy::EnumerateUIElements(InstanceDataNode* node, DynamicEditDataProvider dynamicEditDataProvider)
    {
        for (InstanceDataNode& child : node->m_children)
        {
            EnumerateUIElements(&child, dynamicEditDataProvider);
        }

        AZ::Edit::ClassData* nodeEditData = node->GetClassMetadata() ? node->GetClassMetadata()->m_editData : nullptr;
        if (nodeEditData)
        {
            const AZ::Edit::ElementData* groupData = nullptr;

            // Keep track of where to insert ourselves in the child list
            m_childIndexOverride = 0;
            auto nodeIt = node->m_children.begin();

            for (auto& element : nodeEditData->m_elements)
            {
                if (element.m_elementId == AZ::Edit::ClassElements::Group)
                {
                    groupData = (element.m_description && element.m_description[0]) ? &element : nullptr;
                    continue;
                }

                // If we're looking at element data that's part of the child list, keep track of the index for adjacent UIElement insertion
                // Children appear in the order specified by m_elements, so we can scan as we go
                if (nodeIt != node->m_children.end() && nodeIt->m_classElement->m_editData == &element)
                {
                    ++m_childIndexOverride;
                    ++nodeIt;
                }

                //create ui elements in their relative edit context positions
                if (element.m_elementId == AZ::Edit::ClassElements::UIElement)
                {
                    AZ::Edit::Attribute* attribute = element.FindAttribute(AZ::Edit::Attributes::AcceptsMultiEdit);
                    bool acceptsMultiEdit = GetValueFromAttributeWithParams<bool>(false, node, attribute);

                    size_t numInstances = node->GetNumInstances();
                    if (numInstances == 1 || acceptsMultiEdit)
                    {
                        // For every UIElement, generate an InstanceDataNode pointed at our instance with the corresponding attributes
                        for (size_t i = 0; i < numInstances; ++i)
                        {
                            m_supplementalElementData.push_back();
                            auto& serializeFieldElement = m_supplementalElementData.back();

                            serializeFieldElement.m_name = element.m_description;
                            serializeFieldElement.m_nameCrc = AZ::Crc32(element.m_description);
                            serializeFieldElement.m_azRtti = nullptr;
                            serializeFieldElement.m_dataSize = sizeof(void*);
                            serializeFieldElement.m_offset = 0;
                            serializeFieldElement.m_typeId = AZ::Uuid::CreateNull();
                            serializeFieldElement.m_editData = &element;
                            serializeFieldElement.m_flags = AZ::SerializeContext::ClassElement::FLG_UI_ELEMENT;

                            m_curParentNode = node;
                            m_isMerging = i > 0; // Ensure we always add a node for the first instance, then compare
                            BeginNode(node->GetInstance(i), nullptr, &serializeFieldElement, dynamicEditDataProvider);
                            m_curParentNode->m_groupElementData = groupData;
                            EndNode();
                        }
                        ++m_childIndexOverride;
                    }
                }
            }

            m_childIndexOverride = -1;
            m_curParentNode = nullptr;
        }
    }

    //-----------------------------------------------------------------------------
    void InstanceDataHierarchy::Build(AZ::SerializeContext* sc, unsigned int accessFlags, DynamicEditDataProvider dynamicEditDataProvider, ComponentEditor* editorParent)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        AZ_Assert(sc, "sc can't be NULL!");
        AZ_Assert(m_rootInstances.size() > 0, "No root instances have been added to this hierarchy!");

        m_curParentNode = nullptr;
        m_isMerging = false;
        m_instances.clear();
        m_children.clear();
        m_matched = true;
        m_nodeDiscarded = false;
        m_context = sc;
        m_comparisonHierarchies.clear();
        m_supplementalEditData.clear();

        AZ::SerializeContext::EnumerateInstanceCallContext callContext(
            AZStd::bind(&InstanceDataHierarchy::BeginNode, this, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3, dynamicEditDataProvider),
            AZStd::bind(&InstanceDataHierarchy::EndNode, this),
            sc,
            AZ::SerializeContext::ENUM_ACCESS_FOR_READ,
            nullptr
        );

        sc->EnumerateInstanceConst(
            &callContext
            , m_rootInstances[0].m_instance
            , m_rootInstances[0].m_classId
            , nullptr
            , nullptr
        );

        for (size_t i = 1; i < m_rootInstances.size(); ++i)
        {
            m_curParentNode = nullptr;
            m_isMerging = true;
            m_matched = false;
            sc->EnumerateInstanceConst(
                &callContext
                , m_rootInstances[i].m_instance
                , m_rootInstances[i].m_classId
                , nullptr
                , nullptr
            );
        }

        EnumerateUIElements(this, dynamicEditDataProvider);

        // Fixup our container edit data first, as we may specifically affect comparison data
        FixupEditData(this, 0);
        bool dataIdentical = RefreshComparisonData(accessFlags, dynamicEditDataProvider);

        if (editorParent)
        {
            editorParent->SetComponentOverridden(!dataIdentical);
        }
    }

    namespace InstanceDataHierarchyHelper
    {
        template <class T>
        bool GetEnumStringRepresentation(AZStd::string& value, const AZ::Edit::ElementData* data, void* instance, const AZ::Uuid& storageTypeId)
        {
            if (storageTypeId == azrtti_typeid<T>())
            {
                for (const AZ::AttributePair& attributePair : data->m_attributes)
                {
                    PropertyAttributeReader reader(instance, attributePair.second);
                    AZ::Edit::EnumConstant<T> enumPair;
                    if (reader.Read<AZ::Edit::EnumConstant<T>>(enumPair))
                    {
                        T* enumValue = reinterpret_cast<T*>(instance);
                        if (enumPair.m_value == *enumValue)
                        {
                            value = enumPair.m_description;
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        // Try GetEnumStringRepresentation<Type> on all of the specified types 
        template <class T1, class T2, class... TRest>
        bool GetEnumStringRepresentation(AZStd::string& value, const AZ::Edit::ElementData* data, void* instance, const AZ::Uuid& storageTypeId)
        {
            return GetEnumStringRepresentation<T1>(value, data, instance, storageTypeId) || GetEnumStringRepresentation<T2, TRest...>(value, data, instance, storageTypeId);
        }

        bool GetValueStringRepresentation(const InstanceDataNode* node, AZStd::string& value)
        {
            const AZ::SerializeContext::ClassData* classData = node->GetClassMetadata();
            if (!node || !classData)
            {
                return false;
            }

            // Check to see if the class has a registered ConciseEditorStringRepresentation
            AZStd::string result = GetStringFromAttribute(node, AZ::Edit::Attributes::ConciseEditorStringRepresentation);
            if (!result.empty())
            {
                value = result;
                return true;
            }

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            // Get our enum string value from the edit context if we're actually an enum
            AZ::Uuid enumId;
            if (serializeContext && node->ReadAttribute(AZ::Edit::InternalAttributes::EnumType, enumId))
            {
                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    const AZ::Edit::ElementData* data = editContext->GetEnumElementData(enumId);
                    if (data)
                    {
                        // Check all underlying enum storage types
                        if (GetEnumStringRepresentation<
                            AZ::u8, AZ::u16, AZ::u32, AZ::u64,
                            AZ::s8, AZ::s16, AZ::s32, AZ::s64>(value, data, node->FirstInstance(), node->GetClassMetadata()->m_typeId))
                        {
                            return true;
                        }
                    }
                }
            }

            // Just use our underlying AZStd::string if we're a string
            if (classData->m_typeId == azrtti_typeid<AZStd::string>())
            {
                value = *reinterpret_cast<AZStd::string*>(node->FirstInstance());
                return true;
            }

            // Fall back on using our serializer's DataToText
            const AZ::SerializeContext::ClassElement* elementData = node->GetElementMetadata();
            if (elementData)
            {
                if (auto& serializer = classData->m_serializer)
                {
                    AZ::IO::MemoryStream memStream(node->FirstInstance(), 0, elementData->m_dataSize);
                    AZStd::vector<char> buffer;
                    AZ::IO::ByteContainerStream<AZStd::vector<char>> outStream(&buffer);
                    serializer->DataToText(memStream, outStream, false);
                    value = AZStd::string(buffer.data(), buffer.size());
                    return !value.empty();
                }
            }

            return false;
        }
    }

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    void InstanceDataHierarchy::FixupEditData(InstanceDataNode* node, int siblingIdx)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        bool mergeElementEditData = node->m_classElement && node->m_classElement->m_editData && node->GetElementEditMetadata() != node->m_classElement->m_editData;
        bool mergeContainerEditData = node->m_parent && node->m_parent->m_classData->m_container && node->m_parent->GetElementEditMetadata() && (node->m_classElement->m_flags & AZ::SerializeContext::ClassElement::FLG_POINTER) == 0;

        bool showAsKeyValue = false;
        if (!(m_buildFlags & Flags::IgnoreKeyValuePairs))
        {
            // Default to showing as key values if we're an associative interface, but always respect ShowAsKeyValuePairs if it's specified
            showAsKeyValue = node->m_parent && node->m_parent->m_classData->m_container && node->m_parent->m_classData->m_container->GetAssociativeContainerInterface();
            node->ReadAttribute(AZ::Edit::Attributes::ShowAsKeyValuePairs, showAsKeyValue);
        }

        if (mergeElementEditData || mergeContainerEditData || showAsKeyValue)
        {
            AZStd::string label;
            
            if (!showAsKeyValue)
            {
                label = GetDisplayLabel(node, siblingIdx);
            }

            // Grab a copy of our instances if we might need to move them to a key/value attribute.
            // We need a copy because the current node may well be replaced in its entirety by one of its children.
            AZStd::vector<void*> elementInstances;
            if (showAsKeyValue)
            {
                elementInstances = node->m_instances;
            }

            // If our node is a pair, respect showAsKeyValue and promote the value data element with a label matching our instance's string representation
            if (showAsKeyValue && node->m_children.size() == 2)
            {
                // Make sure we can get a valid string representation before doing the conversion
                if (label.empty())
                {
                    showAsKeyValue = InstanceDataHierarchyHelper::GetValueStringRepresentation(&node->m_children.front(), label);
                }

                if (!showAsKeyValue)
                {
                    int i = 0;
                    for (auto it = node->m_children.begin(); it != node->m_children.end(); ++it, ++i)
                    {
                        InstanceDataNode& childNode = *it;
                        m_supplementalEditData.push_back();
                        AZ::Edit::ElementData* editData = &m_supplementalEditData.back().m_editElementData;
                        if (childNode.GetElementEditMetadata())
                        {
                            *editData = *node->GetElementEditMetadata();
                        }

                        const char* labelText;
                        if (i == 0)
                        {
                            labelText = "Key";
                        }
                        else
                        {
                            labelText = "Value";
                        }
                        m_supplementalEditData.back().m_displayLabel = AZStd::string::format("%s<%s>", labelText, childNode.m_classData->m_name);
                        editData->m_description = nullptr;
                        editData->m_name = m_supplementalEditData.back().m_displayLabel.c_str();
                        childNode.m_elementEditData = editData;
                    }
                }
            }

            m_supplementalEditData.push_back();
            AZ::Edit::ElementData* editData = &m_supplementalEditData.back().m_editElementData;
            if (node->GetElementEditMetadata())
            {
                *editData = *node->GetElementEditMetadata();
            }
            node->m_elementEditData = editData;

            // Flag our new key node with our original instances for any future element modification
            if (showAsKeyValue)
            {
                node->m_identifier = AZ::Crc32(label.c_str());

                auto attribute = aznew AZ::AttributeContainerType<AZStd::vector<void*>>(AZStd::move(elementInstances));
                m_supplementalEditData.back().m_attributes.emplace_back(attribute); // Ensure the attribute gets cleaned up
                editData->m_attributes.emplace_back(
                    AZ::Edit::InternalAttributes::ElementInstances, 
                    attribute);
            }

            if (mergeElementEditData)
            {
                for (AZ::Edit::AttributeArray::const_iterator elemAttrIter = node->m_classElement->m_editData->m_attributes.begin(); elemAttrIter != node->m_classElement->m_editData->m_attributes.end(); ++elemAttrIter)
                {
                    AZ::Edit::AttributeArray::iterator editAttrIter = editData->m_attributes.begin();
                    for (; editAttrIter != editData->m_attributes.end(); ++editAttrIter)
                    {
                        if (elemAttrIter->first == editAttrIter->first)
                        {
                            break;
                        }
                    }
                    if (editAttrIter == editData->m_attributes.end())
                    {
                        editData->m_attributes.push_back(*elemAttrIter);
                    }
                }
            }

            if (mergeContainerEditData || !label.empty())
            {
                m_supplementalEditData.back().m_displayLabel = label.empty() ? AZStd::string::format("[%d]", siblingIdx) : label;
                editData->m_description = nullptr;
                if (!editData->m_name)
                {
                    editData->m_name = m_supplementalEditData.back().m_displayLabel.c_str();
                }

                if (mergeContainerEditData)
                {
                    InstanceDataNode* container = node->m_parent;
                    for (AZ::Edit::AttributeArray::const_iterator containerAttrIter = container->GetElementEditMetadata()->m_attributes.begin(); containerAttrIter != container->GetElementEditMetadata()->m_attributes.end(); ++containerAttrIter)
                    {
                        if (!containerAttrIter->second->m_describesChildren)
                        {
                            continue;
                        }

                        AZ::Edit::AttributeArray::iterator editAttrIter = editData->m_attributes.begin();
                        for (; editAttrIter != editData->m_attributes.end(); ++editAttrIter)
                        {
                            if (containerAttrIter->first == editAttrIter->first)
                            {
                                break;
                            }
                        }
                        if (editAttrIter == editData->m_attributes.end())
                        {
                            editData->m_attributes.push_back(*containerAttrIter);
                        }
                    }
                }
            }
        }

        int childIdx = 0;
        for (NodeContainer::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
        {
            FixupEditData(&(*it), childIdx++);
        }
    }

    //-----------------------------------------------------------------------------
    bool InstanceDataHierarchy::BeginNode(void* ptr, const AZ::SerializeContext::ClassData* classData, const AZ::SerializeContext::ClassElement* classElement, DynamicEditDataProvider dynamicEditDataProvider)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        const AZ::Edit::ElementData* elementEditData = nullptr;

        if (classElement)
        {
            // Find the correct element edit data to use
            elementEditData = classElement->m_editData;
            if (dynamicEditDataProvider)
            {
                void* objectPtr = AZ::Utils::ResolvePointer(ptr, *classElement, *m_context);
                const AZ::Edit::ElementData* labelData = dynamicEditDataProvider(objectPtr, classData);
                if (labelData)
                {
                    elementEditData = labelData;
                }
            }
            else if (!m_editDataOverrides.empty())
            {
                const EditDataOverride& editDataOverride = m_editDataOverrides.back();

                void* objectPtr = AZ::Utils::ResolvePointer(ptr, *classElement, *m_context);
                void* overridingInstance = editDataOverride.m_overridingInstance;

                const AZ::SerializeContext::ClassElement* overridingElementData = editDataOverride.m_overridingNode->GetElementMetadata();
                if (overridingElementData)
                {
                    overridingInstance = AZ::Utils::ResolvePointer(overridingInstance, *overridingElementData, *m_context);
                }

                if (classData)
                {
                    const AZ::Edit::ElementData* useData = editDataOverride.m_override(overridingInstance, objectPtr, classData->m_typeId);
                    if (useData)
                    {
                        elementEditData = useData;
                    }
                }
            }
        }

        InstanceDataNode* node = nullptr;
        // Extra steps need to be taken when we are merging
        if (m_isMerging)
        {
            if (!m_curParentNode)
            {
                // Only accept root instances that are of the same type
                if (classData && classData->m_typeId == m_classData->m_typeId)
                {
                    node = this;
                }
            }
            else
            {
                // Search through the parent's class elements to find a match
                for (NodeContainer::iterator it = m_curParentNode->m_children.begin(); it != m_curParentNode->m_children.end(); ++it)
                {
                    InstanceDataNode* subElement = &(*it);
                    if (!subElement->m_matched &&
                        subElement->m_classElement->m_nameCrc == classElement->m_nameCrc &&
                        subElement->m_classData == classData &&
                        (subElement->m_elementEditData == elementEditData ||
                        (subElement->m_elementEditData && elementEditData &&
                            subElement->m_elementEditData->m_name && elementEditData->m_name &&
                            azstricmp(subElement->m_elementEditData->m_name, elementEditData->m_name) == 0)))
                    {
                        node = subElement;
                        break;
                    }
                }
            }

            if (node)
            {
                AZ::Edit::Attribute* attribute = FindAttributeInNode(node, AZ::Edit::Attributes::AcceptsMultiEdit);
                bool acceptsMultiEdit = GetValueFromAttributeWithParams<bool>(true, node, attribute);
                if (!acceptsMultiEdit)
                {
                    // Reject the node and everything under it if it doesn't support multiple instances
                    m_nodeDiscarded = true;
                    return false;
                }

                // Add the new instance pointer to the list of mapped instances
                node->m_instances.push_back(ptr);
                // Flag the node as already matched for this pass.
                node->m_matched = true;

                // prepare the node's children for matching
                // we set them all to false so that when we unwind the depth-first enumeration,
                // any unmatched nodes can be discarded.
                for (NodeContainer::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
                {
                    it->m_matched = false;
                }
            }
            else
            {
                // Reject the node and everything under it
                m_nodeDiscarded = true;
                return false;
            }
        }
        else
        {
            // Not merging, just add anything being enumerated to the hierarchy.
            if (!m_curParentNode)
            {
                node = this;
            }
            else
            {
                if (m_childIndexOverride >= 0)
                {
                    auto position = m_curParentNode->m_children.begin();
                    AZStd::advance(position, m_childIndexOverride);

                    auto iterator = m_curParentNode->m_children.insert(position);
                    node = &(*iterator);
                }
                else
                {
                    m_curParentNode->m_children.push_back();
                    node = &m_curParentNode->m_children.back();
                }
            }
            node->m_instances.push_back(ptr);
            node->m_classData = classData;

            // ClassElement pointers for DynamicSerializableFields are temporaries, so we need
            // to maintain it locally.
            if (classElement && (classElement->m_flags & AZ::SerializeContext::ClassElement::FLG_DYNAMIC_FIELD))
            {
                m_supplementalElementData.push_back(*classElement);
                classElement = &m_supplementalElementData.back();
            }

            node->m_classElement = classElement;
            node->m_elementEditData = elementEditData;
            node->m_parent = m_curParentNode;
            node->m_root = this;
            node->m_context = m_context;

            // Compute node's identifier, which is used to compute full address within a hierarchy.
            if (m_curParentNode)
            {
                if (m_curParentNode->GetClassMetadata() && m_curParentNode->GetClassMetadata()->m_container)
                {
                    if (classData)
                    {
                        // Within a container, use persistentId if available, otherwise use a CRC of name and container index.
                        AZ::SerializeContext::ClassPersistentId persistentId = classData->GetPersistentId(*m_context);
                        if (persistentId)
                        {
                            node->m_identifier = static_cast<Identifier>(persistentId(AZ::Utils::ResolvePointer(ptr, *classElement, *m_context)));
                        }
                        else
                        {
                            AZStd::string indexedName = AZStd::string::format("%s_%zu", classData->m_name, m_curParentNode->m_children.size() - 1);
                            node->m_identifier = static_cast<Identifier>(AZ::Crc32(indexedName.c_str()));
                        }
                    }
                }
                else
                {
                    // Not in a container, just use crc.
                    node->m_identifier = classElement->m_nameCrc;
                }
            }
            else if (classData)
            {
                // Root level, use crc of class type.
                node->m_identifier = AZ_CRC(classData->m_name);
            }

            AZ_Assert(node->m_identifier != InvalidIdentifier, "InstanceDataNode has an invalid identifier. Addressing will not be valid.");
        }

        // if our data contains dynamic edit data handler, push it on the stack
        if (classData && classData->m_editData && classData->m_editData->m_editDataProvider)
        {
            m_editDataOverrides.push_back();
            m_editDataOverrides.back().m_override = classData->m_editData->m_editDataProvider;
            m_editDataOverrides.back().m_overridingInstance = ptr;
            m_editDataOverrides.back().m_overridingNode = node;
        }

        // Resolve the group metadata for this element
        AZ::Edit::ClassData* parentEditData = (node->GetParent() && node->GetParent()->GetClassMetadata()) ? node->GetParent()->GetClassMetadata()->m_editData : nullptr;
        if (parentEditData)
        {
            // Dig through the class elements in the parent structure looking for groups. Apply the last group created
            // to this node
            const AZ::Edit::ElementData* groupData = nullptr;
            for (const AZ::Edit::ElementData& elementData : parentEditData->m_elements)
            {
                // this element matches this node
                if ((node->m_elementEditData == &elementData) && (elementData.m_elementId != AZ::Edit::ClassElements::Group))
                {
                    // Record the last found group data
                    node->m_groupElementData = groupData;
                    break;
                }
                else if (elementData.m_elementId == AZ::Edit::ClassElements::Group)
                {
                    if (!elementData.m_description || !elementData.m_description[0])
                    { // close the group
                        groupData = nullptr;
                    }
                    else
                    {
                        groupData = &elementData;
                    }
                }
            }
        }

        m_curParentNode = node;

        return true;
    }
    //-----------------------------------------------------------------------------
    bool InstanceDataHierarchy::EndNode()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        AZ_Assert(m_curParentNode, "EndEnum called without a matching BeginNode call!");

        if (!m_nodeDiscarded)
        {
            if (m_isMerging)
            {
                for (NodeContainer::iterator it = m_curParentNode->m_children.begin(); it != m_curParentNode->m_children.end(); )
                {
                    // If we are merging and we did not match this node, remove it from the hierarchy.
                    if (!it->m_matched)
                    {
                        it = m_curParentNode->m_children.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
            }

            // if the node we are leaving pushed an edit data override, then pop it.
            if (!m_editDataOverrides.empty() && m_editDataOverrides.back().m_overridingNode == m_curParentNode)
            {
                m_editDataOverrides.pop_back();
            }

            m_curParentNode = m_curParentNode->m_parent;
        }
        m_nodeDiscarded = false;
        return true;
    }

    //-----------------------------------------------------------------------------
    bool InstanceDataHierarchy::RefreshComparisonData(unsigned int accessFlags, DynamicEditDataProvider dynamicEditDataProvider)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (!m_root || m_comparisonInstances.empty())
        {
            return false;
        }

        bool allDataMatches = true;

        // Clear comparison data.
        AZStd::vector<InstanceDataNode*> stack;
        stack.reserve(32);
        stack.push_back(this);
        while (!stack.empty())
        {
            InstanceDataNode* node = stack.back();
            stack.pop_back();

            node->ClearComparisonData();

            for (auto childIterator = node->m_children.begin(); childIterator != node->m_children.end();)
            {
                // Remove any fake nodes that have been added
                if (childIterator->IsRemovedVersusComparison())
                {
                    childIterator = node->m_children.erase(childIterator);
                }
                else
                {
                    stack.push_back(&(*childIterator));
                    ++childIterator;
                }
            }
        }

        // Generate a hierarchy for the comparison instance.
        m_comparisonHierarchies.clear();
        for (const InstanceData& comparisonInstance : m_comparisonInstances)
        {
            AZ_Assert(comparisonInstance.m_classId == m_rootInstances[0].m_classId, "Compare instance type does not match root instance type.");

            m_comparisonHierarchies.emplace_back(aznew InstanceDataHierarchy());
            auto& comparisonHierarchy = m_comparisonHierarchies.back();
            comparisonHierarchy->AddRootInstance(comparisonInstance.m_instance, comparisonInstance.m_classId);
            comparisonHierarchy->Build(m_root->GetSerializeContext(), accessFlags, dynamicEditDataProvider);

            // Compare the two hierarchies...
            if (comparisonHierarchy->m_root)
            {
                AZStd::vector<AZ::u8> sourceBuffer, targetBuffer;
                CompareHierarchies(comparisonHierarchy->m_root, m_root, sourceBuffer, targetBuffer, m_valueComparisonFunction, m_root->GetSerializeContext(),

                    // New node
                    [&allDataMatches](InstanceDataNode* targetNode, AZStd::vector<AZ::u8>& data)
                    {
                        (void)data;

                        if (!targetNode->IsNewVersusComparison())
                        {
                            targetNode->MarkNewVersusComparison();
                            allDataMatches = false;
                        }
                    },

                    // Removed node (container element).
                    [&allDataMatches](const InstanceDataNode* sourceNode, InstanceDataNode* targetNodeParent)
                    {
                        (void)sourceNode;

                        // Mark the parent as modified.
                        if (targetNodeParent)
                        {
                            // Insert a node to mark the removed element, with no data, but relating to the node in the source hierarchy.
                            targetNodeParent->m_children.push_back();
                            InstanceDataNode& removedMarker = targetNodeParent->m_children.back();
                            removedMarker = *sourceNode;
                            removedMarker.m_instances.clear();
                            removedMarker.m_children.clear();
                            removedMarker.m_comparisonNode = sourceNode;
                            removedMarker.m_parent = targetNodeParent;
                            removedMarker.m_root = targetNodeParent->m_root;
                            removedMarker.m_context = targetNodeParent->m_context;

                            removedMarker.MarkRemovedVersusComparison();
                            targetNodeParent->MarkDifferentVersusComparison();
                        }

                        allDataMatches = false;
                    },

                    // Changed node
                    [&allDataMatches](const InstanceDataNode* sourceNode, InstanceDataNode* targetNode,
                        AZStd::vector<AZ::u8>& sourceData, AZStd::vector<AZ::u8>& targetData)
                    {
                        (void)sourceData;
                        (void)targetData;

                        if (!targetNode->IsDifferentVersusComparison())
                        {
                            targetNode->m_comparisonNode = sourceNode;
                            targetNode->MarkDifferentVersusComparison();
                        }

                        allDataMatches = false;
                    }
                );
            }
        }

        return allDataMatches;
    }

    //-----------------------------------------------------------------------------
    InstanceDataNode* InstanceDataHierarchy::FindNodeByAddress(const InstanceDataNode::Address& address) const
    {
        if (m_root && !address.empty())
        {
            if (m_root->m_identifier != address.back())
            {
                // If this hierarchy's root isn't the same as the root in the address (the last entry), not going to find it here
                return nullptr;
            }
            else if (address.size() == 1)
            {
                // The only address in the list is the root node, matches root node
                return m_root;
            }

            InstanceDataNode* currentNode = m_root;
            size_t addressIndex = address.size() - 2;
            while (currentNode)
            {
                InstanceDataNode* parent = currentNode;
                currentNode = nullptr;

                for (InstanceDataNode& child : parent->m_children)
                {
                    if (child.m_identifier == address[addressIndex])
                    {
                        currentNode = &child;
                        if (addressIndex > 0)
                        {
                            --addressIndex;
                            break;
                        }
                        else
                        {
                            return currentNode;
                        }
                    }
                }
            }
        }

        return nullptr;
    }

    /// Locate a node by partial address (bfs to find closest match)
    InstanceDataNode* InstanceDataHierarchy::FindNodeByPartialAddress(const InstanceDataNode::Address& address) const
    {
        // ensure we have atleast a root and a valid address to search
        if (m_root && !address.empty())
        {
            AZStd::queue<InstanceDataNode*> children;
            children.push(m_root);

            // work our way down the hierarchy in a bfs search
            size_t addressIndex = address.size() - 1;
            while (children.size() > 0)
            {
                InstanceDataNode* curr = children.front();
                children.pop();

                // if we find property - move down the hierarchy
                if (curr->m_identifier == address[addressIndex])
                {
                    if (addressIndex > 0)
                    {
                        // clear existing list as we don't care about
                        // these elements anymore
                        while (!children.empty())
                        {
                            children.pop();
                        }

                        children.push(curr);
                        --addressIndex;
                    }
                    else
                    {
                        return curr;
                    }
                }

                // build fifo list of children to search
                for (InstanceDataNode& child : curr->m_children)
                {
                    children.push(&child);
                }
            }
        }

        return nullptr;
    }

    //-----------------------------------------------------------------------------
    bool InstanceDataHierarchy::DefaultValueComparisonFunction(const InstanceDataNode* sourceNode, const InstanceDataNode* targetNode)
    {
        // special case - while its possible for instance comparisons to differ, if one has an instance, and the other does not, they are definitely
        // not equal!
        if (sourceNode->GetNumInstances() == 0)
        {
            if (targetNode->GetNumInstances() == 0)
            {
                return true;
            }
            return false;
        }
        else if (targetNode->GetNumInstances() == 0)
        {
            return false;
        }
        else if (!targetNode->m_classData || !sourceNode->m_classData)
        {
            return targetNode->m_classData == sourceNode->m_classData;
        }

        return targetNode->m_classData->m_serializer->CompareValueData(sourceNode->FirstInstance(), targetNode->FirstInstance());
    }

    //-----------------------------------------------------------------------------
    void InstanceDataHierarchy::SetValueComparisonFunction(const ValueComparisonFunction& function)
    {
        m_valueComparisonFunction = function ? function : DefaultValueComparisonFunction;
    }

    //-----------------------------------------------------------------------------
    void InstanceDataHierarchy::CompareHierarchies(const InstanceDataNode* sourceNode, InstanceDataNode* targetNode,
        const ValueComparisonFunction& valueComparisonFunction,
        AZ::SerializeContext* context,
        NewNodeCB newNodeCallback,
        RemovedNodeCB removedNodeCallback,
        ChangedNodeCB changedNodeCallback)
    {
        AZ_Assert(sourceNode->GetSerializeContext() == targetNode->GetSerializeContext(),
            "Attempting to compare hierarchies from different serialization contexts.");

        AZStd::vector<AZ::u8> sourceBuffer, targetBuffer;
        CompareHierarchies(sourceNode, targetNode, sourceBuffer, targetBuffer, valueComparisonFunction, context, newNodeCallback, removedNodeCallback, changedNodeCallback);
    }

    //-----------------------------------------------------------------------------
    void InstanceDataHierarchy::CompareHierarchies(const InstanceDataNode* sourceNode, InstanceDataNode* targetNode,
        AZStd::vector<AZ::u8>& tempSourceBuffer,
        AZStd::vector<AZ::u8>& tempTargetBuffer,
        const ValueComparisonFunction& valueComparisonFunction,
        AZ::SerializeContext* context,
        NewNodeCB newNodeCallback,
        RemovedNodeCB removedNodeCallback,
        ChangedNodeCB changedNodeCallback)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        targetNode->m_comparisonNode = sourceNode;

        if (!targetNode->m_classData || !sourceNode->m_classData)
        {
            return;
        }
        else if (targetNode->m_classData->m_typeId == sourceNode->m_classData->m_typeId)
        {
            if (targetNode->m_classData->m_container)
            {
                // Find elements in the container that have been added or modified.
                for (InstanceDataNode& targetElementNode : targetNode->m_children)
                {
                    if (targetElementNode.IsRemovedVersusComparison())
                    {
                        continue; // Don't compare removal placeholders.
                    }

                    const InstanceDataNode* sourceNodeMatch = nullptr;

                    for (const InstanceDataNode& sourceElementNode : sourceNode->m_children)
                    {
                        if (sourceElementNode.m_identifier == targetElementNode.m_identifier)
                        {
                            sourceNodeMatch = &sourceElementNode;
                            break;
                        }
                    }

                    if (sourceNodeMatch)
                    {
                        // The element exists, drill down.
                        CompareHierarchies(sourceNodeMatch, &targetElementNode, tempSourceBuffer, tempTargetBuffer, valueComparisonFunction, context,
                            newNodeCallback, removedNodeCallback, changedNodeCallback);
                    }
                    else
                    {
                        // This is a new element in the container.
                        AZStd::vector<AZ::u8> buffer;
                        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8> > stream(&buffer);
                        if (!AZ::Utils::SaveObjectToStream(stream, AZ::ObjectStream::ST_BINARY, targetElementNode.GetInstance(0), targetElementNode.m_classData->m_typeId, context, targetElementNode.m_classData))
                        {
                            AZ_Assert(false, "Unable to serialize class %s, SaveObjectToStream() failed.", targetElementNode.m_classData->m_name);
                        }

                        // Mark targetElementNode and all of its children, and their children, and their... as new
                        AZStd::vector<InstanceDataNode*> stack;
                        stack.reserve(32);
                        stack.push_back(&targetElementNode);
                        while (!stack.empty())
                        {
                            InstanceDataNode* node = stack.back();
                            stack.pop_back();

                            newNodeCallback(node, buffer);

                            // Don't do the children if the current node represents a component since it will add as one whole thing
                            if (!node->GetClassMetadata() || !node->GetClassMetadata()->m_azRtti || !node->GetClassMetadata()->m_azRtti->IsTypeOf(AZ::Component::RTTI_Type()))
                            {
                                for (InstanceDataNode& child : node->m_children)
                                {
                                    stack.push_back(&child);
                                }
                            }
                        }
                    }
                }

                // Find elements that've been removed.
                for (const InstanceDataNode& sourceElementNode : sourceNode->m_children)
                {
                    bool isRemoved = true;

                    for (InstanceDataNode& targetElementNode : targetNode->m_children)
                    {
                        if (sourceElementNode.m_identifier == targetElementNode.m_identifier)
                        {
                            isRemoved = false;
                            break;
                        }
                    }

                    if (isRemoved)
                    {
                        removedNodeCallback(&sourceElementNode, targetNode);
                    }
                }
            }
            else if (targetNode->m_classData->m_serializer)
            {
                AZ_Assert(targetNode->m_classData == sourceNode->m_classData, "Comparison raw data for mismatched types.");

                if (!valueComparisonFunction(sourceNode, targetNode))
                {
                    // This is a leaf element (has a direct serializer), so it's a data change.
                    tempSourceBuffer.clear();
                    tempTargetBuffer.clear();
                    AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8> > sourceStream(&tempSourceBuffer), targetStream(&tempTargetBuffer);
                    targetNode->m_classData->m_serializer->Save(targetNode->GetInstance(0), targetStream);
                    sourceNode->m_classData->m_serializer->Save(sourceNode->GetInstance(0), sourceStream);

                    changedNodeCallback(sourceNode, targetNode, tempSourceBuffer, tempTargetBuffer);
                }
            }
            else
            {
                // This isn't a container; just drill down on child elements.
                if (targetNode->m_children.size() == sourceNode->m_children.size())
                {
                    auto targetElementIt = targetNode->m_children.begin();
                    auto sourceElementIt = sourceNode->m_children.begin();
                    while (targetElementIt != targetNode->m_children.end())
                    {
                        CompareHierarchies(&(*sourceElementIt), &(*targetElementIt), tempSourceBuffer, tempTargetBuffer, valueComparisonFunction, context,
                            newNodeCallback, removedNodeCallback, changedNodeCallback);

                        ++sourceElementIt;
                        ++targetElementIt;
                    }
                }
                else
                {
                    AZ_Error("Serializer", targetNode->m_children.size() == sourceNode->m_children.size(),
                        "Non-container elements have mismatched sub-element counts. This a recoverable error, "
                        "but the entire sub-hierarchy will be considered differing as no further drill-down is possible.");

                    tempSourceBuffer.clear();
                    tempTargetBuffer.clear();
                    changedNodeCallback(sourceNode, targetNode, tempSourceBuffer, tempTargetBuffer);
                }
            }
        }
    }

    //-----------------------------------------------------------------------------
    bool InstanceDataHierarchy::CopyInstanceData(const InstanceDataNode* sourceNode, 
            InstanceDataNode* targetNode,
            AZ::SerializeContext* context, 
            ContainerChildNodeBeingRemovedCB containerChildNodeBeingRemovedCB, 
            ContainerChildNodeBeingCreatedCB containerChildNodeBeingCreatedCB,
            const InstanceDataNode::Address& filterElementAddress)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (!context)
        {
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(context, "Failed to retrieve application serialization context");
        }

        const AZ::SerializeContext::ClassData* sourceClass = sourceNode->GetClassMetadata();
        const AZ::SerializeContext::ClassData* targetClass = targetNode->GetClassMetadata();

        if (!sourceClass || !targetClass)
        {
            return false;
        }

        if (sourceClass->m_typeId == targetClass->m_typeId)
        {
            // Drill down and apply adds/removes/copies as we go.
            bool result = true;

            if (targetClass->m_eventHandler)
            {
                for (size_t i = 0; i < targetNode->GetNumInstances(); ++i)
                {
                    targetClass->m_eventHandler->OnWriteBegin(targetNode->GetInstance(i));
                }
            }

            if (sourceClass->m_serializer)
            {
                // These are leaf elements, we can just copy directly.
                AZStd::vector<AZ::u8> sourceBuffer;
                AZ::IO::ByteContainerStream<decltype(sourceBuffer)> sourceStream(&sourceBuffer);
                sourceClass->m_serializer->Save(sourceNode->GetInstance(0), sourceStream);

                for (size_t i = 0; i < targetNode->GetNumInstances(); ++i)
                {
                    sourceStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
                    targetClass->m_serializer->Load(targetNode->GetInstance(i), sourceStream, sourceClass->m_version);
                }
            }
            else
            {
                // Storage to track elements to be added or removed to reflect differences between 
                // source and target container contents.
                using InstancePair = AZStd::pair<void*, void*>;
                AZStd::vector<InstancePair> elementsToRemove;
                AZStd::vector<const InstanceDataNode*> elementsToAdd;

                // If we're operating on a container, we need to identify any items in the target
                // node that don't exist in the source, and remove them.
                if (sourceClass->m_container)
                {
                    for (auto targetChildIter = targetNode->GetChildren().begin(); targetChildIter != targetNode->GetChildren().end(); )
                    {
                        InstanceDataNode& targetChild = *targetChildIter;

                        // Skip placeholders, or if we're filtering for a different element.
                        if ((targetChild.IsRemovedVersusComparison()) || 
                            (!filterElementAddress.empty() && filterElementAddress != targetChild.ComputeAddress()))
                        {
                            ++targetChildIter;
                            continue;
                        }

                        bool sourceFound = false;
                        bool removedElement = false;

                        for (const InstanceDataNode& sourceChild : sourceNode->GetChildren())
                        {
                            if (sourceChild.IsRemovedVersusComparison())
                            {
                                continue;
                            }

                            if (sourceChild.m_identifier == targetChild.m_identifier)
                            {
                                sourceFound = true;
                                break;
                            }
                        }

                        if (!sourceFound)
                        {
                            AZ_Assert(targetClass->m_container, "Hierarchy mismatch occurred, but not on a container element.");
                            if (targetClass->m_container)
                            {
                                for (size_t i = 0; i < targetNode->GetNumInstances(); ++i)
                                {
                                    elementsToRemove.push_back(AZStd::make_pair(targetNode->GetInstance(i), targetChild.m_instances[i]));
                                }
                                
                                if (containerChildNodeBeingRemovedCB)
                                {
                                    containerChildNodeBeingRemovedCB(&targetChild);
                                }

                                // The child hierarchy node can be deleted. We'll free the actual container elements in a safe manner below.
                                targetChildIter = targetNode->GetChildren().erase(targetChildIter);

                                removedElement = true;
                            }
                        }

                        if (!removedElement)
                        {
                            ++targetChildIter;
                        }
                    }
                }

                // Recursively deep-copy any differences.
                for (const InstanceDataNode& sourceChild : sourceNode->GetChildren())
                {
                    bool matchedChild = false;

                    // Skip placeholders, or if we're filtering for an element that isn't the sourceChild or one of it's descendants.
                    if ((sourceChild.IsRemovedVersusComparison()) ||
                        (!filterElementAddress.empty() && filterElementAddress != sourceChild.ComputeAddress() && !sourceChild.ChildMatchesAddress(filterElementAddress)))
                    {
                        continue;
                    }

                    // For each source child, locate the respective target child and drill down.
                    for (InstanceDataNode& targetChild : targetNode->GetChildren())
                    {
                        if (targetChild.IsRemovedVersusComparison())
                        {
                            continue;
                        }

                        if (targetChild.m_identifier == sourceChild.m_identifier)
                        {
                            matchedChild = true;

                            const AZ::SerializeContext::ClassData* targetClassData = targetChild.GetClassMetadata();
                            const AZ::SerializeContext::ClassData* sourceClassData = sourceChild.GetClassMetadata();

                            // are these proxy nodes?
                            if (!targetClassData && !sourceClassData)
                            {
                                continue;
                            }
                            else if (targetClassData->m_typeId != sourceClassData->m_typeId)
                            {
                                AZStd::string sourceTypeId;
                                AZStd::string targetTypeId;

                                sourceClassData->m_typeId.ToString(sourceTypeId);
                                targetClassData->m_typeId.ToString(targetTypeId);

                                AZ_Error("Serializer", false, "Nodes with same identifier are not of the same serializable type: types \"%s\" : %s and \"%s\" : %s.",
                                    sourceClassData->m_name, sourceTypeId.c_str(), targetClassData->m_name, targetTypeId.c_str());
                                return false;
                            }

                            // Recurse on child elements.
                            if (!CopyInstanceData(&sourceChild, &targetChild, context, containerChildNodeBeingRemovedCB, containerChildNodeBeingCreatedCB, filterElementAddress))
                            {
                                result = false;
                                break;
                            }
                        }
                    }

                    if (matchedChild)
                    {
                        continue;
                    }

                    // The target node was not found. 
                    // This occurs if the source is a container, and contains an element that the target does not.
                    AZ_Assert(targetClass->m_container, "Hierarchy mismatch occurred, but not on a container element.");
                    if (result && targetClass->m_container)
                    {
                        elementsToAdd.push_back(&sourceChild);
                    }
                }

                // After iterating through all children to apply changes, it's now safe to commit element removals and additions.
                // Containers may grow during additions, or shift during removals, so we can't do it while iterating and recursing
                // on elements to apply data differences.

                //
                // Apply element removals.
                //

                // Sort element removals in reverse memory order for compatibility with contiguous-memory containers.
                AZStd::sort(elementsToRemove.begin(), elementsToRemove.end(),
                    [](const InstancePair& lhs, const InstancePair& rhs)
                    {
                        return rhs.second < lhs.second;
                    }
                );

                // Finally, remove elements from the target container that weren't present in the source.
                for (const auto& pair : elementsToRemove)
                {
                    targetClass->m_container->RemoveElement(pair.first, pair.second, context);
                }

                //
                // Apply element additions.
                //

                // After iterating through all children to apply changes, it's now safe to commit element additions.
                // Containers may grow/reallocate, so we can't do it while iterating.
                for (const InstanceDataNode* sourceChildToAdd : elementsToAdd)
                {
                    if (containerChildNodeBeingCreatedCB)
                    {
                        containerChildNodeBeingCreatedCB(sourceChildToAdd, targetNode);
                    }

                    // Serialize out the entire source element.
                    AZStd::vector<AZ::u8> sourceBuffer;
                    AZ::IO::ByteContainerStream<decltype(sourceBuffer)> sourceStream(&sourceBuffer);
                    bool savedToStream = AZ::Utils::SaveObjectToStream(
                        sourceStream, AZ::DataStream::ST_BINARY, sourceChildToAdd->GetInstance(0), context, sourceChildToAdd->GetClassMetadata());
                    (void)savedToStream;
                    AZ_Assert(savedToStream, "Failed to save source element to data stream.");

                    for (size_t i = 0; i < targetNode->GetNumInstances(); ++i)
                    {
                        // Add a new element to the target container.
                        void* targetInstance = targetNode->GetInstance(i);
                        void* targetPointer = targetClass->m_container->ReserveElement(targetInstance, sourceChildToAdd->m_classElement);
                        AZ_Assert(targetPointer, "Failed to allocate container element");

                        if (sourceChildToAdd->GetClassMetadata() && sourceChildToAdd->m_classElement->m_flags & AZ::SerializeContext::ClassElement::FLG_POINTER)
                        {
                            // It's a container of pointers, so allocate a new target class instance.
                            AZ_Assert(sourceChildToAdd->GetClassMetadata()->m_factory != nullptr, "We are attempting to create '%s', but no factory is provided! Either provide factory or change data member '%s' to value not pointer!", sourceNode->m_classData->m_name, sourceNode->m_classElement->m_name);
                            void* newTargetPointer = sourceChildToAdd->m_classData->m_factory->Create(sourceChildToAdd->m_classData->m_name);
                            (void)newTargetPointer;

                            void* basePtr = context->DownCast(
                                newTargetPointer, 
                                sourceChildToAdd->m_classData->m_typeId, 
                                sourceChildToAdd->m_classElement->m_typeId, 
                                sourceChildToAdd->m_classData->m_azRtti, 
                                sourceChildToAdd->m_classElement->m_azRtti);

                            AZ_Assert(basePtr != nullptr, sourceClass->m_container
                                ? "Can't cast container element %s(0x%x) to %s, make sure classes are registered in the system and not generics!"
                                : "Can't cast %s(0x%x) to %s, make sure classes are registered in the system and not generics!"
                                , sourceChildToAdd->m_classElement->m_name ? sourceChildToAdd->m_classElement->m_name : "NULL"
                                , sourceChildToAdd->m_classElement->m_nameCrc
                                , sourceChildToAdd->m_classData->m_name);

                            // Store ptr to the new instance in the container element.
                            *reinterpret_cast<void**>(targetPointer) = basePtr;

                            targetPointer = newTargetPointer;
                        }

                        // Deserialize in-place.
                        sourceStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

                        bool loadedFromStream = AZ::Utils::LoadObjectFromStreamInPlace(
                            sourceStream, context, sourceChildToAdd->GetClassMetadata(), targetPointer, AZ::ObjectStream::FilterDescriptor(AZ::Data::AssetFilterNoAssetLoading));
                        (void)loadedFromStream;
                        AZ_Assert(loadedFromStream, "Failed to copy element to target.");

                        // Some containers, such as AZStd::map require you to call StoreElement to actually consider the element part of the structure
                        // Since the container is unable to put an uninitialized element into its tree until the key is known
                        targetClass->m_container->StoreElement(targetInstance, targetPointer);
                    }
                }
            }

            if (targetClass->m_eventHandler)
            {
                for (size_t i = 0; i < targetNode->GetNumInstances(); ++i)
                {
                    targetClass->m_eventHandler->OnWriteEnd(targetNode->GetInstance(i));
                }
            }

            return result;
        }

        return false;
    }

    //-----------------------------------------------------------------------------
}   // namespace Property System
