/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/sort.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>
#include <Config/SettingsObjects/SoftNameSetting.h>

namespace AZ
{
    namespace SceneProcessingConfig
    {
        SoftNameSetting::SoftNameSetting(const char* pattern, SceneAPI::SceneCore::PatternMatcher::MatchApproach approach, 
            const char* virtualType)
            : m_pattern(pattern, approach)
            , m_virtualType(virtualType)
        {
        }

        SoftNameSetting::~SoftNameSetting() = default;

        Crc32 SoftNameSetting::GetVirtualTypeHash() const
        {
            if (m_virtualTypeHash == Crc32())
            {
                m_virtualTypeHash = Crc32(m_virtualType.c_str());
            }
            return m_virtualTypeHash;
        }

        const AZStd::string& SoftNameSetting::GetVirtualType() const
        {
            return m_virtualType;
        }

        void SoftNameSetting::Reflect(ReflectContext* context)
        {
            SerializeContext* serialize = azrtti_cast<SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<SoftNameSetting>()
                    ->Version(1)
                    ->Field("pattern", &SoftNameSetting::m_pattern)
                    ->Field("virtualType", &SoftNameSetting::m_virtualType);

                EditContext* editContext = serialize->GetEditContext();
                if (editContext)
                {
                    editContext->Class<SoftNameSetting>("Soft name setting", "A pattern matcher to setup project specific naming conventions.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute(Edit::Attributes::AutoExpand, true)
                            ->Attribute(Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly"))
                        ->DataElement(Edit::UIHandlers::Default, &SoftNameSetting::m_pattern, "Pattern", 
                            "The pattern the matcher will check against.")
                            ->Attribute(Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly"))
                        ->DataElement(Edit::UIHandlers::ComboBox, &SoftNameSetting::m_virtualType, "Virtual Type", 
                            "The node(s) will be converted to this type after their pattern matches.")
                            ->Attribute(Edit::Attributes::StringList, &SoftNameSetting::GetAllVirtualTypes);
                }
            }
        }

        AZStd::vector<AZStd::string> SoftNameSetting::GetAllVirtualTypes() const
        {
            using namespace SceneAPI::Events;

            GraphMetaInfo::VirtualTypesSet virtualTypes;
            GraphMetaInfoBus::Broadcast(&GraphMetaInfoBus::Events::GetAllVirtualTypes, virtualTypes);

            AZStd::vector<AZStd::string> result;
            for (Crc32 virtualType : virtualTypes)
            {
                AZStd::string virtualTypeName;
                GraphMetaInfoBus::Broadcast(&GraphMetaInfoBus::Events::GetVirtualTypeName, virtualTypeName, virtualType);

                AZ_Assert(!virtualTypeName.empty(), "No name found for virtual type with hash %i.", static_cast<u32>(virtualType));
                result.emplace_back(AZStd::move(virtualTypeName));
            }

            AZStd::sort(result.begin(), result.end());

            return result;
        }
    } // namespace SceneProcessingConfig
} // namespace AZ
