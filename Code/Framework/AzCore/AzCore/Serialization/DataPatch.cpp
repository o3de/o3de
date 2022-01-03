/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <cinttypes>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/DataPatch.h>
#include <AzCore/Serialization/DataPatchBus.h>
#include <AzCore/Serialization/DataPatchUpgradeManager.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ
{
    // Legacy PatchMap type to help with conversion
    using LegacyPatchMap = AZStd::unordered_map<AddressType, AZStd::vector<AZ::u8>>;

    // Helper method for converting Legacy Data Patches
    static AZ::Outcome<void, AZStd::string> ConvertByteStreamMapToAnyMap(SerializeContext& context, AZ::SerializeContext::DataElementNode& dataPatchElement, PatchMap& anyPatchMap);

    // Deprecation Converter for Legacy Data Patches of typeId {3A8D5EC9-D70E-41CB-879C-DEF6A6D6ED03}
    static bool LegacyDataPatchConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    static AZ::Outcome<void, AZStd::string> LegacyDataPatchConverter_Impl(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

    class DataNode
    {
    public:
        using ChildDataNodes = AZStd::list<DataNode>;

        DataNode()
        {
            Reset();
        }

        void Reset()
        {
            m_data = nullptr;
            m_parent = nullptr;
            m_children.clear();
            m_classData = nullptr;
            m_classElement = nullptr;
        }

        void*           m_data;
        DataNode*       m_parent;
        ChildDataNodes  m_children;

        const SerializeContext::ClassData*      m_classData;
        const SerializeContext::ClassElement*   m_classElement;
    };

    class DataNodeTree
    {
    public:
        DataNodeTree(SerializeContext* context)
            : m_currentNode(nullptr)
            , m_context(context)
        {}

        void Build(const void* classPtr, const Uuid& classId);

        bool BeginNode(
            void* ptr,
            const SerializeContext::ClassData* classData,
            const SerializeContext::ClassElement* classElement);
        bool EndNode();

        /// Compare two nodes and fill the patch structure
        static void CompareElements(
            const DataNode* sourceNode,
            const DataNode* targetNode,
            PatchMap& patch,
            const DataPatch::FlagsMap& sourceFlagsMap,
            const DataPatch::FlagsMap& targetFlagsMap,
            SerializeContext* context);

        static void CompareElementsInternal(
            const DataNode* sourceNode,
            const DataNode* targetNode,
            PatchMap& patch,
            const DataPatch::FlagsMap& sourceFlagsMap,
            const DataPatch::FlagsMap& targetFlagsMap,
            SerializeContext* context,
            AddressType& address,
            DataPatch::Flags parentAddressFlags,
            AZStd::vector<AZ::u8>& tmpSourceBuffer);

        /// Apply patch to elements, return a valid pointer only for the root element
        static void* ApplyToElements(
            DataNode* sourceNode,
            PatchMap& patch,
            const ChildPatchMap& childPatchLookup,
            const DataPatch::FlagsMap& sourceFlagsMap,
            const DataPatch::FlagsMap& targetFlagsMap,
            DataPatch::Flags parentAddressFlags,
            AddressType& address,
            void* parentPointer,
            const SerializeContext::ClassData* parentClassData,
            AZStd::vector<AZ::u8>& tmpSourceBuffer,
            SerializeContext* context,
            const AZ::ObjectStream::FilterDescriptor& filterDesc,
            int& parentContainerElementCounter);

        static DataPatch::Flags CalculateDataFlagsAtThisAddress(
            const DataPatch::FlagsMap& sourceFlagsMap,
            const DataPatch::FlagsMap& targetFlagsMap,
            DataPatch::Flags parentAddressFlags,
            const AddressType& address);

        // Helper methods for constructing an AZStd::any around PatchData
        static bool CreateDataPatchAny(AZ::SerializeContext& serializeContext,
            const void* classPtr,
            const AZ::TypeId& typeId,
            AZStd::any& dataPatchAnyDest);

        static bool LoadInPlaceStreamWrapper(AZ::SerializeContext& context,
            AZStd::any& patchAny,
            const DataNode& sourceNode,
            const AZ::SerializeContext::ClassData* parentClassData,
            const AZ::ObjectStream::FilterDescriptor& filterDesc);

        DataNode m_root;
        DataNode* m_currentNode;        ///< Used as temp during tree building
        SerializeContext* m_context;
        AZStd::list<SerializeContext::ClassElement> m_dynamicClassElements; ///< Storage for class elements that represent dynamic serializable fields.
    };

    static void ReportDataPatchMismatch(SerializeContext* context, const SerializeContext::ClassElement* classElement, const TypeId& patchDataTypeId);

    //=========================================================================
    // DataNodeTree::Build
    //=========================================================================
    void DataNodeTree::Build(const void* rootClassPtr, const Uuid& rootClassId)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        m_root.Reset();
        m_currentNode = nullptr;

        if (!m_context || !rootClassPtr)
        {
            return;
        }
        SerializeContext::EnumerateInstanceCallContext callContext(
            [this](void* instancePointer, const SerializeContext::ClassData* classData, const SerializeContext::ClassElement* classElement)->bool
            {
                return BeginNode(instancePointer, classData, classElement);
            },
            [this]()->bool { return EndNode(); },
            m_context,
            SerializeContext::ENUM_ACCESS_FOR_READ,
            nullptr
        );

        m_context->EnumerateInstanceConst(
            &callContext,
            rootClassPtr,
            rootClassId,
            nullptr,
            nullptr
        );
        m_currentNode = nullptr;
    }

    //=========================================================================
    // DataNodeTree::BeginNode
    //=========================================================================
    bool DataNodeTree::BeginNode(
        void* ptr,
        const SerializeContext::ClassData* classData,
        const SerializeContext::ClassElement* classElement)
    {
        DataNode* newNode;
        if (m_currentNode)
        {
            m_currentNode->m_children.push_back();
            newNode = &m_currentNode->m_children.back();
        }
        else
        {
            newNode = &m_root;
        }

        newNode->m_parent = m_currentNode;
        newNode->m_classData = classData;

        // ClassElement pointers for DynamicSerializableFields are temporaries, so we need
        // to maintain it locally.
        if (classElement)
        {
            if (classElement->m_flags & SerializeContext::ClassElement::FLG_DYNAMIC_FIELD)
            {
                m_dynamicClassElements.push_back(*classElement);
                classElement = &m_dynamicClassElements.back();
            }
        }

        newNode->m_classElement = classElement;

        if (classElement && (classElement->m_flags & SerializeContext::ClassElement::FLG_POINTER))
        {
            // we always store the value address
            newNode->m_data = *(void**)(ptr);
        }
        else
        {
            newNode->m_data = ptr;
        }

        if (classData->m_eventHandler)
        {
            classData->m_eventHandler->OnReadBegin(newNode->m_data);
        }

        m_currentNode = newNode;
        return true;
    }

    //=========================================================================
    // DataNodeTree::EndNode
    //=========================================================================
    bool DataNodeTree::EndNode()
    {
        if (m_currentNode->m_classData->m_eventHandler)
        {
            m_currentNode->m_classData->m_eventHandler->OnReadEnd(m_currentNode->m_data);
        }

        m_currentNode = m_currentNode->m_parent;
        return true;
    }

    //=========================================================================
    // DataNodeTree::CompareElements
    //=========================================================================
    void DataNodeTree::CompareElements(
        const DataNode* sourceNode,
        const DataNode* targetNode,
        PatchMap& patch,
        const DataPatch::FlagsMap& sourceFlagsMap,
        const DataPatch::FlagsMap& targetFlagsMap,
        SerializeContext* context)
    {
        AddressType tmpAddress;
        AZStd::vector<AZ::u8> tmpSourceBuffer;

        CompareElementsInternal(
            sourceNode,
            targetNode,
            patch,
            sourceFlagsMap,
            targetFlagsMap,
            context,
            tmpAddress,
            0,
            tmpSourceBuffer);
    }

    //=========================================================================
    // DataNodeTree::CompareElementsInternal
    //=========================================================================
    void DataNodeTree::CompareElementsInternal(
        const DataNode* sourceNode,
        const DataNode* targetNode,
        PatchMap& patch,
        const DataPatch::FlagsMap& sourceFlagsMap,
        const DataPatch::FlagsMap& targetFlagsMap,
        SerializeContext* context,
        AddressType& address,
        DataPatch::Flags parentAddressFlags,
        AZStd::vector<AZ::u8>& tmpSourceBuffer)
    {
        // calculate the flags affecting this address
        DataPatch::Flags addressFlags = CalculateDataFlagsAtThisAddress(sourceFlagsMap, targetFlagsMap, parentAddressFlags, address);

        // don't compare any addresses affected by the PreventOverride flag
        if (addressFlags & DataPatch::Flag::PreventOverrideEffect)
        {
            return;
        }

        if (targetNode->m_classData->m_typeId != sourceNode->m_classData->m_typeId)
        {
            // Store the entire target class in an AZStd::any and place into the PatchMap
            auto insertResult = patch.insert_key(address);
            bool createAnyResult = CreateDataPatchAny(*context, targetNode->m_data, targetNode->m_classData->m_typeId, insertResult.first->second);
            AZ_UNUSED(createAnyResult);

            AZ_Assert(createAnyResult, "Unable to store class %s, CreateDataPatchAny Failed. Verify that TypeId %s is properly reflected and is not a generic TypeId",
                targetNode->m_classData->m_name, targetNode->m_classData->m_typeId.ToString<AZStd::string>().c_str());

            return;
        }

        if (targetNode->m_classData->m_container)
        {
            AZStd::unordered_map<const DataNode*, AZStd::pair<u64, bool>> nodesToRemove;
            nodesToRemove.reserve(sourceNode->m_children.size());
            u64 elementIndex = 0;
            AZStd::pair<u64, bool> tempPair(0, true);
            for (auto& sourceElementNode : sourceNode->m_children)
            {
                SerializeContext::ClassPersistentId sourcePersistentIdFunction = sourceElementNode.m_classData->GetPersistentId(*context);

                tempPair.first = 
                    sourcePersistentIdFunction 
                    ? sourcePersistentIdFunction(sourceElementNode.m_data) 
                    : elementIndex;

                nodesToRemove[&sourceElementNode] = tempPair;

                ++elementIndex;
            }

            // find elements that we have added or modified
            elementIndex = 0;
            AZ::u64 elementId = 0;
            for (const DataNode& targetElementNode : targetNode->m_children)
            {
                const DataNode* sourceNodeMatch = nullptr;
                SerializeContext::ClassPersistentId targetPersistentIdFunction = targetElementNode.m_classData->GetPersistentId(*context);
                if (targetPersistentIdFunction)
                {
                    u64 targetElementId = targetPersistentIdFunction(targetElementNode.m_data);

                    for (const DataNode& sourceElementNode : sourceNode->m_children)
                    {
                        SerializeContext::ClassPersistentId sourcePersistentIdFunction = sourceElementNode.m_classData->GetPersistentId(*context);
                        if (sourcePersistentIdFunction && targetElementId == sourcePersistentIdFunction(sourceElementNode.m_data))
                        {
                            sourceNodeMatch = &sourceElementNode;
                            break;
                        }
                    }

                    elementId = targetElementId; // we use persistent ID for an id
                }
                else
                {
                    // if we don't have IDs use the container index
                    if (elementIndex < sourceNode->m_children.size())
                    {
                        sourceNodeMatch = &(*AZStd::next(sourceNode->m_children.begin(), elementIndex));
                    }

                    elementId = elementIndex; // use index as an ID
                }

                address.emplace_back(elementId, 
                                     targetElementNode.m_classData,
                                     nullptr,
                                     AddressTypeElement::ElementType::Index);

                if (sourceNodeMatch)
                {
                    nodesToRemove[sourceNodeMatch].second = false;

                    // compare elements
                    CompareElementsInternal(
                        sourceNodeMatch,
                        &targetElementNode,
                        patch,
                        sourceFlagsMap,
                        targetFlagsMap,
                        context,
                        address,
                        addressFlags,
                        tmpSourceBuffer);
                }
                else
                {
                    // this is a new node store it
                    auto insertResult = patch.insert_key(address);
                    bool createAnyResult = CreateDataPatchAny(*context, targetElementNode.m_data, targetElementNode.m_classData->m_typeId, insertResult.first->second);
                    AZ_UNUSED(createAnyResult);

                    AZ_Assert(createAnyResult, "Unable to store class %s, CreateDataPatchAny Failed. Verify that TypeId %s is properly reflected and is not a generic TypeId",
                        targetNode->m_classData->m_name, targetNode->m_classData->m_typeId.ToString<AZStd::string>().c_str());
                }

                address.pop_back();

                ++elementIndex;
            }

            // find elements we have removed 
            for (auto& node : nodesToRemove)
            {
                //do not need to remove this node
                if (!node.second.second)
                {
                    continue;
                }

                address.emplace_back(node.second.first,
                                     node.first->m_classData,
                                     nullptr,
                                     AddressTypeElement::ElementType::Index);

                // record removal of element by inserting a key with a 0 byte patch
                patch.insert_key(address);

                address.pop_back();
            }
        }
        else if (targetNode->m_classData->m_serializer)
        {
            AZ_Assert(targetNode->m_classData == sourceNode->m_classData, "Comparison raw data for mismatched types.");

            // This is a leaf element (has a direct serializer).
            // Write to patch if values differ, or if the ForceOverride flag affects this address
            if ((addressFlags & DataPatch::Flag::ForceOverrideEffect)
                || !targetNode->m_classData->m_serializer->CompareValueData(sourceNode->m_data, targetNode->m_data))
            {
                //serialize target override
                auto insertResult = patch.insert_key(address);
                bool createAnyResult = CreateDataPatchAny(*context, targetNode->m_data, targetNode->m_classData->m_typeId, insertResult.first->second);
                AZ_UNUSED(createAnyResult);

                AZ_Assert(createAnyResult, "Unable to store class %s, CreateDataPatchAny Failed. Verify that TypeId %s is properly reflected and is not a generic TypeId",
                    targetNode->m_classData->m_name, targetNode->m_classData->m_typeId.ToString<AZStd::string>().c_str());
            }
        }
        else
        {
            // Not containers, just compare elements. Since they are known at compile time and class data is shared
            // elements will be there if they are not nullptr
            // When elements are nullptrs, they are not serialized out and therefore causes the number of children to differ
            /* For example the AzFramework::EntityReference class serializes out a AZ::EntityId and AZ::Entity*
            class EntityReference
            {
                AZ::Entity m_entityId;
                AZ::Entity* m_entity;
            }
                If the EntityReference::m_entity element is nullptr in the source instance, it is not part of the source node children
            */
            // find elements that we have added or modified by creating a union of the source data nodes and target data nodes

            AZStd::unordered_map<AZ::u64, const AZ::DataNode*> sourceAddressMap;
            AZStd::unordered_map<AZ::u64, const AZ::DataNode*> targetAddressMap;
            AZStd::unordered_map<AZ::u64, const AZ::DataNode*> unionAddressMap;
            for (auto targetElementIt = targetNode->m_children.begin(); targetElementIt != targetNode->m_children.end(); ++targetElementIt)
            {
                targetAddressMap.emplace(targetElementIt->m_classElement->m_nameCrc, &(*targetElementIt));
                unionAddressMap.emplace(targetElementIt->m_classElement->m_nameCrc, &(*targetElementIt));
            }

            for (auto sourceElementIt = sourceNode->m_children.begin(); sourceElementIt != sourceNode->m_children.end(); ++sourceElementIt)
            {
                sourceAddressMap.emplace(sourceElementIt->m_classElement->m_nameCrc, &(*sourceElementIt));
                unionAddressMap.emplace(sourceElementIt->m_classElement->m_nameCrc, &(*sourceElementIt));
            }

            for (auto& unionAddressNode : unionAddressMap)
            {
                auto sourceFoundIt = sourceAddressMap.find(unionAddressNode.first);
                auto targetFoundIt = targetAddressMap.find(unionAddressNode.first);

                if (sourceFoundIt != sourceAddressMap.end() && targetFoundIt != targetAddressMap.end())
                {
                    // Use class element name as an ID
                    address.emplace_back(sourceFoundIt->first,
                                         sourceFoundIt->second->m_classData,
                                         sourceFoundIt->second->m_classElement,
                                         AddressTypeElement::ElementType::Class);

                    CompareElementsInternal(
                        sourceFoundIt->second,
                        targetFoundIt->second,
                        patch,
                        sourceFlagsMap,
                        targetFlagsMap,
                        context,
                        address,
                        addressFlags,
                        tmpSourceBuffer);

                    address.pop_back();
                }
                else if (targetFoundIt != targetAddressMap.end())
                {
                    // This is a new node store it
                    // Use class element name as an ID
                    address.emplace_back(targetFoundIt->first,
                                         targetFoundIt->second->m_classData,
                                         targetFoundIt->second->m_classElement,
                                         AddressTypeElement::ElementType::Class);

                    auto insertResult = patch.insert_key(address);

                    auto& targetElementNode = targetFoundIt->second;
                    bool createAnyResult = CreateDataPatchAny(*context, targetElementNode->m_data, targetElementNode->m_classData->m_typeId, insertResult.first->second);
                    AZ_UNUSED(createAnyResult);

                    AZ_Assert(createAnyResult, "Unable to store class %s, CreateDataPatchAny Failed. Verify that TypeId %s is properly reflected and is not a generic TypeId",
                        targetNode->m_classData->m_name, targetNode->m_classData->m_typeId.ToString<AZStd::string>().c_str());

                    address.pop_back();
                }
                else
                {
                    // Use class element name as an ID
                    address.emplace_back(sourceFoundIt->first,
                                         sourceFoundIt->second->m_classData,
                                         sourceFoundIt->second->m_classElement,
                                         AddressTypeElement::ElementType::Class);

                    patch.insert_key(address); // record removal of element by inserting a key with a 0 byte patch
                    address.pop_back();
                }
            }
        }
    }

    //=========================================================================
    // ApplyToElements
    //=========================================================================
    void* DataNodeTree::ApplyToElements(
        DataNode* sourceNode,
        PatchMap& patch,
        const ChildPatchMap& childPatchLookup,
        const DataPatch::FlagsMap& sourceFlagsMap,
        const DataPatch::FlagsMap& targetFlagsMap,
        DataPatch::Flags parentAddressFlags,
        AddressType& address,
        void* parentPointer,
        const SerializeContext::ClassData* parentClassData,
        AZStd::vector<AZ::u8>& tmpSourceBuffer,
        SerializeContext* context,
        const AZ::ObjectStream::FilterDescriptor& filterDesc,
        int& parentContainerElementCounter)
    {
        void* targetPointer = nullptr;
        void* reservePointer = nullptr;

        // calculate the flags affecting this address
        DataPatch::Flags addressFlags = CalculateDataFlagsAtThisAddress(sourceFlagsMap, targetFlagsMap, parentAddressFlags, address);

        auto patchIt = patch.find(address);
        if (patchIt != patch.end() && !(addressFlags & DataPatch::Flag::PreventOverrideEffect))
        {
            if (patchIt->second.empty())
            {
                // Allocate space in the container for our element
                // if patch is empty do remove the element
                return nullptr;
            }

            if (patchIt->second.type() == azrtti_typeid<DataPatch::LegacyStreamWrapper>())
            {
                // Check if this patch went through the Deprecation Converter and contains StreamWrappers that need to be loaded
                // Since we now have the classData of the object we can load from stream successfully
                bool loadSuccess = LoadInPlaceStreamWrapper(*context, patchIt->second, *sourceNode, parentClassData, filterDesc);

                if (!loadSuccess)
                {
                    AZ_Error("DataNodeTree::ApplyToElements", false,
                        "Attempted to load object with Element Name: %s and TypeId: %s from StreamWrapper during DataPatch::Apply and failed\n"
                        "The element and its children will be defaulted or removed from the patched object\n"
                        "This is likely caused by a corrupted DataPatch on disk.\n"
                        "It is recommended to debug how that data is interpreted by the converter in source code.",
                        sourceNode->m_classElement->m_name, sourceNode->m_classElement->m_typeId.ToString<AZStd::string>().c_str());

                    return nullptr;
                }
            }

            AZ::TypeId patchDataTypeId = patchIt->second.type();

            // All Asset patches are typed to AZ::Data::Asset<AZ::Data::AssetData>>.
            // Convert the patch type id to the AssetClassId to properly compare against the source classElement we will be patching into.
            // Type checks against specialized Asset types: mesh, material, etc. are not done at this time
            // as the patch creation flow validates that patch data and location to patch match types.
            if (patchDataTypeId == azrtti_typeid<AZ::Data::Asset<AZ::Data::AssetData>>())
            {
                patchDataTypeId = AZ::GetAssetClassId();
            }

            if (!parentPointer)
            {
                // Since this is the root element, clone it from the patch
                return context->CloneObject(AZStd::any_cast<void>(&patchIt->second), patchDataTypeId);
            }

            // if we add an element to a container provisionally we have to remove it if we fail to deserialize and its a pointer:
            bool addedElementToContainer = false;
            // we have a patch to this node, and the PreventOverride flag is not preventing it from being applied
            if (parentClassData->m_container)
            {
                if (parentClassData->m_container->CanAccessElementsByIndex() && parentClassData->m_container->Size(parentPointer) > parentContainerElementCounter)
                {
                    targetPointer = parentClassData->m_container->GetElementByIndex(parentPointer, sourceNode->m_classElement, parentContainerElementCounter);
                }
                else
                {
                    // Allocate space in the container for our element
                    targetPointer = parentClassData->m_container->ReserveElement(parentPointer, sourceNode->m_classElement);
                    addedElementToContainer = true;
                }

                ++parentContainerElementCounter;
            }
            else
            {
                // We are stored by value, use the parent offset
                targetPointer = reinterpret_cast<char*>(parentPointer) + sourceNode->m_classElement->m_offset;
            }
            reservePointer = targetPointer;

            const TypeId& underlyingTypeId = context->GetUnderlyingTypeId(sourceNode->m_classElement->m_typeId);

            if (sourceNode->m_classElement->m_flags & SerializeContext::ClassElement::FLG_POINTER)
            {
                const SerializeContext::ClassData* patchClassData = context->FindClassData(patchDataTypeId);
                AZ::IRttiHelper* patchRtti = patchClassData ? patchClassData->m_azRtti : nullptr;
                bool canCastPatchTypeToSourceType = sourceNode->m_classElement->m_typeId == patchDataTypeId || underlyingTypeId == patchDataTypeId;
                canCastPatchTypeToSourceType = canCastPatchTypeToSourceType || (patchRtti && patchRtti->IsTypeOf(sourceNode->m_classElement->m_typeId));
                if (!canCastPatchTypeToSourceType)
                {
                    bool deprecatedWithNoConverter = patchClassData && patchClassData->IsDeprecated() && (patchClassData->m_converter == nullptr);

                    if (!deprecatedWithNoConverter)
                    {
                        ReportDataPatchMismatch(context, sourceNode->m_classElement, patchDataTypeId);
                    }
                }
                else
                {
                    // load the element
                    *reinterpret_cast<void**>(targetPointer) = context->CloneObject(AZStd::any_cast<void>(&patchIt->second), patchDataTypeId);
                }
            }
            else
            {
                if (sourceNode->m_classElement->m_typeId != patchDataTypeId && patchDataTypeId != underlyingTypeId)
                {
                    ReportDataPatchMismatch(context, sourceNode->m_classElement, patchDataTypeId);

                    return nullptr;
                }
                context->CloneObjectInplace(targetPointer, AZStd::any_cast<void>(&patchIt->second), patchDataTypeId);
            }

            if (parentClassData->m_container)
            {
                bool failedToLoad = false;
                if (sourceNode->m_classElement->m_flags & SerializeContext::ClassElement::FLG_POINTER)
                {
                    failedToLoad = (*reinterpret_cast<void**>(reservePointer)) == nullptr;
                }

                if (addedElementToContainer && failedToLoad)
                {
                    // since we added an element before we knew whether it would deserialize or not, we have to remove it here.
                    parentClassData->m_container->RemoveElement(parentPointer, targetPointer, context);
                    --parentContainerElementCounter;
                }
                else
                {
                    parentClassData->m_container->StoreElement(parentPointer, targetPointer);
                }

            }
        }
        else
        {
            bool addedElementToContainer = false;
            if (parentPointer)
            {
                if (parentClassData->m_container)
                {
                    if (parentClassData->m_container->CanAccessElementsByIndex() && parentClassData->m_container->Size(parentPointer) > parentContainerElementCounter)
                    {
                        targetPointer = parentClassData->m_container->GetElementByIndex(parentPointer, sourceNode->m_classElement, parentContainerElementCounter);
                    }
                    else
                    {
                        // Allocate space in the container for our element
                        targetPointer = parentClassData->m_container->ReserveElement(parentPointer, sourceNode->m_classElement);
                        addedElementToContainer = true;
                    }

                    ++parentContainerElementCounter;
                }
                else
                {
                    // We are stored by value, use the parent offset
                    targetPointer = reinterpret_cast<char*>(parentPointer) + sourceNode->m_classElement->m_offset;
                }

                reservePointer = targetPointer;
                // create a new instance if needed
                if (sourceNode->m_classElement->m_flags & SerializeContext::ClassElement::FLG_POINTER)
                {
                    // create a new instance if we are referencing it by pointer
                    AZ_Assert(
                        sourceNode->m_classData->m_factory != nullptr,
                        "We are attempting to create '%s', but no factory is provided! Either provide factory or change data member '%s' to value not pointer!",
                        sourceNode->m_classData->m_name,
                        sourceNode->m_classElement->m_name);
                    void* newTargetPointer = sourceNode->m_classData->m_factory->Create(sourceNode->m_classData->m_name);

                    // we need to account for additional offsets if we have a pointer to a base class.
                    void* basePtr = context->DownCast(
                        newTargetPointer,
                        sourceNode->m_classData->m_typeId,
                        sourceNode->m_classElement->m_typeId,
                        sourceNode->m_classData->m_azRtti,
                        sourceNode->m_classElement->m_azRtti);

                    AZ_Assert(basePtr != nullptr, parentClassData->m_container
                        ? "Can't cast container element %s(0x%x) to %s, make sure classes are registered in the system and not generics!"
                        : "Can't cast %s(0x%x) to %s, make sure classes are registered in the system and not generics!"
                        , sourceNode->m_classElement->m_name ? sourceNode->m_classElement->m_name : "NULL"
                        , sourceNode->m_classElement->m_nameCrc
                        , sourceNode->m_classData->m_name);

                    *reinterpret_cast<void**>(targetPointer) = basePtr; // store the pointer in the class

                    // further construction of the members need to be based off the pointer to the
                    // actual type, not the base type!
                    targetPointer = newTargetPointer;
                }
            }
            else
            {
                // this is a root element, create a new element
                targetPointer = sourceNode->m_classData->m_factory->Create(sourceNode->m_classData->m_name);
            }

            if (sourceNode->m_classData->m_eventHandler)
            {
                sourceNode->m_classData->m_eventHandler->OnWriteBegin(targetPointer);
                sourceNode->m_classData->m_eventHandler->OnPatchBegin(targetPointer, { address, patch, childPatchLookup });
            }

            int targetContainerElementCounter = 0;

            if (sourceNode->m_classData->m_serializer)
            {
                // this is leaf node copy from the source
                tmpSourceBuffer.clear();
                IO::ByteContainerStream<AZStd::vector<AZ::u8>> sourceStream(&tmpSourceBuffer);
                sourceNode->m_classData->m_serializer->Save(sourceNode->m_data, sourceStream);
                IO::MemoryStream targetStream(tmpSourceBuffer.data(), tmpSourceBuffer.size());
                sourceNode->m_classData->m_serializer->Load(targetPointer, targetStream, sourceNode->m_classData->m_version);
            }
            else if (sourceNode->m_classData->m_container)
            {
                // Traverse child elements of container
                u64 elementIndex = 0;
                u64 elementId = 0;
                for (DataNode& sourceElementNode : sourceNode->m_children)
                {
                    SerializeContext::ClassPersistentId sourcePersistentIdFunction = sourceElementNode.m_classData->GetPersistentId(*context);
                    if (sourcePersistentIdFunction)
                    {
                        // we use persistent ID for an id
                        elementId = sourcePersistentIdFunction(sourceElementNode.m_data);
                    }
                    else
                    {
                        // use index as an ID
                        elementId = elementIndex;
                    }

                    address.emplace_back(elementId,
                                         sourceElementNode.m_classData,
                                         nullptr,
                                         AddressTypeElement::ElementType::Index);

                    ApplyToElements(
                        &sourceElementNode,
                        patch,
                        childPatchLookup,
                        sourceFlagsMap,
                        targetFlagsMap,
                        addressFlags,
                        address,
                        targetPointer,
                        sourceNode->m_classData,
                        tmpSourceBuffer,
                        context,
                        filterDesc,
                        targetContainerElementCounter);

                    address.pop_back();

                    ++elementIndex;
                }

                // Find missing elements that need to be added to container (new element patches).
                // Skip this step if PreventOverride flag is preventing creation of new elements.
                if (!(addressFlags & DataPatch::Flag::PreventOverrideEffect))
                {
                    AZStd::vector<AZStd::pair<AZ::u64, AZ::TypeId>> newElementIds;
                    {
                        // Check each datapatch that matches our address + 1 address element ("possible new element datapatches")
                        auto foundIt = childPatchLookup.find(address);
                        if (foundIt != childPatchLookup.end())
                        {
                            const AZStd::vector<AddressType>& childPatches = foundIt->second;
                            for (auto& childPatchAddress : childPatches)
                            {
                                auto foundPatchIt = patch.find(childPatchAddress);
                                if (foundPatchIt != patch.end() && foundPatchIt->second.empty())
                                {
                                    continue; // this is removal of element (actual patch is empty), we already handled removed elements above
                                }

                                AZ::u64 newElementId = childPatchAddress.back().GetAddressElement();
                                const AZ::TypeId& newElementTypeId = childPatchAddress.back().GetElementTypeId();

                                elementIndex = 0;
                                bool isFound = false;
                                for (DataNode& sourceElementNode : sourceNode->m_children)
                                {
                                    SerializeContext::ClassPersistentId sourcePersistentIdFunction = sourceElementNode.m_classData->GetPersistentId(*context);
                                    if (sourcePersistentIdFunction)
                                    {
                                        // we use persistent ID for an id
                                        elementId = sourcePersistentIdFunction(sourceElementNode.m_data);
                                    }
                                    else
                                    {
                                        // use index as an ID
                                        elementId = elementIndex;
                                    }

                                    if (elementId == newElementId)
                                    {
                                        isFound = true;
                                        break;
                                    }
                                    ++elementIndex;
                                }

                                if (!isFound) // if element is not in the source container, it will be added
                                {
                                    newElementIds.push_back({ newElementId, newElementTypeId });
                                }
                            }
                        }

                        // Sort so that elements using index as ID retain relative order.
                        AZStd::sort(newElementIds.begin(), newElementIds.end());
                    }

                    // Add missing elements to container.
                    for (const AZStd::pair<AZ::u64, AZ::TypeId>& newElementId : newElementIds)
                    {
                        // pick any child element for a classElement sample
                        DataNode defaultSourceNode;
                        SerializeContext::ClassElement patchClassElement;

                        // Build a DataElement representing the type owned by the container
                        // so we can query for its ClassElement
                        SerializeContext::DataElement patchDataElement;
                        patchDataElement.m_id = newElementId.second;
                        patchDataElement.m_nameCrc = sourceNode->m_classData->m_container->GetElementNameCrC();

                        if (sourceNode->m_classData->m_container->GetElement(patchClassElement, patchDataElement))
                        {
                            defaultSourceNode.m_classElement = &patchClassElement;
                        }
                        else
                        {
                            AZStd::string sourceTypeIdName;
                            if (auto classData = context->FindClassData(sourceNode->m_classElement->m_typeId))
                            {
                                sourceTypeIdName = classData->m_name;
                            }

                            // Could not find default data type for this node
                            AZ_Warning("DataNodeTree::ApplyToElements", false,
                                "Attempted to get class element data from %s of type %s (Type Id: %s) during DataPatch::Apply and failed.\n"
                                 "The element will not be patched into the container. This is likely caused by a corrupted DataPatch on disk.\n"
                                 "It is recommended to debug the patch and source asset.",
                                sourceNode->m_classElement->m_name,
                                sourceTypeIdName.c_str(),
                                sourceNode->m_classElement->m_typeId.ToString<AZStd::string>().c_str()
                            );

                            continue;
                        }

                        // Attempt to add version and typeId information to address based on classElement
                        const AZ::SerializeContext::ClassData* elementClassData = context->FindClassData(defaultSourceNode.m_classElement->m_typeId, parentClassData, defaultSourceNode.m_classElement->m_nameCrc);

                        // Failed to find classData for container element
                        // We will be unable to version this patch element
                        AZ_Error("DataNodeTree::ApplyToElements",
                            elementClassData,
                            "Unable to find ClassData for ClassElement: %s with TypeId: %s while building patch Address during DataPatch::Apply\n"
                            "Please verify that the reported TypeId has been reflected in the serializer",
                            defaultSourceNode.m_classElement->m_name,
                            defaultSourceNode.m_classElement->m_typeId.ToString<AZStd::string>().c_str());

                        defaultSourceNode.m_classData = elementClassData;

                        address.emplace_back(newElementId.first,
                            elementClassData,
                            nullptr,
                            AddressTypeElement::ElementType::Index);

                        ApplyToElements(
                            &defaultSourceNode,
                            patch,
                            childPatchLookup,
                            sourceFlagsMap,
                            targetFlagsMap,
                            addressFlags,
                            address,
                            targetPointer,
                            sourceNode->m_classData,
                            tmpSourceBuffer,
                            context,
                            filterDesc,
                            targetContainerElementCounter);

                        address.pop_back();
                    }
                }
            }
            else
            {
                // Traverse child elements
                AZStd::unordered_set<u64> parsedElementIds;
                auto sourceElementIt = sourceNode->m_children.begin();
                while (sourceElementIt != sourceNode->m_children.end())
                {
                    // Use class element name as an ID
                    address.emplace_back(sourceElementIt->m_classElement->m_nameCrc,
                                         sourceElementIt->m_classData,
                                         sourceElementIt->m_classElement,
                                         AddressTypeElement::ElementType::Class);

                    parsedElementIds.emplace(address.back().GetAddressElement());
                    ApplyToElements(
                        &(*sourceElementIt),
                        patch,
                        childPatchLookup,
                        sourceFlagsMap,
                        targetFlagsMap,
                        addressFlags,
                        address,
                        targetPointer,
                        sourceNode->m_classData,
                        tmpSourceBuffer,
                        context,
                        filterDesc,
                        targetContainerElementCounter);

                    address.pop_back();

                    ++sourceElementIt;
                }

                // Find missing elements that need to be added to structure.
                // \note check performance, tag new elements to improve it.
                // Skip this step if PreventOverride flag is preventing creation of new elements.
                if (!(addressFlags & DataPatch::Flag::PreventOverrideEffect))
                {
                    AZStd::vector<u64> newElementIds;
                    auto foundIt = childPatchLookup.find(address);
                    if (foundIt != childPatchLookup.end())
                    {
                        const AZStd::vector<AddressType>& childPatches = foundIt->second;
                        for (auto& childPatchAddress : childPatches)
                        {
                            auto foundPatchIt = patch.find(childPatchAddress);
                            if (foundPatchIt != patch.end() && foundPatchIt->second.empty())
                            {
                                continue; // this is removal of element (actual patch is empty), we already handled removed elements above
                            }

                            u64 newElementId = childPatchAddress.back().GetAddressElement();

                            if (parsedElementIds.count(newElementId) == 0)
                            {
                                newElementIds.push_back(newElementId);
                            }
                        }
                    }

                    // Add missing elements to class.
                    for (u64 newElementId : newElementIds)
                    {
                        DataNode defaultSourceNode;
                        for (const auto& classElement : sourceNode->m_classData->m_elements)
                        {
                            if (classElement.m_nameCrc == static_cast<AZ::u32>(newElementId))
                            {
                                defaultSourceNode.m_classElement = &classElement;

                                // Attempt to add version and typeId information to address based on classElement
                                const AZ::SerializeContext::ClassData* elementClassData = context->FindClassData(defaultSourceNode.m_classElement->m_typeId, parentClassData, defaultSourceNode.m_classElement->m_nameCrc);

                                // Failed to find classData for container element
                                // We will be unable to version this patch element
                                AZ_Error("DataNodeTree::ApplyToElements",
                                    elementClassData,
                                    "Unable to find ClassData for ClassElement: %s with TypeId: %s while building patch Address during DataPatch::Apply\n"
                                    "Please verify that the reported TypeId has been reflected in the serializer",
                                    defaultSourceNode.m_classElement->m_name,
                                    defaultSourceNode.m_classElement->m_typeId.ToString<AZStd::string>().c_str());

                                defaultSourceNode.m_classData = elementClassData;

                                address.emplace_back(newElementId,
                                                     elementClassData,
                                                     nullptr,
                                                     AddressTypeElement::ElementType::Index);

                                ApplyToElements(
                                    &defaultSourceNode,
                                    patch,
                                    childPatchLookup,
                                    sourceFlagsMap,
                                    targetFlagsMap,
                                    addressFlags,
                                    address,
                                    targetPointer,
                                    sourceNode->m_classData,
                                    tmpSourceBuffer,
                                    context,
                                    filterDesc,
                                    targetContainerElementCounter);

                                address.pop_back();
                                break;
                            }
                        }
                    }
                }
            }

            if (sourceNode->m_classData->m_eventHandler)
            {
                sourceNode->m_classData->m_eventHandler->OnPatchEnd(targetPointer, { address, patch, childPatchLookup });
                sourceNode->m_classData->m_eventHandler->OnWriteEnd(targetPointer);
            }

            if (parentPointer && parentClassData->m_container)
            {
                bool failedToLoad = false;
                if (sourceNode->m_classElement->m_flags & SerializeContext::ClassElement::FLG_POINTER)
                {
                    failedToLoad = (*reinterpret_cast<void**>(reservePointer)) == nullptr;
                }

                if (addedElementToContainer && failedToLoad)
                {
                    // since we added an element before we knew whether it would deserialize or not, we have to remove it here.
                    parentClassData->m_container->RemoveElement(parentPointer, reservePointer, context);
                    --parentContainerElementCounter;
                }
                else
                {
                    parentClassData->m_container->StoreElement(parentPointer, reservePointer);
                }
            }
        }

        return targetPointer;
    }

    //! The following code is to catch a legacy DataPatch case scenario where a bool is being loaded from a legacy data patch
    //! but the current type has been updated to be an enum type or an integral type.
    //! This updates the legacy DataPatch byte stream with the current enum type data
    static bool ConvertLegacyBoolToEnum(AZStd::any& patchAny, const DataNode& sourceNode)
    {
        bool loadedTypeMatches = true;
        if (sourceNode.m_classElement)
        {
            const AZ::TypeId& loadedType = patchAny.type();

            // Validate that the types either directly match or Rtti confirms that one type derives from the other
            loadedTypeMatches = (loadedType == sourceNode.m_classElement->m_typeId ||
                (sourceNode.m_classElement->m_azRtti && sourceNode.m_classElement->m_azRtti->IsTypeOf(loadedType)));
        }

        if (!loadedTypeMatches)
        {
            bool* legacyBool = AZStd::any_cast<bool>(&patchAny);

            if (legacyBool)
            {
                if (azrtti_typeid<int8_t>() == sourceNode.m_classElement->m_typeId)
                {
                    patchAny = static_cast<int8_t>(*legacyBool);
                    return true;
                }
                else if (azrtti_typeid<uint8_t>() == sourceNode.m_classElement->m_typeId)
                {
                    patchAny = static_cast<uint8_t>(*legacyBool);
                    return true;
                }
                else if (azrtti_typeid<int16_t>() == sourceNode.m_classElement->m_typeId)
                {
                    patchAny = static_cast<int16_t>(*legacyBool);
                    return true;
                }
                else if (azrtti_typeid<uint16_t>() == sourceNode.m_classElement->m_typeId)
                {
                    patchAny = static_cast<uint16_t>(*legacyBool);
                    return true;
                }
                else if (azrtti_typeid<int32_t>() == sourceNode.m_classElement->m_typeId)
                {
                    patchAny = static_cast<int32_t>(*legacyBool);
                    return true;
                }
                else if (azrtti_typeid<uint32_t>() == sourceNode.m_classElement->m_typeId)
                {
                    patchAny = static_cast<uint32_t>(*legacyBool);
                    return true;
                }

                else if (azrtti_typeid<int64_t>() == sourceNode.m_classElement->m_typeId)
                {
                    patchAny = static_cast<int64_t>(*legacyBool);
                    return true;
                }
                else if (azrtti_typeid<uint64_t>() == sourceNode.m_classElement->m_typeId)
                {
                    patchAny = static_cast<uint64_t>(*legacyBool);
                    return true;
                }
                else
                {
                    AZ_Warning("Serialization",
                        false,
                        "Unable to convert legacy boolean patch data into an enum value during DataPatch::Apply.\n"
                        "This is likely due to a previously made type change on element: %s from bool to type with Id: %s.\n",
                        sourceNode.m_classElement->m_name,
                        sourceNode.m_classElement->m_typeId.ToString<AZStd::string>().c_str());

                    return false;
                }
            }
        }

        return false;
    }

    //=========================================================================
    // LoadStreamWrapper
    //=========================================================================
    bool DataNodeTree::LoadInPlaceStreamWrapper(AZ::SerializeContext& context, AZStd::any& patchAny, const DataNode& sourceNode, const AZ::SerializeContext::ClassData* parentClassData, const AZ::ObjectStream::FilterDescriptor& filterDesc)
    {
        // Pull the StreamWrapper object out of the provided AZStd::any
        const DataPatch::LegacyStreamWrapper* wrapper = AZStd::any_cast<DataPatch::LegacyStreamWrapper>(&patchAny);
        bool loadSuccess = false;

        if (wrapper && !wrapper->m_stream.empty())
        {
            // Build a stream from the wrapper data
            DataPatch::LegacyStreamWrapper byteStreamPatchWrapper = AZStd::move(*wrapper);
            AZ::IO::MemoryStream stream(byteStreamPatchWrapper.m_stream.data(), byteStreamPatchWrapper.m_stream.size());

            AZ::Uuid loadedTypeId = sourceNode.m_classElement->m_typeId;

            bool classDataRequested = false;
            loadSuccess = ObjectStream::LoadBlocking(&stream, context, ObjectStream::ClassReadyCB(), filterDesc,
                [&sourceNode, parentClassData, &loadedTypeId, &classDataRequested, &patchAny](void** rootAddress, const SerializeContext::ClassData** classData, const Uuid& typeId, SerializeContext* sc)
            {
                if (!classDataRequested)
                {
                    loadedTypeId = typeId;
                }

                // Convert our patchAny to the specified type
                // Provide its data address to be written to during load
                // Load into same any that held the StreamWrapper to mitigate making AZStd::any temporaries and copies
                // Which can be expensive
                if (rootAddress)
                {
                    patchAny = sc->CreateAny(loadedTypeId);
                    *rootAddress = AZStd::any_cast<void>(&patchAny);
                }

                // Specify the classData from our sourceNode
                if (classData)
                {
                    *classData = sourceNode.m_classData;
                    if (!*classData)
                    {
                        if (sourceNode.m_classElement)
                        {
                            *classData = sc->FindClassData(
                                sourceNode.m_classElement->m_typeId,
                                parentClassData,
                                sourceNode.m_classElement->m_nameCrc);
                        }
                    }
                    if (*classData)
                    {
                        loadedTypeId = (*classData)->m_typeId;
                        classDataRequested = true;
                    }
                }
            });

            // If we failed to load then restore patchAny to retain the original streamWrapper
            // Otherwise we are at risk of saving out a corrupted patch and lose the original data
            if (!loadSuccess)
            {
                patchAny = AZStd::move(byteStreamPatchWrapper);
            }
            else
            {
                // Check the scenario where a bool is being loaded from a legacy data patch
                // but the current type has been updated to be an enum type or an integral type
                ConvertLegacyBoolToEnum(patchAny, sourceNode);
            }
        }

        return loadSuccess;
    }

    //=========================================================================
    // CreateDataPatchAny
    //=========================================================================
    bool DataNodeTree::CreateDataPatchAny(AZ::SerializeContext& serializeContext, const void* classPtr, const AZ::TypeId& typeId, AZStd::any& dataPatchAnyDest)
    {
        // Build an AZStd::any from provided typeId and then clone into it
        dataPatchAnyDest = serializeContext.CreateAny(typeId);

        // If the patch is empty we will be unable to clone classPtr into it
        // This likely occured because typeId was not reflected properly or is a generic typeId
        if (dataPatchAnyDest.empty())
        {
            return false;
        }

        serializeContext.CloneObjectInplace(AZStd::any_cast<void>(&dataPatchAnyDest), classPtr, typeId);

        return true;
    }

    //=========================================================================
    // CalculateDataFlagsAtThisAddress
    //=========================================================================
    DataPatch::Flags DataNodeTree::CalculateDataFlagsAtThisAddress(
        const DataPatch::FlagsMap& sourceFlagsMap,
        const DataPatch::FlagsMap& targetFlagsMap,
        DataPatch::Flags parentAddressFlags,
        const AddressType& address)
    {
        DataPatch::Flags flags = DataPatch::GetEffectOfParentFlagsOnThisAddress(parentAddressFlags);

        auto foundSourceFlags = sourceFlagsMap.find(address);
        if (foundSourceFlags != sourceFlagsMap.end())
        {
            flags |= DataPatch::GetEffectOfSourceFlagsOnThisAddress(foundSourceFlags->second);
        }

        auto foundTargetFlags = targetFlagsMap.find(address);
        if (foundTargetFlags != targetFlagsMap.end())
        {
            flags |= DataPatch::GetEffectOfTargetFlagsOnThisAddress(foundTargetFlags->second);
        }

        return flags;
    }

    inline namespace DataPatchInternal
    {
        //=========================================================================
        // AddressTypeElement Crc32 constructor
        //=========================================================================
        AddressTypeElement::AddressTypeElement()
        {
            // If we cannot acquire a version number we should set it to a value we will not attempt to version convert
            // This could still represent a valid AddressTypeElement as long as we read a valid address element
            constexpr AZ::u32 unConvertableVersionVal = (std::numeric_limits<AZ::u32>::max)();

            // If we cannot acquire an addressElement this is an error and we will mark it with max of u64 and m_isValid = false
            constexpr AZ::u64 addressElementErrorVal = (std::numeric_limits<AZ::u64>::max)();

            m_isValid = false;
            m_addressClassVersion = unConvertableVersionVal;
            m_addressElement = addressElementErrorVal;
            m_addressClassTypeId = Uuid::CreateNull();
        }

        AddressTypeElement::AddressTypeElement(const AZ::Crc32 addressElement, const AZ::SerializeContext::ClassData* classData, const AZ::SerializeContext::ClassElement* classElement, AddressTypeElement::ElementType elementType)
            : AddressTypeElement(static_cast<AZ::u64>(addressElement), classData, classElement, elementType)
        {
        }

        //=========================================================================
        // AddressTypeElement u64 constructor
        //=========================================================================
        AddressTypeElement::AddressTypeElement(const AZ::u64 addressElement, const AZ::SerializeContext::ClassData* classData, const AZ::SerializeContext::ClassElement* classElement, AddressTypeElement::ElementType elementType)
            : m_addressElement(addressElement)
        {
            m_addressClassVersion = classData ? classData->m_version : std::numeric_limits<AZ::u32>::max();
            m_addressClassTypeId = classData ? classData->m_typeId : AZ::Uuid::CreateNull();

            // Fill out m_pathElement based on supplied data and ElementType
            if (classData && classElement && elementType == ElementType::Class)
            {
                AZ_Error("Serialization",
                    AZStd::string(classData->m_name).find(VersionDelimiter) == AZStd::string::npos,
                    "Detected elementName containing restricted character \"%s\" within classElement: %s owned by class: %s while building an address element for a DataPatch\n"
                    "This likely occured while making an override on a slice instance in the Editor\n"
                    "If this change is pushed to slice the override will fail to load back in\n"
                    "It is recommended to remove the \"%s\" character from the identified classElement where it is reflected to the serializer",
                    VersionDelimiter,
                    classElement->m_name,
                    classData->m_name,
                    VersionDelimiter);

                // className(class typeId)::elementName<versionDelimiter>version<pathDelimiter>
                m_pathElement = AZStd::string::format("%s(%s)::%s%s%u%s", classData->m_name, m_addressClassTypeId.ToString<AZStd::string>().c_str(), classElement->m_name, VersionDelimiter, m_addressClassVersion, PathDelimiter);
            }
            else if (classData && elementType == ElementType::Index)
            {
                // className(class typeId)#containerId<versionDelimiter>version<pathDelimiter>
                m_pathElement = AZStd::string::format("%s(%s)#%" PRIu64 "%s%u%s", classData->m_name, m_addressClassTypeId.ToString<AZStd::string>().c_str(), static_cast<uint64_t>(m_addressElement), VersionDelimiter, m_addressClassVersion, PathDelimiter);
            }
            else if (elementType == ElementType::None)
            {
                // addressElement/
                m_pathElement = AZStd::to_string(addressElement) + PathDelimiter;
            }
        }

        AddressTypeElement::AddressTypeElement(const AZStd::string& newElementName, const AZ::u32 newElementVersion, const AddressTypeElement& original)
            : m_addressElement(AZ::u64(AZ::Crc32(newElementName)))
            , m_addressClassTypeId(original.m_addressClassTypeId)
            , m_addressClassVersion(newElementVersion)
            , m_isValid(true)
        {
            AZ::SerializeContext* context;
            ComponentApplicationBus::BroadcastResult(context, &ComponentApplicationBus::Events::GetSerializeContext);

            auto classData = context->FindClassData(original.GetElementTypeId());

            m_pathElement = AZStd::string::format("%s(%s)::%s%s%u%s", classData->m_name, m_addressClassTypeId.ToString<AZStd::string>().c_str(), newElementName.data(), AddressTypeElement::VersionDelimiter, newElementVersion, AddressTypeElement::PathDelimiter);
        }

        AddressTypeElement::AddressTypeElement(const AZ::TypeId& newTypeId, const AZ::u32 newElementVersion, const AddressTypeElement& original)
            : m_addressElement(original.m_addressElement)
            , m_pathElement(original.m_pathElement)
            , m_addressClassTypeId(newTypeId)
            , m_addressClassVersion(newElementVersion)
            , m_isValid(true)
        {}

        bool operator==(const AddressTypeElement& lhs, const AddressTypeElement&rhs)
        {
            return lhs.GetAddressElement() == rhs.GetAddressElement();
        }

        bool operator!=(const AddressTypeElement& lhs, const AddressTypeElement&rhs)
        {
            return !operator==(lhs, rhs);
        }

        void AddressTypeSerializer::LoadElementFinalize(AddressTypeElement& addressElement, const AZStd::string& pathElement, const AZStd::string& version) const
        {
            if (!addressElement.m_addressClassTypeId.IsNull())
            {
                addressElement.m_isValid = true;
            }

            addressElement.m_addressClassVersion = static_cast<AZ::u32>(AZStd::stoul(version));
            addressElement.m_pathElement = pathElement;
        }

        void AddressTypeSerializer::LoadLegacyElement(AddressTypeElement& addressElement, const AZStd::string& pathElement, const size_t pathDelimLength) const
        {
            const AZStd::string& element = pathElement.substr(0, pathElement.size() - pathDelimLength);
            for (const char digit : element)
            {
                // If an address is in this format it means its address was saved to disk after type conversion
                // but before it was filled out with typeId and version data by being applied once
                // In this format we expect the whole path element to be numerical except for the trailing /
                if (!AZStd::is_digit(digit))
                {
                    addressElement.m_pathElement = pathElement;
                    return;
                }
            }
            addressElement.m_addressElement = AZStd::stoull(element);
            addressElement.m_pathElement = pathElement;
            addressElement.m_isValid = true;
        }

        AddressTypeElement AddressTypeSerializer::LoadAddressElementFromPath(const AZStd::string& pathElement) const
        {
            AZ_PROFILE_FUNCTION(AzCore);

            // AddressTypeElement default constructor defaults to an invalid addressElement
            AddressTypeElement addressElement;

            DataPatchInternal::AddressTypeElement::ElementType elementType = AddressTypeElement::ElementType::None;
            constexpr const char* typeIdOpenDelim = "({";
            constexpr const char* typeIdCloseDelim = "})";
            constexpr const char* classTypeElemDelim = "::";
            constexpr const char* indexTypeElemDelim = "#";

            const size_t    typeIdOpenDelimLength = strlen(typeIdOpenDelim);
            const size_t    typeIdCloseDelimLength = strlen(typeIdCloseDelim);
            const size_t    classTypeElemDelimLength = strlen(classTypeElemDelim);
            const size_t    indexTypeElemDelimLength = strlen(indexTypeElemDelim);
            const size_t    versionDelimLength = strlen(AddressTypeElement::VersionDelimiter);
            const size_t    pathDelimLength = strlen(AddressTypeElement::PathDelimiter);

            const AZStd::string_view pathElementView(pathElement);
            const size_t typeIdOpen = pathElementView.find(typeIdOpenDelim);

            // Early out on the legacy case. 
            // If no expected formatting is found then check for <someNumber>/.
            // if found then use <someNumber> as the addressElement
            if (typeIdOpen == AZStd::string::npos)
            {
                LoadLegacyElement(addressElement, pathElement, pathDelimLength);
                return addressElement;
            }

            const size_t typeIdClosed = pathElementView.find(typeIdCloseDelim, typeIdOpen);
            size_t elementMarker = pathElementView.find(classTypeElemDelim, typeIdClosed);

            if (elementMarker == AZStd::string::npos)
            {
                elementType = AddressTypeElement::ElementType::Index;
                elementMarker = pathElementView.find(indexTypeElemDelim, typeIdClosed);
            }
            else
            {
                elementType = AddressTypeElement::ElementType::Class;
            }

            // Every find chains from the previous: find(substr, lastFindPosition)
            // if versionMarker != npos then all markers are != npos
            // This way we save on find times and do not require an npos check on each
            size_t versionMarker = pathElementView.find(AddressTypeElement::VersionDelimiter, elementMarker);

            // Validate that we have found our appropriate markers and that version marker occurs after elementMarker.
            // Also validate that elementMarker occurs directly after bracketClosed
            if (versionMarker != AZStd::string::npos && (elementMarker - typeIdClosed) == typeIdCloseDelimLength)
            {
                const AZStd::string_view typeId = pathElementView.substr(typeIdOpen + typeIdOpenDelimLength, typeIdClosed - typeIdOpen - typeIdOpenDelimLength);

                if (elementType == AddressTypeElement::ElementType::Class)
                {
                    const AZStd::string_view element = pathElementView.substr(elementMarker + classTypeElemDelimLength, versionMarker - elementMarker - classTypeElemDelimLength);
                    const AZStd::string version = pathElement.substr(versionMarker + versionDelimLength, pathElement.size() - versionMarker - versionDelimLength - pathDelimLength);

                    // Address Element is the CRC of the elementName
                    addressElement.m_addressElement = AZ_CRC(element);
                    addressElement.m_addressClassTypeId = AZ::TypeId(typeId.data(), typeId.size());

                    LoadElementFinalize(addressElement, pathElement, version);
                    return addressElement;
                }
                else if (elementType == AddressTypeElement::ElementType::Index)
                {
                    const AZStd::string element = pathElement.substr(elementMarker + indexTypeElemDelimLength, versionMarker - elementMarker - indexTypeElemDelimLength);
                    const AZStd::string version = pathElement.substr(versionMarker + versionDelimLength, pathElement.size() - versionMarker - versionDelimLength - pathDelimLength);

                    addressElement.m_addressElement = AZStd::stoull(element);
                    addressElement.m_addressClassTypeId = AZ::TypeId(typeId.data(), typeId.size());

                    LoadElementFinalize(addressElement, pathElement, version);
                    return addressElement;
                }
            }

            return addressElement;
        }

        /// Load the class data from a stream.
        bool AddressTypeSerializer::Load(void* classPtr, IO::GenericStream& stream, unsigned int version, bool isDataBigEndian /*= false*/)
        {
            AZ_PROFILE_FUNCTION(AzCore);
            (void)isDataBigEndian;
            constexpr unsigned int version1PathAddress = 1;

            if (version < version1PathAddress)
            {
                AZ_PROFILE_SCOPE(AzCore, "AddressTypeSerializer::Load::LegacyUpgrade");
                // Grab the AddressType object to be filled
                AddressType* address = reinterpret_cast<AddressType*>(classPtr);
                address->clear();

                address->m_isLegacy = true;

                // Determine size of LegacyAddressType being read in
                // LegacyAddressType inherited from an AZStd::vector<u64>
                size_t dataSize = static_cast<size_t>(stream.GetLength());
                size_t numElements = dataSize / sizeof(u64);

                // Store LegacyAddressType in a temp
                AZStd::vector<u64> legacyAddress;
                legacyAddress.resize_no_construct(numElements);
                stream.Read(dataSize, legacyAddress.data());

                // Add Each LegacyAddressElement to AddressType v1 structure
                for (u64 legacyAddressElement : legacyAddress)
                {
                    address->emplace_back(legacyAddressElement);
                }
            }
            else
            {
                AZ_PROFILE_SCOPE(AzCore, "AddressTypeSerializer::Load::CurrentFlow");
                // Grab the AddressType object to be filled
                AddressType* address = reinterpret_cast<AddressType*>(classPtr);
                address->clear();

                // Determine size of AddressType being read in
                size_t dataSize = static_cast<size_t>(stream.GetLength());

                // An empty address path denotes the root element
                if (dataSize == 0)
                {
                    return true;
                }

                // Store the stream in a temp string
                AZStd::string streamData;
                streamData.resize_no_construct(dataSize);

                stream.Read(dataSize, streamData.data());

                bool addressIsInvalid = false;
                size_t elementWalker = streamData.find(AddressTypeElement::PathDelimiter);

                // Used to verify that we loaded the address in its entirety
                size_t loadedAddressLength = 0;

                // Invalid path if it does not contain the m_PathDelimiter delimiter
                if (elementWalker == AZStd::string::npos)
                {
                    addressIsInvalid = true;
                }
                else
                {
                    const size_t pathDelimLength = strlen(AddressTypeElement::PathDelimiter);
                    // Walk the path and build an AddressTypeElement per delimited block
                    size_t startingPos = 0;
                    while (elementWalker != AZStd::string::npos)
                    {
                        address->emplace_back(LoadAddressElementFromPath(streamData.substr(startingPos, (elementWalker - startingPos) + pathDelimLength)));

                        // Check to see if our added AddressTypeElement was formed correctly
                        if (!address->back().IsValid())
                        {
                            addressIsInvalid = true;
                            break;
                        }

                        loadedAddressLength += address->back().GetPathElement().size();
                        startingPos = elementWalker + pathDelimLength;
                        elementWalker = streamData.find(AddressTypeElement::PathDelimiter, startingPos);
                    }
                }

                // Confirm that no individual element was invalid and that we loaded the whole address
                if (addressIsInvalid || loadedAddressLength != streamData.size())
                {
                    AZ_Error("DataPatch::AddressTypeSerializer::Load",
                        false,
                        "DataPatch AddressType: %s failed to load.\n"
                        "This could be caused by a corrupted DataPatch AddressType on disk.\n"
                        "DataPatch owning this address will fail to apply.",
                        streamData.c_str());

                    address->clear();
                    address->m_isValid = false;
                    return false;
                }
            }

            return true;
        }

        /// Store the class data into a stream.
        size_t AddressTypeSerializer::Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian /*= false*/)
        {
            (void)isDataBigEndian;
            const  AddressType* container = reinterpret_cast<const AddressType*>(classPtr);

            AZStd::string finalPath;
            for (const AddressTypeElement& element : *container)
            {
                finalPath += element.GetPathElement();
            }

            return static_cast<size_t>(stream.Write(finalPath.size(), finalPath.data()));
        }

        size_t AddressTypeSerializer::DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian)
        {
            AZ::Internal::AZStdString<AZStd::string> stringSerializer;

            return stringSerializer.DataToText(in, out, isDataBigEndian);
        }

        size_t AddressTypeSerializer::TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian /*= false*/)
        {
            if (textVersion < 1)
            {
                return AZ::Internal::AZBinaryData::TextToData(text, textVersion, stream, isDataBigEndian);
            }
            else
            {
                // Version passed in needs to be valid for the serializer used. String serializer currently does not use version
                static constexpr uint32_t stringSerializerVersion = 0;
                AZ::Internal::AZStdString<AZStd::string> stringSerializer;

                return stringSerializer.TextToData(text, stringSerializerVersion, stream, isDataBigEndian);
            }
        }

        bool AddressTypeSerializer::CompareValueData(const void* lhs, const void* rhs)
        {
            return SerializeContext::EqualityCompareHelper<AddressType>::CompareValues(lhs, rhs);
        }
    }

    //=========================================================================
    // GetEffectOfParentFlagsOnThisAddress
    //=========================================================================
    DataPatch::Flags DataPatch::GetEffectOfParentFlagsOnThisAddress(Flags flagsAtParentAddress)
    {
        Flags flagsAtDescendantAddress = 0;

        // currently, all "effect" flags are passed down from parent address
        const Flags inheritFlagsFromParentMask = Flag::EffectMask;

        flagsAtDescendantAddress |= flagsAtParentAddress & inheritFlagsFromParentMask;

        return flagsAtDescendantAddress;
    }

    //=========================================================================
    // GetEffectOfSourceFlagsOnThisAddress
    //=========================================================================
    DataPatch::Flags DataPatch::GetEffectOfSourceFlagsOnThisAddress(Flags flagsAtSourceAddress)
    {
        Flags flagsAtChildAddress = 0;

        // PreventOverride prevents targets from overriding data that comes from the source.
        // This "effect" is passed along to all "descendants" of the data (ex: deeply nested slices).
        if (flagsAtSourceAddress & (Flag::PreventOverrideSet | Flag::PreventOverrideEffect))
        {
            flagsAtChildAddress |= Flag::PreventOverrideEffect;
        }

        if (flagsAtSourceAddress & (Flag::HidePropertySet | Flag::HidePropertyEffect))
        {
            flagsAtChildAddress |= Flag::HidePropertyEffect;
        }

        return flagsAtChildAddress;
    }

    //=========================================================================
    // GetEffectOfTargetFlagsOnThisAddress
    //=========================================================================
    DataPatch::Flags DataPatch::GetEffectOfTargetFlagsOnThisAddress(Flags flagsAtTargetAddress)
    {
        Flags flags = flagsAtTargetAddress;

        // ForceOverride forces the target to override data from its source.
        // This effect begins at the address it's set on and is passed down to child addresses in the data hierarchy.
        if (flags & Flag::ForceOverrideSet)
        {
            flags |= Flag::ForceOverrideEffect;
        }

        return flags;
    }

    //=========================================================================
    // DataPatch
    //=========================================================================
    DataPatch::DataPatch()
    {
        m_targetClassId = Uuid::CreateNull();
        m_targetClassVersion = (std::numeric_limits<AZ::u32>::max)();
    }

    //=========================================================================
    // DataPatch
    //=========================================================================
    DataPatch::DataPatch(const DataPatch& rhs)
    {
        m_patch = rhs.m_patch;
        m_targetClassId = rhs.m_targetClassId;
        m_targetClassVersion = rhs.m_targetClassVersion;
    }

    //=========================================================================
    // DataPatch
    //=========================================================================
    DataPatch::DataPatch(DataPatch&& rhs)
    {
        m_patch = AZStd::move(rhs.m_patch);
        m_targetClassId = AZStd::move(rhs.m_targetClassId);
        m_targetClassVersion = AZStd::move(rhs.m_targetClassVersion);
    }

    //=========================================================================
    // operator=
    //=========================================================================
    DataPatch& DataPatch::operator = (DataPatch&& rhs)
    {
        m_patch = AZStd::move(rhs.m_patch);
        m_targetClassId = AZStd::move(rhs.m_targetClassId);
        m_targetClassVersion = AZStd::move(rhs.m_targetClassVersion);
        return *this;
    }

    //=========================================================================
    // operator=
    //=========================================================================
    DataPatch& DataPatch::operator = (const DataPatch& rhs)
    {
        m_patch = rhs.m_patch;
        m_targetClassId = rhs.m_targetClassId;
        m_targetClassVersion = rhs.m_targetClassVersion;
        return *this;
    }

    //=========================================================================
    // Create
    //=========================================================================
    bool DataPatch::Create(
        const void* source,
        const Uuid& sourceClassId,
        const void* target,
        const Uuid& targetClassId,
        const FlagsMap& sourceFlagsMap,
        const FlagsMap& targetFlagsMap,
        SerializeContext* context)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        if (!source || !target)
        {
            AZ_Error("Serialization", false, "Can't generate a patch with invalid input source %p and target %p\n", source, target);
            return false;
        }

        if (!context)
        {
            EBUS_EVENT_RESULT(context, ComponentApplicationBus, GetSerializeContext);
            if (!context)
            {
                AZ_Error("Serialization", false, "Not serialize context provided! Failed to get component application default serialize context! ComponentApp is not started or input serialize context should not be null!");
                return false;
            }
        }

        if (!context->FindClassData(sourceClassId))
        {
            AZ_Error("Serialization", false, "Can't find class data for the source type Uuid %s.", sourceClassId.ToString<AZStd::string>().c_str());
            return false;
        }

        const SerializeContext::ClassData* targetClassData = context->FindClassData(targetClassId);
        if (!targetClassData)
        {
            AZ_Error("Serialization", false, "Can't find class data for the target type Uuid %s.", sourceClassId.ToString<AZStd::string>().c_str());
            return false;
        }

        m_patch.clear();
        m_targetClassId = targetClassId;
        m_targetClassVersion = targetClassData->m_version;

        if (sourceClassId != targetClassId)
        {
            // Store the entire target class in an AZStd::any and place into the PatchMap
            auto insertResult = m_patch.insert_key(AddressType());
            bool createAnyResult = DataNodeTree::CreateDataPatchAny(*context, target, targetClassId, insertResult.first->second);
            AZ_UNUSED(createAnyResult);

            AZ_Assert(createAnyResult, "Unable to store class %s, CreateDataPatchAny Failed. Verify that TypeId %s is properly reflected and is not a generic TypeId",
                targetClassData->m_name, targetClassData->m_typeId.ToString<AZStd::string>().c_str());
        }
        else
        {
            // Build the tree for the course and compare it against the target
            DataNodeTree sourceTree(context);
            sourceTree.Build(source, sourceClassId);

            DataNodeTree targetTree(context);
            targetTree.Build(target, targetClassId);

            {
                AZ_PROFILE_SCOPE(AzCore, "DataPatch::Create:RecursiveCallToCompareElements");

                sourceTree.CompareElements(
                    &sourceTree.m_root,
                    &targetTree.m_root,
                    m_patch,
                    sourceFlagsMap,
                    targetFlagsMap,
                    context);
            }
        }
        return true;
    }

    //=========================================================================
    // Apply
    //=========================================================================
    void* DataPatch::Apply(
        const void* source,
        const Uuid& sourceClassId,
        SerializeContext* context,
        const AZ::Utils::FilterDescriptor& filterDesc,
        const FlagsMap& sourceFlagsMap,
        const FlagsMap& targetFlagsMap) const
    {
        AZ_PROFILE_FUNCTION(AzCore);

        if (!source)
        {
            AZ_Error("Serialization", false, "Can't apply patch to invalid source %p\n", source);
            return nullptr;
        }

        if (!context)
        {
            EBUS_EVENT_RESULT(context, ComponentApplicationBus, GetSerializeContext);
            if (!context)
            {
                AZ_Error("Serialization", false, "No serialize context provided! Failed to get component application default serialize context! ComponentApp is not started or input serialize context should not be null!");
                return nullptr;
            }
        }

        if (m_patch.empty())
        {
            // If no patch just clone the object
            return context->CloneObject(source, sourceClassId);
        }

        if (m_patch.size() == 1 && m_patch.begin()->first.empty() && m_patch.begin()->first.IsValid())  // if we replace the root element
        {
            return context->CloneObject(AZStd::any_cast<void>(&m_patch.begin()->second), m_patch.begin()->second.type());
        }

        DataNodeTree sourceTree(context);
        sourceTree.Build(source, sourceClassId);

        AddressType address;
        AZStd::vector<AZ::u8> tmpSourceBuffer;
        void* result;

        // Copy the patch so we can repair it before application.
        decltype(m_patch) fixedPatch;

        {
            // Loop over the original data patch and make a copy of the key value pair
            AZ_PROFILE_SCOPE(AzCore, "DataPatch::Apply:UpgradeDataPatch");
            // Copy of the patch element is purposefully being created here(notice no ampersand) so that the UpgradeDataPatch
            // function can modify the key and insert it into the fixed patch map
            for (PatchMap::value_type patch : m_patch)
            {
                DataPatchUpgradeManager::UpgradeDataPatch(context, m_targetClassId, m_targetClassVersion, patch.first, patch.second);
                fixedPatch.insert(AZStd::move(patch));
            }
        }

        // Build a mapping of child patches for quick look-up: [parent patch address] -> [list of patches for child elements (parentAddress + one more address element)]
            ChildPatchMap childPatchMap;
        {
            AZ_PROFILE_SCOPE(AzCore, "DataPatch::Apply:GenerateChildPatchMap");
            for (auto& patch : fixedPatch)
            {
                AddressType parentAddress = patch.first;
                if (parentAddress.empty())
                {
                    const char* sourceClassName = sourceTree.m_root.m_classData && sourceTree.m_root.m_classData->m_name
                        ? sourceTree.m_root.m_classData->m_name : "Unknown Class Name";
                    AZ_UNUSED(sourceClassName);
                    AZ_Error("Serialization", false, "Attempting to apply DataPatch has been aborted. The Patch contains an empty address so there is nothing to patch."
                        " The source object(Class: %s) has not been modified", sourceClassName);
                    return nullptr;
                }
                if (!parentAddress.IsValid())
                {
                    const char* sourceClassName = sourceTree.m_root.m_classData && sourceTree.m_root.m_classData->m_name
                        ? sourceTree.m_root.m_classData->m_name : "Unknown Class Name";
                    AZ_UNUSED(sourceClassName);
                    AZ_Error("Serialization", false, "Attempting to apply DataPatch has been aborted . The Patch contains an invalid address to the patch data."
                        " The source object(Class: %s) has not been modified", sourceClassName);
                    return nullptr;
                }

                parentAddress.pop_back();
                auto foundIt = childPatchMap.find(parentAddress);
                if (foundIt != childPatchMap.end())
                {
                    foundIt->second.push_back(patch.first);
                }
                else
                {
                    AZStd::vector<AddressType> newChildPatchCollection;
                    newChildPatchCollection.push_back(patch.first);
                    childPatchMap[parentAddress] = AZStd::move(newChildPatchCollection);
                }
            }
        }
        {
            AZ_PROFILE_SCOPE(AzCore, "DataPatch::Apply:RecursiveCallToApplyToElements");
            int rootContainerElementCounter = 0;

            result = DataNodeTree::ApplyToElements(
                &sourceTree.m_root,
                fixedPatch,
                childPatchMap,
                sourceFlagsMap,
                targetFlagsMap,
                0,
                address,
                nullptr,
                nullptr,
                tmpSourceBuffer,
                context,
                filterDesc,
                rootContainerElementCounter);
        }
        return result;
    }

    /**
    * Helper method to convert over the legacy bytestream format to using AZStd::any to store patch data
    */
    AZ::Outcome<void, AZStd::string> ConvertByteStreamMapToAnyMap(SerializeContext& context, AZ::SerializeContext::DataElementNode& dataPatchElement, PatchMap& anyPatchMap)
    {
        // Find all pair elements within the legacy DataPatch
        AZStd::vector<AZ::SerializeContext::DataElementNode*> pairElements = Utils::FindDescendantElements(context,
            dataPatchElement,
            AZStd::vector<AZ::Crc32>({ AZ_CRC("m_patch", 0xaedc2952), AZ_CRC("element", 0x41405e39) }));

        for (AZ::SerializeContext::DataElementNode* pairElement : pairElements)
        {
            // Pull out the first and second elements from each pair
            AZ::SerializeContext::DataElementNode* first = pairElement->FindSubElement(AZ_CRC("value1", 0xa2756c5a));
            AZ::SerializeContext::DataElementNode* second = pairElement->FindSubElement(AZ_CRC("value2", 0x3b7c3de0));

            if (!first || !second)
            {
                AZStd::string errorMessage = "ConvertByteStreamMapToAnyMap: Unable to find both first and second values from AZStd::pair element during Legacy DataPatch conversion";
                return AZ::Failure(errorMessage);
            }

            // Extract the AddressType data from the first element
            AddressType address;
            if (!first->GetData(address))
            {
                AZStd::string errorMessage = "ConvertByteStreamMapToAnyMap: Unable to retrieve data from AddressType element during Legacy DataPatch conversion";
                return AZ::Failure(errorMessage);
            }

            // Preserve the original DataElement of the bytestream before conversion as well as set its typeId to the conversion type
            // The DataElement contains the byteStream data itself
            // Conversion will overwrite ClassData and clear the underlying DataElement
            AZ::SerializeContext::DataElement byteStreamRawElement = second->GetRawDataElement();
            byteStreamRawElement.m_id = azrtti_typeid<LegacyPatchMap::mapped_type>();

            if (!second->Convert<LegacyPatchMap::mapped_type>(context))
            {
                AZStd::string errorMessage = "ConvertByteStreamMapToAnyMap: Unable to convert LegacyPatchMap type to current PatchMap type during Legacy DataPatch conversion";
                return AZ::Failure(errorMessage);
            }

            // Restore the original DataElement
            second->GetRawDataElement() = byteStreamRawElement;

            // Retrieve the converted ByteStream data
            LegacyPatchMap::mapped_type byteStream;
            if (!second->GetData(byteStream))
            {
                AZStd::string errorMessage = "ConvertByteStreamMapToAnyMap: Unable to retrieve data from ByteStream element during Legacy DataPatch conversion";
                return AZ::Failure(errorMessage);
            }

            if (!byteStream.empty())
            {
                anyPatchMap.emplace(address, DataPatch::LegacyStreamWrapper({ AZStd::move(byteStream) }));
            }
            else
            {
                // Add an empty any to match the bytestream
                // An any wrapped around the empty byteStream is not considered empty
                anyPatchMap.emplace(address, AZStd::any());
            }
        }

        return AZ::Success();
    }

    /**
    * Wrapper around our deprecation converter logic to better handle various error states returned
    */
    bool LegacyDataPatchConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        AZ_PROFILE_FUNCTION(AzCore);
        AZ::Outcome<void, AZStd::string> conversionResult = LegacyDataPatchConverter_Impl(context, classElement);

        if (!conversionResult.IsSuccess())
        {
            AZ_Error("DataPatch::VersionConverter",
                     false,
                     "Failed to convert legacy patch with version %i.\n"
                     "Cause of failure: %s\n"
                     "Failed DataPatch will be defaulted and will be unable to apply expected values.\n"
                     "This is likely caused by a corrupted DataPatch on disk.\n"
                     "It is recommended to debug how that data is interpreted by the converter in source code.",
                     classElement.GetVersion(),
                     conversionResult.GetError().c_str());

            return false;
        }

        return true;
    }

    /**
    * Convert legacy data patch from binary to XML format.
    * Manipulates the provided DataElementNode containing a legacy DataPatch representation so that its PatchMap represents its data in an AZStd::any instead of a bytestream
    * Storing the data in an AZStd::any allows us to organically serialize the patch out in XML
    */
    AZ::Outcome<void, AZStd::string> LegacyDataPatchConverter_Impl(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        AZ_PROFILE_FUNCTION(AzCore);
        // Pull the targetClassId value out of the class element before it gets cleared when converting the DataPatch TypeId
        AZ::TypeId targetClassTypeId;
        if (!classElement.GetChildData(AZ_CRC("m_targetClassId", 0xcabab9dc), targetClassTypeId))
        {
            AZStd::string errorMessage = "Unable to retrieve data from the TypeId field m_targetClassId in the OldDataPatch class";

            DataPatchNotificationBus::Broadcast(&DataPatchNotificationBus::Events::OnLegacyDataPatchLoadFailed);

            return AZ::Failure(errorMessage);
        }

        // Convert the bytestream map into an AZStd::any map
        AZStd::unordered_map<AddressType, AZStd::any> anyPatchMap;
        AZ::Outcome<void, AZStd::string> mapConversionResult = ConvertByteStreamMapToAnyMap(context, classElement, anyPatchMap);
        
        if (!mapConversionResult.IsSuccess())
        {
            DataPatchNotificationBus::Broadcast(&DataPatchNotificationBus::Events::OnLegacyDataPatchLoadFailed);

            // Error is handled at a higher scope
            return mapConversionResult;
        }

        // Convert the DataPatch from its deprecated Id to its new Id
        // Conversion will clear sub elements
        if (!classElement.Convert(context, azrtti_typeid<DataPatch>()))
        {
            AZStd::string errorMessage = AZStd::string::format("Unable to convert from the OldDataPatch class to the new DataPatch class with TypeId: %s",
                azrtti_typeid<DataPatch>().ToString<AZStd::string>().c_str());

            DataPatchNotificationBus::Broadcast(&DataPatchNotificationBus::Events::OnLegacyDataPatchLoadFailed);

            return AZ::Failure(errorMessage);
        }

        // Restore the targetClassId sub element
        if (classElement.AddElementWithData(context, "m_targetClassId", targetClassTypeId) == -1)
        {
            AZStd::string errorMessage = "Unable to add the TypeId field m_targetClassId to converted DataPatch";

            DataPatchNotificationBus::Broadcast(&DataPatchNotificationBus::Events::OnLegacyDataPatchLoadFailed);

            return AZ::Failure(errorMessage);
        }

        if (classElement.AddElementWithData(context, "m_targetClassVersion", (std::numeric_limits<AZ::u32>::max)()) == -1)
        {
            AZStd::string errorMessage = "Unable to add the TypeVersion field m_targetClassVersion to converted DataPatch";

            DataPatchNotificationBus::Broadcast(&DataPatchNotificationBus::Events::OnLegacyDataPatchLoadFailed);

            return AZ::Failure(errorMessage);
        }

        // Restore the converted PatchMap sub element
        if (classElement.AddElementWithData<AZStd::unordered_map<AddressType, AZStd::any>>(context, "m_patch", anyPatchMap) == -1)
        {
            AZStd::string errorMessage = "Unable to add the converted PatchMap field to converted DataPatch";

            DataPatchNotificationBus::Broadcast(&DataPatchNotificationBus::Events::OnLegacyDataPatchLoadFailed);

            return AZ::Failure(errorMessage);
        }

        DataPatchNotificationBus::Broadcast(&DataPatchNotificationBus::Events::OnLegacyDataPatchLoaded);

        return AZ::Success();
    }

    //=========================================================================
    // Apply
    //=========================================================================
    void DataPatch::Reflect(ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<AddressType>()->
                Version(1)->
                Serializer<DataPatchInternal::AddressTypeSerializer>();

            // Reflect old PatchMap format so type can be found during conversion operations
            auto genericInfoOldPatchMap = AZ::SerializeGenericTypeInfo<LegacyPatchMap>::GetGenericInfo();
            if (genericInfoOldPatchMap)
            {
                genericInfoOldPatchMap->Reflect(serializeContext);
            }

            // Reflect helper class to assist with Deprecation Conversion
            serializeContext->Class<LegacyStreamWrapper>()->
                Field("m_stream", &LegacyStreamWrapper::m_stream);

            serializeContext->ClassDeprecate("OldDataPatch", GetLegacyDataPatchTypeId(), &LegacyDataPatchConverter);

            serializeContext->Class<DataPatch>()->
                Field("m_targetClassId", &DataPatch::m_targetClassId)->
                Field("m_targetClassVersion", &DataPatch::m_targetClassVersion)->
                Field("m_patch", &DataPatch::m_patch);
        }
    }

    void ReportDataPatchMismatch(SerializeContext* context, const SerializeContext::ClassElement* classElement, const TypeId& patchDataTypeId)
    {
        AZStd::string patchTypeIdName;
        if (auto classData = context->FindClassData(patchDataTypeId))
        {
            patchTypeIdName = classData->m_name;
        }

        AZStd::string sourceTypeIdName;
        if (auto classData = context->FindClassData(classElement->m_typeId))
        {
            sourceTypeIdName = classData->m_name;
        }

        AZ_Warning("DataNodeTree::ApplyToElements", false, "Patch element with Type: %s (Type Id: %s) does not match the source element with Name: '%s' of Type: %s (Type Id: %s)",
            patchTypeIdName.c_str(),
            patchDataTypeId.ToString<AZStd::string>().c_str(),
            classElement->m_name,
            sourceTypeIdName.c_str(),
            classElement->m_typeId.ToString<AZStd::string>().c_str());
    }
}   // namespace AZ
