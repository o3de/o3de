/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/EntityCompositionRequest.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzCore/Serialization/EditContext.h>

namespace AzToolsFramework
{
    bool AppearsInAddComponentMenu(const AZ::SerializeContext::ClassData& classData, const AZ::Crc32& entityType)
    {
        if (classData.m_editData)
        {
            if (auto editorDataElement = classData.m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
            {
                for (const AZ::Edit::AttributePair& attribPair : editorDataElement->m_attributes)
                {
                    if (attribPair.first == AZ::Edit::Attributes::AppearsInAddComponentMenu)
                    {
                        PropertyAttributeReader reader(nullptr, attribPair.second);
                        AZ::Crc32 classEntityType = 0;
                        AZStd::vector<AZ::Crc32> classEntityTypes;

                        if (reader.Read<AZ::Crc32>(classEntityType))
                        {
                            if (static_cast<AZ::u32>(entityType) == classEntityType)
                            {
                                return true;
                            }
                        }
                        else if (reader.Read<AZStd::vector<AZ::Crc32>>(classEntityTypes))
                        {
                            if (AZStd::find(classEntityTypes.begin(), classEntityTypes.end(), entityType) != classEntityTypes.end())
                            {
                                return true;
                            }
                        }
                    }
                }
            }
        }
        return false;
    }

    bool AppearsInGameComponentMenu(const AZ::SerializeContext::ClassData& classData)
    {
        // We don't call AppearsInAddComponentMenu(...) because we support legacy values.
        // AppearsInAddComponentMenu used to be a bool,
        // and it used to only be applied to components on in-game entities.
        if (classData.m_editData)
        {
            if (auto editorDataElement = classData.m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
            {
                for (const AZ::Edit::AttributePair& attribPair : editorDataElement->m_attributes)
                {
                    if (attribPair.first == AZ::Edit::Attributes::AppearsInAddComponentMenu)
                    {
                        PropertyAttributeReader reader(nullptr, attribPair.second);
                        AZ::Crc32 classEntityType;
                        AZStd::vector<AZ::Crc32> classEntityTypes;
                        if (reader.Read<AZ::Crc32>(classEntityType))
                        {
                            if (classEntityType == AZ_CRC_CE("Game"))
                            {
                                return true;
                            }
                        }
                        else if (reader.Read<AZStd::vector<AZ::Crc32>>(classEntityTypes))
                        {
                            if (AZStd::find(classEntityTypes.begin(), classEntityTypes.end(), AZ_CRC_CE("Game")) != classEntityTypes.end())
                            {
                                return true;
                            }
                        }

                        bool legacyAppearsInComponentMenu = false;
                        if (reader.Read<bool>(legacyAppearsInComponentMenu))
                        {
                            AZ_WarningOnce(classData.m_name, false, "%s %s 'AppearsInAddComponentMenu' uses legacy value 'true', should be 'AZ_CRC(\"Game\")'.",
                                classData.m_name, classData.m_typeId.ToString<AZStd::string>().c_str());
                            return legacyAppearsInComponentMenu;
                        }
                    }
                }
            }
        }
        return false;
    }
} // AzToolsFramework
