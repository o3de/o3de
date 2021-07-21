/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <stdlib.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>


namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            namespace Utilities
            {

                template<typename T>
                bool IsNameAvailable(const AZStd::string& name, const Containers::SceneManifest& manifest)
                {
                    return IsNameAvailable(name, manifest, T::TYPEINFO_Uuid());
                }

                template<typename T>
                AZStd::string CreateUniqueName(const AZStd::string& baseName, const Containers::SceneManifest& manifest)
                {
                    return CreateUniqueName(baseName, manifest, T::TYPEINFO_Uuid());
                }

                template<typename T>
                AZStd::string CreateUniqueName(const AZStd::string& baseName, const AZStd::string& subName, const Containers::SceneManifest& manifest)
                {
                    return CreateUniqueName(baseName, subName, manifest, T::TYPEINFO_Uuid());
                }

            } // Utilities
        } // DataTypes
    } // SceneAPI
} // AZ
