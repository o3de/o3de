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
        void FeatureProcessorDeleter::operator()(FeatureProcessor* featureProcessor) const
        {
            auto featureProcessorLib = FeatureProcessorFactory::Get();

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            FeatureProcessorId featureProcessorId{ featureProcessor->RTTI_GetTypeName() };

            if (!serializeContext)
            {
                AZ_Warning("FeatureProcessorFactory", false, "FeatureProcessor '%s' could not be destroyed since could not retrieve serialize context.", featureProcessorId.GetCStr());
                return;
            }

            auto foundIt = featureProcessorLib->GetEntry(featureProcessorId);
            if (foundIt == AZStd::end(featureProcessorLib->m_registry))
            {
                AZ_Warning("FeatureProcessorFactory", false, "FeatureProcessor '%s' could not be destroyed since failed to find it in registry.", featureProcessorId.GetCStr());
                return;
            }

            auto* classData = serializeContext->FindClassData(foundIt->m_typeId);
            if (!classData)
            {
                AZ_Warning("FeatureProcessorFactory", false, "FeatureProcessor '%s' could not be destroyed since failed to get class data.", featureProcessorId.GetCStr());
                return;
            }
            
            classData->m_factory->Destroy(featureProcessor);
        }

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
                AZ_Warning("FeatureProcessorFactory", false, "FeatureProcessor '%s' could not be created since could not retrieve serialize context.", featureProcessorId.GetCStr());
                return nullptr;
            }

            auto foundIt = GetEntry(featureProcessorId);
            if (foundIt == AZStd::end(m_registry))
            {
                AZ_Warning("FeatureProcessorFactory", false, "FeatureProcessor '%s' could not be created since failed to find it in registry.", featureProcessorId.GetCStr());
                return nullptr;
            }

            auto* classData = serializeContext->FindClassData(foundIt->m_typeId);
            if (!classData)
            {
                AZ_Warning("FeatureProcessorFactory", false, "FeatureProcessor '%s' could not be created since failed to get class data.", featureProcessorId.GetCStr());
                return nullptr;
            }

            return FeatureProcessorPtr(reinterpret_cast<FeatureProcessor*>(classData->m_factory->Create("FeatureProcessor")));
        }

        AZ::TypeId FeatureProcessorFactory::GetFeatureProcessorTypeId(FeatureProcessorId featureProcessorId)
        {
            auto foundIt = GetEntry(featureProcessorId);
            if (foundIt == AZStd::end(m_registry))
            {
                AZ_Warning("FeatureProcessorFactory", false, "FeatureProcessor '%s' could not be found in registry.", featureProcessorId.GetCStr());
                return nullptr;
            }

            return foundIt->m_typeId;
        }

        AZ::TypeId FeatureProcessorFactory::GetFeatureProcessorInterfaceTypeId(FeatureProcessorId featureProcessorId)
        {
            auto foundIt = GetEntry(featureProcessorId);
            if (foundIt == AZStd::end(m_registry))
            {
                AZ_Warning("FeatureProcessorFactory", false, "FeatureProcessor '%s' could not be found in registry.", featureProcessorId.GetCStr());
                return nullptr;
            }

            return foundIt->m_interfaceTypeId;
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
