/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/EBus/EBus.h>
#include <SceneAPI/SceneCore/Utilities/PatternMatcher.h>

namespace AZ
{
    namespace SceneProcessingConfig
    {
        class SoftNameSetting;
        class SceneProcessingConfigRequests
            : public EBusTraits
        {

        public:
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
            static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::Single;
            
            virtual const AZStd::vector<AZStd::unique_ptr<SoftNameSetting>>* GetSoftNames() = 0;
            
            /**
             * @brief Adds a virtual type for matching against the name of scene nodes.
             * @pattern: The string pattern that will be used to match the name of nodes within asset files.
             *     If \p approach == PreFix, then \p pattern looks like "name_". And a node named "name_abc" would match.
             *     If \p approach == PostFix, then \p pattern looks like "_name". And a node named "abc_name" would match.
             *     If \p approach == RegEx, the \p pattern is a regular expression.
             * @approach: See \p pattern.
             * @virtualType: This string will be internally CRC32'ed. For nodes that match \p pattern, this will be
             *     the virtual type of the node.
             * @includeChildren: For each parent node, If true, it will do pattern matching across the children nodes,
             *     otherwise the pattern matching will always stop at root nodes. 
             * @returns: true if the new \p virtualType doesn't exist already (matched by Crc) AND
             *     it is added to the end of the list.
             * @pre
             *     Here is an example:
                   AddNodeSoftName("_lod1", PatternMatcher::MatchApproach::PostFix, "LODMesh1", true)
             */
            virtual bool AddNodeSoftName(const char* pattern, SceneAPI::SceneCore::PatternMatcher::MatchApproach approach,
                const char* virtualType, bool includeChildren) = 0;


            /**
             * @brief Adds a virtual type for matching against the name of asset files.
             * @pattern See AddNodeSoftName(...)
             * @approach See AddNodeSoftName(...)
             * @virtualType See AddNodeSoftName(...)
             * @inclusive 1. If the asset file name doesn't match the pattern then the value
             *     of this flag is irrelevant. No VirtualType will be assigned to any root node
             *     within the asset file.
             *     2. If the asset file name MATCHES the pattern, then:
             *         2.1. If at least one root node of type \p graphObjectTypeName
             *              is found in the scene then the Virtual Type is assigned or NOT
             *              depending on the value of this parameter.
             *         2.2. If none of the root nodes are of type \p graphObjectTypeName
             *              then the Virtual Type is assigned or NOT
             *              depending on the NEGATED value of this parameter.
             * @graphObjectTypeName TYPEINFO_Name() of a SceneAPI::DataTypes::IGraphObject derived class.
             *     e.g. SceneAPI::DataTypes::IAnimationData::TYPEINFO_Name().
             * @returns See AddNodeSoftName(...)
             * @pre
             *     Example:
             *     AddFileSoftName("_anim", PatternMatcher::MatchApproach::PostFix, "Ignore", false,
             *                     SceneAPI::DataTypes::IAnimationData::TYPEINFO_Name())
             *     If the filename ends with "_anim" this will mark all nodes as "Ignore" unless they're derived from IAnimationData.
             *     This will cause only animations to be exported from the source scene file even if there's other data available.
             */
            virtual bool AddFileSoftName(const char* pattern, SceneAPI::SceneCore::PatternMatcher::MatchApproach approach,
                const char* virtualType, bool inclusive, const AZStd::string& graphObjectTypeName) = 0;
        };

        using SceneProcessingConfigRequestBus = EBus<SceneProcessingConfigRequests>;
    } // namespace SceneProcessingConfig
} // namespace AZ
