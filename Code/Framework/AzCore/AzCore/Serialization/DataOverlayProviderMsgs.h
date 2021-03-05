/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef AZCORE_DATA_OVERLAY_PROVIDER_MSGS_H
#define AZCORE_DATA_OVERLAY_PROVIDER_MSGS_H

#include <AzCore/Serialization/DataOverlay.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/EBus/EBus.h>

namespace AZ
{
    class DataOverlayTarget;

    /**
     * DataOverlayProviderBus is used to communicate with overlay providers
     */
    class DataOverlayProviderMsgs
        : public EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        typedef DataOverlayProviderId BusIdType;
        static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        //////////////////////////////////////////////////////////////////////////

        virtual ~DataOverlayProviderMsgs() {}

        virtual void FillOverlayData(DataOverlayTarget* dest, const DataOverlayToken& dataToken) = 0;
    };
    typedef EBus<DataOverlayProviderMsgs> DataOverlayProviderBus;

    class DataOverlayTarget
    {
    public:
        DataOverlayTarget(SerializeContext::DataElementNode* dataContainer, SerializeContext* sc, SerializeContext::ErrorHandler* errorLogger)
            : m_dataContainer(dataContainer)
            , m_sc(sc)
            , m_errorLogger(errorLogger)
        {
        }

        template<typename T>
        void   SetData(const T& obj);

    protected:
        typedef AZStd::vector<SerializeContext::DataElementNode*> NodeStack;

        void Parse(const void* classPtr, const SerializeContext::ClassData* classData);
        bool ElementBegin(NodeStack* nodeStack, const void* elemPtr, const SerializeContext::ClassData* classData, const SerializeContext::ClassElement* elementData);
        bool ElementEnd(NodeStack* nodeStack);

        SerializeContext::DataElementNode*      m_dataContainer;
        SerializeContext*                       m_sc;
        SerializeContext::ErrorHandler*         m_errorLogger;
    };

    template<typename T>
    void DataOverlayTarget::SetData(const T& obj)
    {
        AZ_Assert(!AZStd::is_pointer<T>::value, "Cannot serialize pointer-to-pointer as root element! This makes no sense!");
        const void* classPtr = SerializeTypeInfo<T>::RttiCast(&obj, SerializeTypeInfo<T>::GetRttiTypeId(&obj));
        const Uuid& classId = SerializeTypeInfo<T>::GetUuid(&obj);
        const SerializeContext::ClassData* classData = m_sc->FindClassData(classId, nullptr, 0);
        if (!classData)
        {
            GenericClassInfo* genericClassInfo = SerializeGenericTypeInfo<T>::GetGenericInfo();
            classData = genericClassInfo ? genericClassInfo->GetClassData() : nullptr;
            if (!classData)
            {
                AZ_Error("Serializer", false, "Class '%s' is not registered with the serializer!", SerializeTypeInfo<T>::GetRttiTypeName(&obj));
                return;
            }
        }
        Parse(classPtr, classData);
    }
}   // namespace AZ

#endif  // AZCORE_DATA_OVERLAY_PROVIDER_MSGS_H
#pragma once
