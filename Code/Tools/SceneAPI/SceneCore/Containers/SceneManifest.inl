/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Trace.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            bool SceneManifest::IsEmpty() const
            {
                // Any of the containers would do as they should be in-sync with each other, so pick one arbitrarily.
                AZ_Assert(m_values.empty() == m_storageLookup.empty(), "SceneManifest values and storage-lookup tables have gone out of lockstep.");
                return m_values.empty();
            }

            bool SceneManifest::AddEntry(const AZStd::shared_ptr<DataTypes::IManifestObject>& value)
            {
                return AddEntry(AZStd::shared_ptr<DataTypes::IManifestObject>(value));
            }

            bool SceneManifest::RemoveEntry(const AZStd::shared_ptr<DataTypes::IManifestObject>& value)
            {
                return RemoveEntry(value.get());
            }

            size_t SceneManifest::GetEntryCount() const
            {
                // Any of the containers would do as they should be in-sync with each other, so pick one randomly.
                AZ_Assert(m_values.size() == m_storageLookup.size(), 
                    "SceneManifest values and storage-lookup tables have gone out of lockstep. (%i vs. %i)", 
                    m_values.size(), m_storageLookup.size());
                return m_values.size();
            }

            AZStd::shared_ptr<DataTypes::IManifestObject> SceneManifest::GetValue(Index index)
            {
                return index < m_values.size() ? m_values[index] : AZStd::shared_ptr<DataTypes::IManifestObject>();
            }

            AZStd::shared_ptr<const DataTypes::IManifestObject> SceneManifest::GetValue(Index index) const
            {
                return index < m_values.size() ? m_values[index] : AZStd::shared_ptr<const DataTypes::IManifestObject>();
            }

            SceneManifest::Index SceneManifest::FindIndex(const AZStd::shared_ptr<DataTypes::IManifestObject>& value) const
            {
                return FindIndex(value.get());
            }

            SceneManifest::ValueStorageData SceneManifest::GetValueStorage()
            {
                return ValueStorageData(m_values.begin(), m_values.end());
            }

            SceneManifest::ValueStorageConstData SceneManifest::GetValueStorage() const
            {
                return ValueStorageConstData(
                    Views::MakeConvertIterator(m_values.cbegin(), SceneManifestConstDataConverter),
                    Views::MakeConvertIterator(m_values.cend(), SceneManifestConstDataConverter));
            }
        } // Containers
    } // SceneAPI
} // AZ
