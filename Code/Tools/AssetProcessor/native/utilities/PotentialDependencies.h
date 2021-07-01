/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ
{
    namespace Data
    {
        struct AssetId;
    }
}

namespace AssetProcessor
{
    class SpecializedDependencyScanner;
    
    /// Tracks additional information about a potential dependency, such as
    /// what string in the file is associated with the dependency, and what scanner.
    class PotentialDependencyMetaData
    {
    public:

        PotentialDependencyMetaData() { }

        PotentialDependencyMetaData(
            AZStd::string sourceString,
            AZStd::shared_ptr<SpecializedDependencyScanner> scanner) :
            m_sourceString(sourceString),
            m_scanner(scanner)
        {
        }

        // Needed to store these in a sorted container, which is used to guarantee
        // logs show up in the same order for every sacn.
        bool operator<(const PotentialDependencyMetaData& rhs) const
        {
            return m_sourceString < rhs.m_sourceString;
        }

        /// The portion of the scanned file that matches the missing dependency.
        AZStd::string m_sourceString;
        /// Which scanner found this dependency.
        AZStd::shared_ptr<SpecializedDependencyScanner> m_scanner;
    };

    /// Stores the collections of potential product dependencies found in a file.
    class PotentialDependencies
    {
    public:
        AZStd::set<PotentialDependencyMetaData> m_paths;
        // Using a map instead of a multimap to avoid polluting the results with the
        // same missing dependency. If a file references the same potential dependency
        // more than once, then only one result will be available.
        AZStd::map<AZ::Uuid, PotentialDependencyMetaData> m_uuids;
        AZStd::map<AZ::Data::AssetId, PotentialDependencyMetaData> m_assetIds;
    };
}
