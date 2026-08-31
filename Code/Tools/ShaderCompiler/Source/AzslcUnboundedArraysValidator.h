/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "GenericUtils.h"
#include "AzslcKindInfo.h"
#include "AzslcRegisters.h"

namespace AZ::ShaderCompiler
{
    //! Helper class for validation of all possible use cases for unbounded arrays.
    struct UnboundedArraysValidator final
    {
        UnboundedArraysValidator() = default;
        ~UnboundedArraysValidator() = default;

        struct Options
        {
            //! Takes the value of --unique-idx command line option.
            //! When false each register resource type b, t, u, s can have its own unbounded array per register space.
            //! When true only one of b, t, u, s can have the unbounded array.
            bool m_useUniqueIndicesEnabled = false;
            //! Takes the value of --max-spaces
            //! Max number of register spaces space0, space1, space2, ... space<m_optionMaxSpaces - 1>
            uint32_t m_maxSpaces = std::numeric_limits<uint32_t>::max();
        };

        //! asserts if at least one option has an invalid value.
        void SetOptions(const Options& options);

        //! Validates, semantically speaking, if a variable/field can be added to a SRG.
        bool CheckFieldCanBeAddedToSrg(bool isUnboundedArray, const IdentifierUID& srgUid, const IdentifierUID& varUid, const VarInfo& varInfo, TypeClass typeClass,
            string* errorMessage = nullptr);

    private:
        //! Helper for CheckFieldCanBeAddedToSrg. Only called if @varUid was declared as an unbounded array.
        //! If it returns false, *errorMessage will have the details.
        bool CheckUnboundedArrayFieldCanBeAddedToSrg(const IdentifierUID& srgUid, const IdentifierUID& varUid, const VarInfo& varInfo, TypeClass typeClass,
            string* errorMessage = nullptr);

        //! Returns the space index that corresponds to the given SRG.
        //! The calculated space index is stored in m_srgToSpaceIndex the first time
        //! this function is called for any given SRG.
        SpaceIndex GetSpaceIndexForSrg(const IdentifierUID& srgUid);

        //! Some of the command line options pertinent to validation of unbounded arrays.
        Options m_options;

        //! Key is an SRG Uid, value is its space index.
        unordered_map<IdentifierUID, SpaceIndex> m_srgToSpaceIndex;
        //! Keeps track of max SpaceIndex value in m_srgToSpaceIndex.
        SpaceIndex m_maxSpaceIndex = 0; 

    };
} // namespace AZ::ShaderCompiler
