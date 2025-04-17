/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Configuration.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    namespace RPI
    {
        class FeatureProcessor;
        using FeatureProcessorPtr = AZStd::unique_ptr<FeatureProcessor>;

        //! The feature processor factory is where gems should register their
        //! feature processors. Once registered, these feature processors can
        //! be queried and created for scenes attempting to enable/disable a
        //! specific feature processor.
        class ATOM_RPI_PUBLIC_API FeatureProcessorFactory final
        {
        public:
            friend class FeatureProcessorDeleter;
            friend class Scene;

            AZ_RTTI(FeatureProcessorFactory, "{3F16394E-D801-4FAC-B329-40B8D7724AEE}");
            AZ_CLASS_ALLOCATOR(FeatureProcessorFactory, AZ::SystemAllocator);

            static FeatureProcessorFactory* Get();

            FeatureProcessorFactory();
            virtual ~FeatureProcessorFactory() = default;

            // Note that you have to delete these for safety reasons, you will trip a static_assert if you do not
            AZ_DISABLE_COPY_MOVE(FeatureProcessorFactory);

            template<typename FeatureProcessorType>
            void RegisterFeatureProcessor();

            template<typename FeatureProcessorType, typename InterfaceType>
            void RegisterFeatureProcessorWithInterface();

            template<typename FeatureProcessorType>
            void UnregisterFeatureProcessor();

            void Init();
            void Shutdown();

            FeatureProcessorPtr CreateFeatureProcessor(FeatureProcessorId featureProcessorId);
            AZ::TypeId GetFeatureProcessorTypeId(FeatureProcessorId featureProcessorId);
            AZ::TypeId GetFeatureProcessorInterfaceTypeId(FeatureProcessorId featureProcessorId);

        private:

            // Enable all registered feature processors for the scene
            void EnableAllForScene(Scene* scene);

            struct FeatureProcessorEntry
            {
                FeatureProcessorEntry(FeatureProcessorId featureProcessorId, AZ::TypeId typeId, FeatureProcessorId interfaceFeatureProcessorId, AZ::TypeId interfaceTypeId)
                    : m_featureProcessorId(featureProcessorId)
                    , m_typeId(typeId)
                    , m_interfaceFeatureProcessorId(interfaceFeatureProcessorId)
                    , m_interfaceTypeId(interfaceTypeId)
                {}

                FeatureProcessorId m_featureProcessorId;
                AZ::TypeId m_typeId;
                FeatureProcessorId m_interfaceFeatureProcessorId;
                AZ::TypeId m_interfaceTypeId;
            };

            using FeatureProcessorRegistry = AZStd::vector<FeatureProcessorEntry>;

            FeatureProcessorRegistry::const_iterator GetEntry(FeatureProcessorId featureProcessorId);

            FeatureProcessorRegistry m_registry;
        };

        // --- Template functions ---

        template<typename FeatureProcessorType>
        void FeatureProcessorFactory::RegisterFeatureProcessor()
        {
            static_assert(!AZStd::is_abstract<FeatureProcessorType>::value, "Cannot register an abstract feature processor. Register a specific implementation instead.");

            AZ::TypeId typeId = azrtti_typeid<FeatureProcessorType>();
            FeatureProcessorId featureProcessorId{ FeatureProcessorType::RTTI_TypeName() };

            auto foundIt = GetEntry(featureProcessorId);

            if (foundIt == AZStd::end(m_registry))
            {
                m_registry.emplace_back(featureProcessorId, typeId, FeatureProcessorId{}, AZ::TypeId::CreateNull());
            }
            else
            {
                AZ_Warning("FeatureProcessorFactory", false, "FeatureProcessor '%s' is already registered.", featureProcessorId.GetCStr());
            }
        }

        template<typename FeatureProcessorType, typename InterfaceType>
        void FeatureProcessorFactory::RegisterFeatureProcessorWithInterface()
        {
            static_assert(!AZStd::is_abstract<FeatureProcessorType>::value, "Cannot register an abstract feature processor. Register a specific implementation instead.");

            AZ::TypeId typeId = azrtti_typeid<FeatureProcessorType>();
            FeatureProcessorId featureProcessorId{ FeatureProcessorType::RTTI_TypeName() };

            AZ::TypeId interfaceTypeId = azrtti_typeid<InterfaceType>();
            FeatureProcessorId interfaceFeatureProcessorId{ InterfaceType::RTTI_TypeName() };

            auto foundIt = GetEntry(featureProcessorId);

            if (foundIt == AZStd::end(m_registry))
            {
                m_registry.emplace_back(featureProcessorId, typeId, interfaceFeatureProcessorId, interfaceTypeId);
            }
            else
            {
                AZ_Warning("FeatureProcessorFactory", false, "FeatureProcessor '%s' is already registered.", featureProcessorId.GetCStr());
            }
        }

        template<typename FeatureProcessorType>
        void FeatureProcessorFactory::UnregisterFeatureProcessor()
        {
            static_assert(!AZStd::is_abstract<FeatureProcessorType>::value, "Cannot unregister an abstract feature processor. Unregister a specific implementation instead.");

            FeatureProcessorId nameId{ FeatureProcessorType::RTTI_TypeName() };

            auto foundIt = GetEntry(nameId);

            if (foundIt != AZStd::end(m_registry))
            {
                m_registry.erase(foundIt);
            }
            else
            {
                AZ_Warning("FeatureProcessorFactory", false, "FeatureProcessor '%s' is already unregistered.", nameId.GetCStr());
            }
        }
    }
}
