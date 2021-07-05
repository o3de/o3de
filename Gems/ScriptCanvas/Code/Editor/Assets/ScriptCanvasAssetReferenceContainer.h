/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <Editor/Assets/ScriptCanvasAssetReference.h>

namespace ScriptCanvasEditor
{
    class ScriptCanvasAssetReferenceContainer
        : public AZ::SerializeContext::IDataContainer
    {
    public:
        ScriptCanvasAssetReferenceContainer()
        {
            m_storeInObjectStreamElement.m_name = "m_storeInObjectStream";
            m_storeInObjectStreamElement.m_nameCrc = AZ_CRC("m_storeInObjectStream", 0xf5a45371);
            m_storeInObjectStreamElement.m_typeId = azrtti_typeid<bool>();
            m_storeInObjectStreamElement.m_offset = 0;
            m_storeInObjectStreamElement.m_dataSize = sizeof(bool);
            m_storeInObjectStreamElement.m_azRtti = nullptr;
            m_storeInObjectStreamElement.m_genericClassInfo = AZ::SerializeGenericTypeInfo<bool>::GetGenericInfo();
            m_storeInObjectStreamElement.m_editData = nullptr;
            m_storeInObjectStreamElement.m_flags = 0;

            m_assetElement.m_name = "m_asset";
            m_assetElement.m_nameCrc = AZ_CRC("m_asset", 0x4e58e538);
            m_assetElement.m_typeId = AZ::SerializeGenericTypeInfo<AZ::Data::Asset<ScriptCanvasAsset>>::GetClassTypeId();
            m_assetElement.m_offset = m_storeInObjectStreamElement.m_dataSize;
            m_assetElement.m_dataSize = sizeof(AZ::Data::Asset<ScriptCanvasAsset>);
            m_assetElement.m_azRtti = AZ::GetRttiHelper<AZ::Data::Asset<ScriptCanvasAsset>>();
            m_assetElement.m_genericClassInfo = AZ::SerializeGenericTypeInfo<AZ::Data::Asset<ScriptCanvasAsset>>::GetGenericInfo();
            m_assetElement.m_editData = nullptr;
            m_assetElement.m_flags = 0;
            
            m_baseDataElement.m_name = "m_scriptCanvasData";
            m_baseDataElement.m_nameCrc = AZ_CRC("m_scriptCanvasData", 0x78a93f93);
            m_baseDataElement.m_typeId = AZ::SerializeGenericTypeInfo<ScriptCanvas::ScriptCanvasData>::GetClassTypeId();
            m_baseDataElement.m_offset = m_assetElement.m_offset + m_assetElement.m_dataSize;
            m_baseDataElement.m_dataSize = sizeof(ScriptCanvas::ScriptCanvasData);
            m_baseDataElement.m_azRtti = AZ::GetRttiHelper<ScriptCanvas::ScriptCanvasData>();
            m_baseDataElement.m_genericClassInfo = AZ::SerializeGenericTypeInfo<ScriptCanvas::ScriptCanvasData>::GetGenericInfo();
            m_baseDataElement.m_editData = nullptr;
            m_baseDataElement.m_flags = 0;
        }
        /// Null if element with this name can't be found.
        const AZ::SerializeContext::ClassElement* GetElement(AZ::u32 elementNameCrc) const override
        {
            if (m_storeInObjectStreamElement.m_nameCrc == elementNameCrc)
            {
                return &m_storeInObjectStreamElement;
            }
            if (m_assetElement.m_nameCrc == elementNameCrc)
            {
                return &m_assetElement;
            }
            if (m_baseDataElement.m_nameCrc == elementNameCrc)
            {
                return &m_baseDataElement;
            }

            return nullptr;
        }

        bool GetElement(AZ::SerializeContext::ClassElement& classElement, const AZ::SerializeContext::DataElement& dataElement) const override
        {
            if (m_storeInObjectStreamElement.m_nameCrc == dataElement.m_nameCrc)
            {
                classElement = m_storeInObjectStreamElement;
                return true;
            }
            if (m_assetElement.m_nameCrc == dataElement.m_nameCrc)
            {
                classElement = m_assetElement;
                return true;
            }
            if (m_baseDataElement.m_nameCrc == dataElement.m_nameCrc)
            {
                classElement = m_baseDataElement;
                return true;
            }
            return false;
        }

        /// Enumerate elements in the array.
        void EnumElements(void* instance, const ElementCB& cb) override
        {
            auto assetRef = reinterpret_cast<ScriptCanvasAssetReference*>(instance);

            if (!cb(&assetRef->m_storeInObjectStream, m_storeInObjectStreamElement.m_typeId, m_storeInObjectStreamElement.m_genericClassInfo ? m_storeInObjectStreamElement.m_genericClassInfo->GetClassData() : nullptr, &m_storeInObjectStreamElement))
            {
                return;
            }

            if (!cb(&assetRef->m_asset, m_assetElement.m_typeId, m_assetElement.m_genericClassInfo ? m_assetElement.m_genericClassInfo->GetClassData() : nullptr, &m_assetElement))
            {
                return;
            }

            if (assetRef->m_storeInObjectStream && assetRef->m_asset.Get())
            {
                cb(&assetRef->m_asset.Get()->GetScriptCanvasData(), m_baseDataElement.m_typeId, m_baseDataElement.m_genericClassInfo ? m_baseDataElement.m_genericClassInfo->GetClassData() : nullptr, &m_baseDataElement);
            }
        }

        void EnumTypes(const ElementTypeCB& cb) override
        {
            cb(m_storeInObjectStreamElement.m_typeId, &m_storeInObjectStreamElement);
            cb(m_assetElement.m_typeId, &m_assetElement);
            cb(m_baseDataElement.m_typeId, &m_baseDataElement);
        }

        /// Return number of elements in the container.
        size_t  Size(void* instance) const override
        {
            auto assetRef = reinterpret_cast<ScriptCanvasAssetReference*>(instance);
            return assetRef->m_storeInObjectStream && assetRef->m_asset.Get() ? m_numClassElements : m_numClassElements - 1;
        }

        /// Returns the capacity of the container. Returns 0 for objects without fixed capacity.
        size_t Capacity(void*) const override
        {
            return m_numClassElements;
        }

        /// Returns true if elements pointers don't change on add/remove. If false you MUST enumerate all elements.
        bool    IsStableElements() const override { return true; }

        /// Returns true if the container is fixed size, otherwise false.
        bool    IsFixedSize() const override { return false; }

        /// Returns if the container is fixed capacity, otherwise false
        bool    IsFixedCapacity() const override { return true; }

        /// Returns true if the container is a smart pointer.
        bool    IsSmartPointer() const override { return false; }

        /// Returns true if elements can be retrieved by index.
        bool    CanAccessElementsByIndex() const override { return false; }

        /// Reserve element
        void* ReserveElement(void* instance, const AZ::SerializeContext::ClassElement* classElement) override
        {
            auto assetRef = reinterpret_cast<ScriptCanvasAssetReference*>(instance);
            if (classElement->m_nameCrc == m_storeInObjectStreamElement.m_nameCrc)
            {
                return &assetRef->m_storeInObjectStream;
            }
            if (classElement->m_nameCrc == m_assetElement.m_nameCrc)
            {
                if (assetRef->m_storeInObjectStream)
                {
                    assetRef->m_asset.SetFlags(static_cast<AZ::u8>(AZ::Data::AssetLoadBehavior::NoLoad));
                }
                return &assetRef->m_asset;
            }
            if (assetRef->m_storeInObjectStream && classElement->m_nameCrc == m_baseDataElement.m_nameCrc)
            {
                auto* scriptCanvasAsset = aznew ScriptCanvasAsset(assetRef->m_asset.GetId(), AZ::Data::AssetData::AssetStatus::Ready);
                assetRef->m_asset = { scriptCanvasAsset, AZ::Data::AssetLoadBehavior::Default };
                return &assetRef->m_asset.Get()->GetScriptCanvasData();
            }

            return instance;
        }

        /// Get an element's address by its index (called before the element is loaded).
        void* GetElementByIndex(void*, const AZ::SerializeContext::ClassElement*, size_t) override
        {
            return nullptr;
        }

        /// Store element
        void StoreElement(void* instance, void* element) override
        {
            auto assetRef = reinterpret_cast<ScriptCanvasAssetReference*>(instance);
            if (assetRef->m_storeInObjectStream && assetRef->m_asset.Get() && element == &assetRef->m_asset.Get()->GetScriptCanvasData())
            {
                auto existingAsset = AZ::Data::AssetManager::Instance().FindAsset<ScriptCanvasAsset>(assetRef->m_asset.GetId(), assetRef->m_asset.GetAutoLoadBehavior());
                if (existingAsset.IsReady())
                {
                    assetRef->m_asset = existingAsset;
                }
            }
        }

        /// Remove element in the container.
        bool RemoveElement(void* instance, const void* element, AZ::SerializeContext* deletePointerDataContext) override
        {
            if (deletePointerDataContext)
            {
                auto assetRef = reinterpret_cast<ScriptCanvasAssetReference*>(instance);
                if (&assetRef->m_storeInObjectStream == element)
                {
                    DeletePointerData(deletePointerDataContext, &m_storeInObjectStreamElement, &assetRef->m_storeInObjectStream);
                }
                else if (&assetRef->m_asset == element)
                {
                    DeletePointerData(deletePointerDataContext, &m_assetElement, &assetRef->m_asset);
                }
            }
            return false;
        }

        /// Remove elements (removed array of elements) regardless if the container is Stable or not (IsStableElements)
        size_t RemoveElements(void* instance, const void** elements, size_t numElements, AZ::SerializeContext* deletePointerDataContext) override
        {
            if (deletePointerDataContext)
            {
                for (size_t i = 0; i < numElements; ++i)
                {
                    RemoveElement(instance, elements[i], deletePointerDataContext);
                }
            }
            return 0;
        }

        /// Clear elements in the instance.
        void ClearElements(void*, AZ::SerializeContext*) override
        {
        }

        AZ::SerializeContext::ClassElement m_storeInObjectStreamElement;
        AZ::SerializeContext::ClassElement m_assetElement;
        AZ::SerializeContext::ClassElement m_baseDataElement;
        const size_t m_numClassElements = 3;
    };
}
