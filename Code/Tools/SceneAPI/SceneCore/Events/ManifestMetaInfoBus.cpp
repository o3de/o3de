/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Events
        {
            ManifestMetaInfo::ManifestMetaInfo()
            {
            }

            void ManifestMetaInfo::GetCategoryAssignments(CategoryRegistrationList& /*categories*/, const Containers::Scene& /*scene*/)
            {
            }

            void ManifestMetaInfo::GetIconPath(AZStd::string& /*iconPath*/, const DataTypes::IManifestObject& /*target*/)
            {
            }

            void ManifestMetaInfo::GetAvailableModifiers(ModifiersList& /*modifiers*/,
                const Containers::Scene& /*scene*/, const DataTypes::IManifestObject& /*target*/)
            {
            }

            void ManifestMetaInfo::InitializeObject(const Containers::Scene& /*scene*/, DataTypes::IManifestObject& /*target*/)
            {
            }

            void ManifestMetaInfo::ObjectUpdated(const Containers::Scene& /*scene*/, const DataTypes::IManifestObject* /*target*/, void* /*sender*/)
            {
            }
        } // namespace Events
    } // namespace SceneAPI
} // namespace AZ
