/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
