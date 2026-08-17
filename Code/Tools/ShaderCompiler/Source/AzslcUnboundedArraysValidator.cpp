/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzslcUnboundedArraysValidator.h"

namespace AZ::ShaderCompiler
{
    void UnboundedArraysValidator::SetOptions(const Options& options)
    {
        if (!options.m_maxSpaces)
        {
            throw std::runtime_error{ "--max-spaces must be greater than 0" };
        }
        m_options = options;
    }

    bool UnboundedArraysValidator::CheckUnboundedArrayFieldCanBeAddedToSrg(const IdentifierUID& srgUid, const IdentifierUID& varUid, const VarInfo& varInfo, TypeClass typeClass,
        string* errorMessage)
    {
        if (!CanBeDeclaredAsUnboundedArray(typeClass))
        {
            if (errorMessage)
            {
                *errorMessage = ConcatString("Data type of ( ", varUid.GetName(), " ) can not be packed as unbounded array");
            }
            return false;
        }

        return true;
    }

    bool UnboundedArraysValidator::CheckFieldCanBeAddedToSrg(bool isUnboundedArray, const IdentifierUID& srgUid, const IdentifierUID& varUid, const VarInfo& varInfo, TypeClass typeClass,
        string* errorMessage)
    {
        if (isUnboundedArray)
        {
            return CheckUnboundedArrayFieldCanBeAddedToSrg(srgUid, varUid, varInfo, typeClass, errorMessage);
        }

        auto spaceIndex = GetSpaceIndexForSrg(srgUid);
        if (spaceIndex >= FirstUnboundedSpace)
        {
            if (errorMessage)
            {
                *errorMessage = ConcatString("The number of SRG register spaces overflowed into the spaces reserved for unbounded arrays.");
                return false;
            }
        }

        return true;
    }

    SpaceIndex UnboundedArraysValidator::GetSpaceIndexForSrg(const IdentifierUID& srgUid)
    {
        SpaceIndex spaceIndex = 0;
        auto findIt = m_srgToSpaceIndex.find(srgUid);
        if (findIt == m_srgToSpaceIndex.end())
        {
            if (m_options.m_maxSpaces - 1 > m_maxSpaceIndex)
            {
                spaceIndex = static_cast<SpaceIndex>(m_srgToSpaceIndex.size());
                m_maxSpaceIndex = std::max(spaceIndex, m_maxSpaceIndex);
            }
            else
            {
                spaceIndex = m_maxSpaceIndex;
            }

            m_srgToSpaceIndex.emplace(srgUid, spaceIndex);
        }
        else
        {
            spaceIndex = findIt->second;
        }
        return spaceIndex;
    }


} // namespace AZ::ShaderCompiler
