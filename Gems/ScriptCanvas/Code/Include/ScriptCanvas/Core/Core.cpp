/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/StringFunc/StringFunc.h>

#include "Core.h"
#include "Attributes.h"

namespace ScriptCanvas
{
    ScopedAuxiliaryEntityHandler::ScopedAuxiliaryEntityHandler(AZ::Entity* buildEntity)
        : m_buildEntity(buildEntity)
        , m_wasAdded(false)
    {
        if (AZ::Interface<AZ::ComponentApplicationRequests>::Get() != nullptr)
        {
            AZ::Interface<AZ::ComponentApplicationRequests>::Get()->RemoveEntity(buildEntity);
        }

        if (buildEntity->GetState() == AZ::Entity::State::Constructed)
        {
            buildEntity->Init();
            m_wasAdded = true;
        }
    }

    ScopedAuxiliaryEntityHandler::~ScopedAuxiliaryEntityHandler()
    {
        if (!m_wasAdded)
        {
            if (AZ::Interface<AZ::ComponentApplicationRequests>::Get() != nullptr)
            {
                AZ::Interface<AZ::ComponentApplicationRequests>::Get()->AddEntity(m_buildEntity);
            }
        }
    }

    bool IsNamespacePathEqual(const NamespacePath& lhs, const NamespacePath& rhs)
    {
        if (lhs.size() != rhs.size())
        {
            return false;
        }
        auto lhsIter = lhs.begin();
        auto rhsIter = rhs.begin();

        for (; lhsIter != lhs.end(); ++lhsIter, ++rhsIter)
        {
            if (!AZ::StringFunc::Equal(lhsIter->c_str(), rhsIter->c_str()))
            {
                return false;
            }
        }

        return true;
    }

    void DependencyReport::MergeWith(const DependencyReport& other)
    {
        auto unite = [](auto& target, const auto& source)
        {
            for (const auto& entry : source)
            {
                target.insert(entry);
            }
        };

        unite(nativeLibraries, other.nativeLibraries);
        unite(userSubgraphs, other.userSubgraphs);
        unite(scriptEventsAssetIds, other.scriptEventsAssetIds);
        unite(userSubgraphAssetIds, other.userSubgraphAssetIds);
    }

    DependencyReport DependencyReport::NativeLibrary(const AZStd::string& library)
    {
        DependencyReport report;
        report.nativeLibraries.insert({ library });
        return report;
    }

    bool SlotIdVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        //! Version 1: Slot Ids contained a Crc32 hash of the name given
        //! Version 2+: Slot Ids now contain a random Uuid
        //! The converter below reads in the old GraphCanvas node <-> ScriptCanvas node map and then iterates over all the 
        //! GraphCanvas nodes adding a reference to the ScriptCanvas node in it's user data field
        if (classElement.GetVersion() <= 1)
        {
            if (!classElement.RemoveElementByName(AZ_CRC("m_id", 0x7108ece0)))
            {
                return false;
            }

            if (classElement.RemoveElementByName(AZ_CRC("m_name", 0xc08c4427)))
            {
                return false;
            }

            classElement.AddElementWithData(context, "m_id", AZ::Uuid::CreateRandom());
        }

        return true;
    }

    void SlotId::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<SlotId>()
                ->Version(2, &SlotIdVersionConverter)
                ->Field("m_id", &SlotId::m_id)
                ;
        }

        if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SlotId>("SlotId")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All);
        }
    }

    void VersionData::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<VersionData>()
                ->Version(2, [](AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
                    {
                        if (classElement.GetVersion() < 2)
                        {
                            FileVersion fileVersion = ScriptCanvas::FileVersion::Initial;
                            classElement.AddElementWithData(context, "_fileVersion", fileVersion);
                        }

                        return true;
                    })
                ->Field("_grammarVersion", &VersionData::grammarVersion)
                ->Field("_runtimeVersion", &VersionData::runtimeVersion)
                ->Field("_fileVersion", &VersionData::fileVersion)
                ;
        }
    }

    VersionData VersionData::Latest()
    {
        VersionData data;
        data.MarkLatest();
        return data;
    }

    void VersionData::MarkLatest()
    {
        grammarVersion = GrammarVersion::Current;
        runtimeVersion = RuntimeVersion::Current;
        fileVersion = FileVersion::Current;
    }

    void ReflectEventTypeOnDemand(const AZ::TypeId& typeId, AZStd::string_view name, AZ::IRttiHelper* rttiHelper)
    {
        AZ::SerializeContext* serializeContext{};
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ::SerializeContext::ClassData classData;
        classData.m_name = name.data();
        classData.m_typeId = typeId;
        classData.m_azRtti = rttiHelper;

        auto EventPlaceholderAnyCreator = [](AZ::SerializeContext*) -> AZStd::any
        {
            return AZStd::make_any<AZStd::monostate>();
        };

        serializeContext->RegisterType(typeId, AZStd::move(classData), EventPlaceholderAnyCreator);
    }

}
