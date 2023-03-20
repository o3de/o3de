/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/StringFunc/StringFunc.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            namespace Utilities
            {
                bool IsNameAvailable(const AZStd::string& name, const Containers::SceneManifest& manifest, const Uuid& type)
                {
                    for (AZStd::shared_ptr<const IManifestObject> object : manifest.GetValueStorage())
                    {
                        if (object->RTTI_IsTypeOf(IGroup::TYPEINFO_Uuid()) && object->RTTI_IsTypeOf(type))
                        {
                            const IGroup* group = azrtti_cast<const IGroup*>(object.get());
                            if (AzFramework::StringFunc::Equal(group->GetName().c_str(), name.c_str()))
                            {
                                return false;
                            }
                        }
                    }
                    return true;
                }

                AZStd::string CreateUniqueName(const AZStd::string& baseName, const Containers::SceneManifest& manifest, const Uuid& type)
                {
                    int highestIndex = -1;
                    for (AZStd::shared_ptr<const IManifestObject> object : manifest.GetValueStorage())
                    {
                        if (object->RTTI_IsTypeOf(IGroup::TYPEINFO_Uuid()) && object->RTTI_IsTypeOf(type))
                        {
                            const IGroup* group = azrtti_cast<const IGroup*>(object.get());
                            const AZStd::string& groupName = group->GetName();
                            if (groupName.length() < baseName.length())
                            {
                                continue;
                            }

                            if (AzFramework::StringFunc::Equal(groupName.c_str(), baseName.c_str(), false, baseName.length()))
                            {
                                if (groupName.length() == baseName.length())
                                {
                                    highestIndex = AZStd::max(0, highestIndex);
                                }
                                else if (groupName[baseName.length()] == '-')
                                {
                                    int index = 0;
                                    if (AzFramework::StringFunc::LooksLikeInt(groupName.c_str() + baseName.length() + 1, &index))
                                    {
                                        highestIndex = AZStd::max(index, highestIndex);
                                    }
                                }
                            }
                        }
                    }

                    AZStd::string result;
                    if (highestIndex == -1)
                    {
                        result = baseName;
                    }
                    else
                    {
                        result = AZStd::string::format("%s-%i", baseName.c_str(), highestIndex + 1);
                    }

                    // Replace any characters that are invalid as part of a file name.
                    const char* invalidCharactersBegin = AZ_FILESYSTEM_INVALID_CHARACTERS;
                    const char* invalidCharactersEnd = invalidCharactersBegin + AZ_ARRAY_SIZE(AZ_FILESYSTEM_INVALID_CHARACTERS);
                    for (size_t i = 0; i < result.length(); ++i)
                    {
                        if (result[i] == AZ_FILESYSTEM_DRIVE_SEPARATOR || result[i] == AZ_FILESYSTEM_WILDCARD ||
                            result[i] == AZ_CORRECT_FILESYSTEM_SEPARATOR || result[i] == AZ_WRONG_FILESYSTEM_SEPARATOR ||
                            AZStd::find(invalidCharactersBegin, invalidCharactersEnd, result[i]) != invalidCharactersEnd)
                        {
                            result[i] = '_';
                        }
                    }

                    return result;
                }

                AZStd::string CreateUniqueName(const AZStd::string& baseName, const AZStd::string& subName,
                    const Containers::SceneManifest& manifest, const Uuid& type)
                {
                    return CreateUniqueName(AZStd::string::format("%s_%s", baseName.c_str(), subName.c_str()), manifest, type);
                }

                Uuid CreateStableUuid(const Containers::Scene& scene, const Uuid& typeId)
                {
                    return scene.GetSourceGuid() + typeId;
                }

                Uuid CreateStableUuid(const Containers::Scene& scene, const Uuid& typeId, const AZStd::string& subId)
                {
                    AZStd::string guid;
                    guid += scene.GetSourceGuid().ToString<AZStd::string>();
                    guid += typeId.ToString<AZStd::string>();
                    guid += subId;

                    return Uuid::CreateData(guid);
                }

                Uuid CreateStableUuid(const Containers::Scene& scene, const Uuid& typeId, const char* subId)
                {
                    AZStd::string guid;
                    guid += scene.GetSourceGuid().ToString<AZStd::string>();
                    guid += typeId.ToString<AZStd::string>();
                    guid += subId;

                    return Uuid::CreateData(guid);
                }
            } // namespace Utilities
        } // namespace DataTypes
    } // namespace SceneAPI
} // namespace AZ
