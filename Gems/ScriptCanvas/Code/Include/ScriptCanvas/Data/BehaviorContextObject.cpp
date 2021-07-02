/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BehaviorContextObject.h"
#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Data/DataRegistry.h>

#include <AzCore/Component/EntityBus.h>
#include <AzCore/Serialization/Utils.h>

#include <ScriptCanvas/Core/GraphScopedTypes.h>

namespace ScriptCanvas
{
    void BehaviorContextObject::OnReadBegin()
    {
        if (!IsOwned())
        {
            Clear();
        }
    }

    void BehaviorContextObject::OnWriteEnd()
    {
        // Id Remapping invokes this method as well, not just serializing from an ObjectStream
    }

    void BehaviorContextObject::Reflect(AZ::ReflectContext* reflection)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->Class<BehaviorContextObject>()
                ->Version(0)
                ->EventHandler<SerializeContextEventHandler>()
                ->Field("m_flags", &BehaviorContextObject::m_flags)
                ->Field("m_object", &BehaviorContextObject::m_object)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<BehaviorContextObject>("", "BehaviorContextObject")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "BehaviorContextObject")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BehaviorContextObject::m_object, "Datum", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                    ;
            }
        }
    }

    void BehaviorContextObject::SerializeContextEventHandler::OnReadBegin(void* classPtr)
    {
        BehaviorContextObject* object = reinterpret_cast<BehaviorContextObject*>(classPtr);
        object->OnReadBegin();
    }

    void BehaviorContextObject::SerializeContextEventHandler::OnWriteEnd(void* classPtr)
    {
        BehaviorContextObject* object = reinterpret_cast<BehaviorContextObject*>(classPtr);
        object->OnWriteEnd();
    }

    BehaviorContextObjectPtr BehaviorContextObject::CloneObject(const AZ::BehaviorClass& behaviorClass)
    {
        if (SystemRequestBus::HasHandlers())
        {
            AZStd::vector<char> buffer;

            {
                bool wasOwned = IsOwned();
                m_flags |= Flags::Owned;

                AZ::IO::ByteContainerStream<AZStd::vector<char>> writeStream(&buffer);
                AZ::Utils::SaveObjectToStream<BehaviorContextObject>(writeStream, AZ::DataStream::ST_BINARY, this);

                if (!wasOwned)
                {
                    m_flags &= ~Flags::Owned;
                }
            }

            AZ::IO::ByteContainerStream<AZStd::vector<char>> readStream(&buffer);

            BehaviorContextObject* newObject = CreateDefault(behaviorClass);
            AZ::Utils::LoadObjectFromStreamInPlace(readStream, (*newObject));

            if (newObject != nullptr)
            {
                SystemRequestBus::Broadcast(&SystemRequests::AddOwnedObjectReference, newObject->Get(), newObject);
                return BehaviorContextObjectPtr(newObject);
            }
        }

        return nullptr;
    }

    BehaviorContextObjectPtr BehaviorContextObject::Create(const AZ::BehaviorClass& behaviorClass, const void* value)
    {
        if (SystemRequestBus::HasHandlers())
        {
            BehaviorContextObject* ownedObject = value ? CreateCopy(behaviorClass, value) : CreateDefault(behaviorClass);
            SystemRequestBus::Broadcast(&SystemRequests::AddOwnedObjectReference, ownedObject->Get(), ownedObject);
            return BehaviorContextObjectPtr(ownedObject);
        }

        AZ_Assert(false, "The Script Canvas SystemRequest Bus needs to be handled by at least one class!");
        return nullptr;
    }

    BehaviorContextObjectPtr BehaviorContextObject::CreateReference(const AZ::Uuid& typeID, void* reference)
    {
        const AZ::u32 referenceFlags(0);
        BehaviorContextObject* ownedObject{};
        SystemRequestBus::BroadcastResult(ownedObject, &SystemRequests::FindOwnedObjectReference, reference);
        return ownedObject
            ? BehaviorContextObjectPtr(ownedObject)
            : BehaviorContextObjectPtr(aznew BehaviorContextObject(reference, GetAnyTypeInfoReference(typeID), referenceFlags));
    }

    void BehaviorContextObject::release()
    {
        if (--m_referenceCount == 0)
        {
            SystemRequestBus::Broadcast(&SystemRequests::RemoveOwnedObjectReference, Get());
            delete this;
        }
    }

} 
