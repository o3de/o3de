/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/Scene.h>

#include <AzCore/Interface/Interface.h>

#include <AzCore/Component/ComponentApplicationBus.h>

namespace AZ
{
    namespace RPI
    {

        FeatureProcessorFactory* FeatureProcessorFactory::Get()
        {
            return Interface<FeatureProcessorFactory>::Get();
        }

        FeatureProcessorFactory::FeatureProcessorFactory()
        {
        }

        void FeatureProcessorFactory::Init()
        {
            Interface<FeatureProcessorFactory>::Register(this);
        }

        void FeatureProcessorFactory::Shutdown()
        {
            Interface<FeatureProcessorFactory>::Unregister(this);
        }

        FeatureProcessorPtr FeatureProcessorFactory::CreateFeatureProcessor(FeatureProcessorId featureProcessorId)
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

            if (!serializeContext)
            {
                AZ_Warning("FeatureProcessorFactory", false, "Provided type '%s' could not be created since the serialize context could not be retrieved.", featureProcessorId.GetCStr());
                return nullptr;
            }

            AZ::Uuid typeId{};

            // First check the registry for the feature processor id, otherwise fall back on the serialize context.
            auto foundIt = GetEntry(featureProcessorId);
            if (foundIt != AZStd::end(m_registry))
            {
                typeId = foundIt->m_typeId;
            }
            else
            {
                auto foundUuids = serializeContext->FindClassId(AZ::Crc32(featureProcessorId.GetStringView()));

                if (foundUuids.empty())
                {
                    AZ_Warning("FeatureProcessorFactory", false, "Provided type %s is either an invalid TypeId or does not match any class names", featureProcessorId.GetCStr());
                }
                else
                {
                    typeId = foundUuids[0];
                }
            }

            // Create the class from the type id.
            auto* classData = serializeContext->FindClassData(typeId);
            if (!classData)
            {
                AZ_Warning("FeatureProcessorFactory", false, "Provided type '%s' could not be created since failed to get class data. Make sure it was reflected to the serialize context.", featureProcessorId.GetCStr());
                return nullptr;
            }
            else if (!(classData->m_azRtti && classData->m_azRtti->IsTypeOf(FeatureProcessor::RTTI_Type())))
            {
                AZ_Warning("FeatureProcessorFactory", false, "Provided type %s is not a Feature Processor, or could not find RTTI information.", featureProcessorId.GetCStr());
                return nullptr;
            }
            return FeatureProcessorPtr(reinterpret_cast<FeatureProcessor*>(classData->m_factory->Create("FeatureProcessor")));
        }

        AZ::TypeId FeatureProcessorFactory::GetFeatureProcessorTypeId(FeatureProcessorId featureProcessorId)
        {
            auto foundIt = GetEntry(featureProcessorId);
            return foundIt == AZStd::end(m_registry) ? AZ::TypeId{} : foundIt->m_typeId;
        }

        AZ::TypeId FeatureProcessorFactory::GetFeatureProcessorInterfaceTypeId(FeatureProcessorId featureProcessorId)
        {
            auto foundIt = GetEntry(featureProcessorId);
            return foundIt == AZStd::end(m_registry) ? AZ::TypeId{} : foundIt->m_interfaceTypeId;
        }

        FeatureProcessorFactory::FeatureProcessorRegistry::const_iterator FeatureProcessorFactory::GetEntry(FeatureProcessorId featureProcessorId)
        {
            auto findFn = [featureProcessorId](const FeatureProcessorEntry& entry)
            {
                return entry.m_featureProcessorId == featureProcessorId;
            };

            return AZStd::find_if(AZStd::begin(m_registry), AZStd::end(m_registry), findFn);
        }

        void FeatureProcessorFactory::EnableAllForScene(Scene* scene)
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

            if (!serializeContext)
            {
                AZ_Warning("FeatureProcessorFactory", false, "Enable feature processors requires a valid SerializeContext");
                return;
            }

            for (auto& entry : m_registry)
            {
                auto* classData = serializeContext->FindClassData(entry.m_typeId);
                if (!classData)
                {
                    AZ_Warning("FeatureProcessorFactory", false, "Can't create feature processor [%s] since we failed to get class data ", entry.m_featureProcessorId.GetCStr());
                    continue;
                }

                FeatureProcessorPtr fp = FeatureProcessorPtr(reinterpret_cast<FeatureProcessor*>(classData->m_factory->Create("FeatureProcessor")));
                scene->AddFeatureProcessor(AZStd::move(fp));
            }
        }
    }
}
