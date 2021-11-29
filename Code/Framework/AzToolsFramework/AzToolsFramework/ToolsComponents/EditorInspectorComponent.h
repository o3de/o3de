/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "EditorComponentBase.h"
#include "EditorInspectorComponentBus.h"
#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{
    namespace Components
    {
        /**
         * Contains Inspector related data that needs to be stored on a per-entity level, such as component ordering per-entity
         */
        class EditorInspectorComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , public EditorInspectorComponentRequestBus::Handler
        {
        public:
            AZ_COMPONENT(EditorInspectorComponent, "{47DE3DDA-50C5-4F50-B1DB-BA4AE66AB056}", EditorComponentBase);
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static bool SerializationConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
            
            ~EditorInspectorComponent();

            ////////////////////////////////////////////////////////////////////
            // EditorInspectorComponentRequestBus
            ComponentOrderArray GetComponentOrderArray() override;
            void SetComponentOrderArray(const ComponentOrderArray& componentOrderArray) override;

        private:
            ////////////////////////////////////////////////////////////////////
            // AZ::Entity
            void Init() override;
            ////////////////////////////////////////////////////////////////////

            /** 
             * Ensures m_componentOrderEntryArray is up to date before serializing out
             */
            void PrepareSave();

            /**
             * Ensures m_componentOrderArray is up to date after serializing in
             */
            void PostLoad();

            /**
             * ComponentOrderSerializationEvents intercepts the serialization events to ensure the data is complete before and after serialization
             */
            class ComponentOrderSerializationEvents
                : public AZ::SerializeContext::IEventHandler
            {
                /** 
                 * Called right before we start reading from the instance pointed by classPtr.
                 */
                void OnReadBegin(void* classPtr) override
                {
                    EditorInspectorComponent* component = reinterpret_cast<EditorInspectorComponent*>(classPtr);
                    component->PrepareSave();
                }

                /**
                 * Called right after we finish writing data to the instance pointed at by classPtr.
                 */
                void OnWriteEnd(void* classPtr) override
                {
                    EditorInspectorComponent* component = reinterpret_cast<EditorInspectorComponent*>(classPtr);
                    component->PostLoad();
                }
            };

            /**
             * ComponentOrderEntry stores the component id and the sort index (which is the absolute sort index relative to the other entries, 0 is the first, 1 is the second, so on)
             * We serialize out the order data in this fashion because the slice data patching system will traditionally use the vector index to know what data goes where
             * In the case of this data, it does not make sense to data patch by vector index since the underlying data may have changed and the data patch will create duplicate or incorrect data.
             * The slice data patch system has the concept of a "Persistent ID" which can be used instead such that data patches will try to match persistent ids which can be identified regardless
             * of vector index. In this way, our vector order no longer matters and the Component Id is now the identifier which the data patcher will use to update the sort index.
             */
            struct ComponentOrderEntry
            {
                AZ_TYPE_INFO(ComponentOrderEntry, "{335C5861-5197-4DD5-A766-EF2B551B0D9D}");

                AZ::ComponentId m_componentId;
                AZ::u64 m_sortIndex;
            };
            using ComponentOrderEntryArray = AZStd::vector<ComponentOrderEntry>;
            ComponentOrderEntryArray m_componentOrderEntryArray; ///< The serialized order array which uses the persistent id mechanism as described above*/

            ComponentOrderArray m_componentOrderArray; ///< The simple vector of component id is what is used by the component order ebus and is generated from the serialized data
            
            bool m_componentOrderIsDirty = true; ///< This flag indicates our stored serialization order data is out of date and must be rebuilt before serialization occurs
        };
    } // namespace Components
} // namespace AzToolsFramework
