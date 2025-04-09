/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SurfaceData/SurfaceTag.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SurfaceData/SurfaceDataTagProviderRequestBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/sort.h>

AZ_DECLARE_BUDGET(SurfaceData);

namespace SurfaceData
{
    namespace SurfaceTagUtil
    {
        static bool UpdateVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < 1)
            {
                AZStd::string surfaceTag;
                if (classElement.GetChildData(AZ_CRC_CE("SurfaceTag"), surfaceTag))
                {
                    classElement.RemoveElementByName(AZ_CRC_CE("SurfaceTag"));
                    classElement.AddElementWithData(context, "SurfaceTagCrc", (AZ::u32)(AZ::Crc32(surfaceTag.data())));
                }
            }
            if (classElement.GetVersion() < 2)
            {
                SurfaceTag surfaceTag;
                if (classElement.GetData(surfaceTag))
                {
                    if (surfaceTag == AZ_CRC_CE("(default)"))
                    {
                        surfaceTag = Constants::s_unassignedTagCrc;
                        classElement.SetData(context, surfaceTag);
                    }
                }
            }
            return true;
        }
    }

    void SurfaceTag::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceTag>()
                ->Version(2, &SurfaceTagUtil::UpdateVersion)
                ->Field("SurfaceTagCrc", &SurfaceTag::m_surfaceTagCrc)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<SurfaceTag>(
                    "Surface Tag", "Matches a surface value like a mask or material")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SurfaceTag::m_surfaceTagCrc, "Surface Tag", "Matches a surface value like a mask or material")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &SurfaceTag::BuildSelectableTagList)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SurfaceTag>()
                ->Constructor()
                ->Constructor<const AZStd::string&>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Attribute(AZ::Script::Attributes::Module, "surface_data")
                ->Method("SetTag", &SurfaceTag::SetTag)
                ->Method("Equal", &SurfaceTag::operator==)
                ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                ->Method(
                    "ToString",
                    [](const SurfaceTag& tag) -> AZStd::string
                    {
                        return tag.GetDisplayName();
                    })
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;
        }
    }

    AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>> SurfaceTag::GetRegisteredTags()
    {
        AZ_PROFILE_FUNCTION(SurfaceData);

        SurfaceTagNameSet labels;
        SurfaceDataTagProviderRequestBus::Broadcast(&SurfaceDataTagProviderRequestBus::Events::GetRegisteredSurfaceTagNames, labels);
        labels.insert(Constants::s_unassignedTagName);

        AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>> registeredTags;
        registeredTags.reserve(labels.size());
        for (const auto& label : labels)
        {
            const AZ::u32 crc = AZ::Crc32(label.data());

            //warn when two tags have the same crc
            auto entryItr = AZStd::find_if(registeredTags.begin(), registeredTags.end(), [crc](const auto& entry) {return entry.first == crc;});
            if (entryItr != registeredTags.end())
            {
                AZ_Warning("SurfaceData", false, "SurfaceTag CRC collision between \"%s\" and \"%s\"!  \"%s\" not added.", entryItr->second.data(), label.data(), label.data());
                continue;
            }

            registeredTags.push_back({ crc, label });
        }

        return registeredTags;
    }

    bool SurfaceTag::FindDisplayName(const AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>>& selectableTags, AZStd::string& name) const
    {
        auto it = AZStd::find_if(
            selectableTags.begin(),
            selectableTags.end(),
            [this](const auto& entry) {return m_surfaceTagCrc == entry.first; });

        if (it == selectableTags.end())
        {
            //if a match was not found, generate a name using the crc
            name = AZStd::string::format("(unregistered %u)", m_surfaceTagCrc);
            return false;
        }

        name = it->second;
        return true;
    }

    AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>> SurfaceTag::BuildSelectableTagList() const
    {
        AZ_PROFILE_FUNCTION(SurfaceData);

        AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>> selectableTags = GetRegisteredTags();

        AZStd::string name;
        if (!FindDisplayName(selectableTags, name))
        {
            //if a match was not found, add the generated name to the selectable set
            selectableTags.push_back({ m_surfaceTagCrc, name });
            AZ_Warning("SurfaceData", false, "SurfaceTag CRC %u is not a registered tag.", m_surfaceTagCrc);
        }

        AZStd::sort(selectableTags.begin(), selectableTags.end(), [](const auto& lhs, const auto& rhs) {return lhs.second < rhs.second;});
        return selectableTags;
    }

    AZStd::string SurfaceTag::GetDisplayName() const
    {
        AZ_PROFILE_FUNCTION(SurfaceData);

        AZStd::string name;
        FindDisplayName(GetRegisteredTags(), name);
        return name;
    }
}
