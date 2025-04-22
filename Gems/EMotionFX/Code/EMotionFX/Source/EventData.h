/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "EMotionFXConfig.h"
#include "EMotionFXManager.h"
#include "EventManager.h"
#include <MCore/Source/ReflectionSerializer.h>

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace EMotionFX
{
    /** @brief A description of a set of parameters and values that is sent
     * when an event is dispatched.
     *
     * This is the base class of all event data types. General-purpose
     * parameters should subclass this type.  For parameter types that are
     * designed to be placed on the Sync track, use EventDataSyncable instead.
     *
     * @section subclassing Subclassing Guidelines
     *
     * Subclasses of this type should implement Reflect() to reflect the type
     * to the Serialization and Edit contexts, and should implement Equal().
     * For the type to be visible in the EMotionFX Editor, the "Creatable"
     * attribute should be added to the class's ClassElement in the
     * EditContext. Without this attribute, the type will not show up in the
     * QComboBox that allows users to select the EventData type in the
     * EMotionFX's Motion Events tab. Here's an example:
     *
     * @code
     * editContext->Class<MyEventDataType>("MyEventDataType", "")
     *     ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
     *         ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
     *         ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
     *         ->Attribute(AZ_CRC_CE("Creatable"), true)
     * @endcode
     *
     * @sa MotionEvent EventDataSyncable
     *
     */
    class EMFX_API EventData
    {
    public:
        AZ_RTTI(EventData, "{F6AFCD3B-D58E-4821-9E7C-D1F437304E5D}");

        virtual ~EventData() = default;

        virtual bool operator==(const EventData& rhs) const;

        static void Reflect(AZ::ReflectContext* context);

        /** @brief Test if two EventData instances are equal
         *
         * This method is used by EMotionFX to deduplicate instances of
         * EventData subclasses, and by the AnimGraphMotionCondition's Motion
         * Event matching logic.
         *
         * When loading a .motion file and deserializing the MotionEvents on
         * the event tracks, each EventData instance is run through
         * EventManager::FindOrCreateEventData. The EventManager stores a list
         * of all EventData instances in use, and attempts to find one where
         * Equal(loadedEventData) returns true. If it finds one that is equal,
         * the duplicate data is discarded. So this method is essential in
         * saving memory of the duplicated EventData instances.
         *
         * When an AnimGraphMotionCondition is used to test against a Motion
         * Event, this method is called by
         * AnimGraphMotionCondition::TestCondition. In this case, @p
         * ignoreEmptyFields is true. This allows the condition to match
         * against parts of the EventData and not others. For example, if one
         * of the fields is a string, and that string value is empty in the
         * condition, it can act as a wildcard match for that field.
         *
         * This method is not used by the AnimGraphSyncTrack to determine if
         * events are the same for syncing (that method is
         * EventDataSyncable::HashForSyncing).
         *
         */
        virtual bool Equal(const EventData& rhs, bool ignoreEmptyFields = false) const = 0;

        virtual AZStd::string ToString() const;
    };

    /*
    * This is used in the specialization of AZ::SerializeGenericTypeInfo below.

    * The goal is to only store any specific configuration of an EventData
    * subclass once. To accomplish this, the EMotionFX::EventManager contains a
    * vector of weak_ptrs to EventDatas. To create a MotionEvent with
    * parameters, you ask the EventManager if it already has an instance of the
    * EventData for your parameters. If it does, you get back a pointer to a
    * const EventData* (really, it is a AZStd::shared_ptr<const EventData>).
    * The fact that the EventData is const is important: that prevents edits to
    * one EventData from affecting the other events that are using the same
    * instance.

    * There's a few issues with the Serialization framework when combined with
    * this design. First, the serialization framework doesn't support
    * AZStd::shared_ptr<const T>. Attempting to serialize that type will fail.
    * The second is that serializing a vector of shared_ptrs that all point to
    * the same thing (say, for instance,
    * AZStd::vector<AZStd::shared_ptr<EventData>> {myPtr, myPtr, myPtr}), and
    * then deserializing that vector, results in 3 instances of EventData,
    * whereas the original vector had only 1. This happens because the
    * shared_ptr is treated like a container, and the contents of that
    * container are written out to the object stream for every instance of that
    * container. They are duplicated when serialized and not deduplicated when
    * deserializing.

    * The specialization of AZ::SerializeGenericTypeInfo specializes for the
    * AZStd::shared_ptr<const EMotionFX::EventData> type (note the const
    * qualifier), to allow the serialization of the const data. The
    * EventDataSharedPtrContainer overrides the StoreElement method in order to
    * register/deduplicate the loaded data.
    */

    template<class T>
    class EventDataSharedPtrContainer
        : public AZ::Internal::AZStdSmartPtrContainer<T>
    {
        using ValueType = typename AZ::Internal::AZStdSmartPtrContainer<T>::ValueType;

        /// Store element
        virtual void StoreElement(void* instance, void* element) override
        {
            AZ::Internal::AZStdSmartPtrContainer<T>::StoreElement(instance, element);

            // Deduplicate the event data when loading from a serialized string
            // using the EventManager
            T* smartPtr = reinterpret_cast<T*>(instance);
            *smartPtr = AZStd::const_pointer_cast<typename T::value_type>(
                EMotionFX::GetEventManager().FindEventData<typename T::value_type>(*smartPtr)
            );
        }
    };
} // end namespace EMotionFX

namespace AZ
{
    // This specialization exists to make it possible to serialize a shared_ptr
    // to a const EventData, and to deduplicate the event data when loading a
    // serialized object
    template<>
    struct SerializeGenericTypeInfo< AZStd::shared_ptr<const EMotionFX::EventData> >
    {
        // Note that the container type is a pointer-to-non-const
        typedef typename AZStd::shared_ptr<EMotionFX::EventData>   ContainerType;

        class GenericClassSharedPtr
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassSharedPtr, "{D5B5ACA6-A81E-410E-8151-80C97B8CD2A0}");
            GenericClassSharedPtr()
            {
                m_classData = SerializeContext::ClassData::Create<ContainerType>("AZStd::shared_ptr", GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage);
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            AZ::TypeId GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<const EMotionFX::EventData>::GetClassTypeId();
            }

            // AZStdSmartPtrContainer uses the underlying smart_ptr container value_type typedef type id for serialization
            AZ::TypeId GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            AZ::TypeId GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext) override
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            bool CanStoreType(const Uuid& typeId) const override
            {
                return GetSpecializedTypeId() == typeId || GetGenericTypeId() == typeId || m_containerStorage.m_classElement.m_typeId == typeId;
            }

            static GenericClassSharedPtr* Instance()
            {
                static GenericClassSharedPtr s_instance;
                return &s_instance;
            }

            EMotionFX::EventDataSharedPtrContainer<ContainerType> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        static GenericClassInfo* GetGenericInfo()
        {
            return GenericClassSharedPtr::Instance();
        }

        static AZ::TypeId GetClassTypeId()
        {
            return GenericClassSharedPtr::Instance()->m_classData.m_typeId;
        }
    };
}
