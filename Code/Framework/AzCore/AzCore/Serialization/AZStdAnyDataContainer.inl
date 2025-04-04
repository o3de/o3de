/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/any.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
    namespace Internal
    {
        struct AZStdAnyContainer
            : public SerializeContext::IDataContainer
        {
        public:
            AZStdAnyContainer() = default;

            const char* GetElementName([[maybe_unused]] int index = 0) override { return "m_data"; }

            u32 GetElementNameCrC([[maybe_unused]] int index = 0) override { return AZ_CRC_CE("m_data"); }

            /// Null if element with this name can't be found.
            const SerializeContext::ClassElement* GetElement(u32) const override
            {
                return nullptr;
            }

            bool GetElement(SerializeContext::ClassElement& classElement, const SerializeContext::DataElement& dataElement) const override
            {
                if (static_cast<AZ::u32>(AZ_CRC_CE("m_data")) == dataElement.m_nameCrc)
                {
                    classElement.m_name = "m_data";
                    classElement.m_nameCrc = AZ_CRC_CE("m_data");
                    classElement.m_typeId = dataElement.m_id;
                    classElement.m_dataSize = sizeof(void*);
                    classElement.m_offset = 0;
                    classElement.m_azRtti = nullptr;
                    classElement.m_genericClassInfo = m_serializeContext->FindGenericClassInfo(dataElement.m_id);
                    classElement.m_editData = nullptr;
                    classElement.m_flags = SerializeContext::ClassElement::FLG_DYNAMIC_FIELD;
                    return true;
                }
                return false;
            }

            /// Enumerate elements in the array.
            void EnumElements(void* instance, const ElementCB& cb) override
            {
                auto anyPtr = reinterpret_cast<AZStd::any*>(instance);

                AZ::Uuid anyTypeId = anyPtr->type();
                // Empty AZStd::any will not have the empty element serialized
                if (anyPtr->empty() || !AZStd::any_cast<void>(anyPtr))
                {
                    return;
                }

                SerializeContext::ClassElement anyChildElement;
                anyChildElement.m_name = "m_data";
                anyChildElement.m_nameCrc = AZ_CRC_CE("m_data");
                anyChildElement.m_typeId = anyTypeId;
                anyChildElement.m_dataSize = sizeof(void*);
                anyChildElement.m_offset = 0;
                anyChildElement.m_azRtti = nullptr;
                anyChildElement.m_genericClassInfo = m_serializeContext->FindGenericClassInfo(anyTypeId);
                anyChildElement.m_editData = nullptr;
                anyChildElement.m_flags = SerializeContext::ClassElement::FLG_DYNAMIC_FIELD | (anyPtr->get_type_info().m_isPointer ? SerializeContext::ClassElement::FLG_POINTER : 0);

                if (!cb(AZStd::any_cast<void>(anyPtr), anyTypeId, anyChildElement.m_genericClassInfo ? anyChildElement.m_genericClassInfo->GetClassData() : nullptr, &anyChildElement))
                {
                    return;
                }
            }

            void EnumTypes(const ElementTypeCB& /*cb*/) override
            {
            }

            /// Return number of elements in the container.
            size_t  Size(void* instance) const override
            {
                (void)instance;
                return 1;
            }

            /// Returns the capacity of the container. Returns 0 for objects without fixed capacity.
            size_t Capacity(void* instance) const override
            {
                (void)instance;
                return 1;
            }

            /// Returns true if elements pointers don't change on add/remove. If false you MUST enumerate all elements.
            bool    IsStableElements() const override { return true; }

            /// Returns true if the container is fixed size, otherwise false.
            bool    IsFixedSize() const override { return true; }

            /// Returns if the container is fixed capacity, otherwise false
            bool    IsFixedCapacity() const override { return true; }

            /// Returns true if the container is a smart pointer.
            bool    IsSmartPointer() const override { return false; }

            /// Returns true if elements can be retrieved by index.
            bool    CanAccessElementsByIndex() const override { return false; }

            /// Reserve element
            void* ReserveElement(void* instance, const SerializeContext::ClassElement* classElement) override
            {
                if (m_serializeContext && classElement)
                {
                    auto anyPtr = reinterpret_cast<AZStd::any*>(instance);
                    *anyPtr = m_serializeContext->CreateAny(classElement->m_genericClassInfo ? classElement->m_genericClassInfo->GetSpecializedTypeId() : classElement->m_typeId);
                    return AZStd::any_cast<void>(anyPtr);
                }
                
                return instance;
            }

            /// Get an element's address by its index (called before the element is loaded).
            void* GetElementByIndex(void* instance, const SerializeContext::ClassElement* classElement, size_t index) override
            {
                (void)instance;
                (void)classElement;
                (void)index;
                return nullptr;
            }

            /// Store element
            void StoreElement(void* instance, void* element) override
            {
                (void)instance;
                (void)element;
                // do nothing as we have already pushed the element,
                // we can assert and check if the element belongs to the container
            }

            /// Remove element in the container.
            bool RemoveElement(void* instance, const void*, SerializeContext*) override
            {
                auto anyPtr = reinterpret_cast<AZStd::any*>(instance);
                if (anyPtr->empty())
                {
                    return false;
                }

                anyPtr->clear();
                return true;
            }

            /// Remove elements (removed array of elements) regardless if the container is Stable or not (IsStableElements)
            size_t RemoveElements(void* instance, const void** elements, size_t numElements, SerializeContext* deletePointerDataContext) override
            {
                return numElements == 1 && RemoveElement(instance, elements[0], deletePointerDataContext) ? 1 : 0;
            }

            /// Clear elements in the instance.
            void ClearElements(void* instance, SerializeContext*) override
            {
                auto anyPtr = reinterpret_cast<AZStd::any*>(instance);
                anyPtr->clear();
            }

            void SetSerializeContext(SerializeContext* serializeContext)
            {
                m_serializeContext = serializeContext;
            }

            SerializeContext* m_serializeContext = nullptr;
        };

        inline void ReflectAny(ReflectContext* reflectContext)
        {       
            if (auto serializeContext = azrtti_cast<SerializeContext*>(reflectContext))
            {
                auto dataContainer = AZStd::make_unique<AZStdAnyContainer>();
                dataContainer->SetSerializeContext(serializeContext);
                serializeContext->Class<AZStd::any>()
                    ->DataContainer(dataContainer.get())
                    ;
                // serializeContext owns each dataContainer instance, it is destroyed once serializeContext is destroyed
                serializeContext->RegisterDataContainer(AZStd::move(dataContainer));
                // Value data is injected into the hierarchy per-instance, since type is dynamic.
                if (EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<AZStd::any>("any", "Type safe container which can store a type that specializes the TypeInfo template")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
                }
            }
        }
    }


    namespace Helpers
    {
        //! Used to compare the data inside an AZStd::any when the types are not known at compile time.
        //! This method may rely on serialization to determine if the values are equal, avoid using it in 
        //! performance sensitive code.
        AZ_INLINE bool CompareAnyValue(const AZStd::any& lhs, const AZStd::any& rhs)
        {
            if (lhs.type() == rhs.type())
            {
                AZ::SerializeContext* serializeContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

                const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(lhs.type());
                if (classData)
                {
                    if (classData->m_serializer)
                    {
                        return classData->m_serializer->CompareValueData(AZStd::any_cast<void>(&lhs), AZStd::any_cast<void>(&rhs));
                    }
                    else
                    {
                        AZStd::vector<AZ::u8> myData;
                        AZ::IO::ByteContainerStream<decltype(myData)> myDataStream(&myData);

                        AZ::Utils::SaveObjectToStream(myDataStream, AZ::ObjectStream::ST_BINARY, AZStd::any_cast<void>(&lhs), lhs.type());

                        AZStd::vector<AZ::u8> otherData;
                        AZ::IO::ByteContainerStream<decltype(otherData)> otherDataStream(&otherData);

                        AZ::Utils::SaveObjectToStream(otherDataStream, AZ::ObjectStream::ST_BINARY, AZStd::any_cast<void>(&rhs), rhs.type());
                        return (myData.size() == otherData.size()) && (memcmp(myData.data(), otherData.data(), myData.size()) == 0);
                    }
                }
            }

            return false;
        }
    }
}

