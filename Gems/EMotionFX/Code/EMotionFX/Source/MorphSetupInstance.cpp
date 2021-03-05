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

// include the required headers
#include "MorphSetupInstance.h"
#include "MorphSetup.h"
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MorphSetupInstance, DeformerAllocator, 0)


    // default constructor
    MorphSetupInstance::MorphSetupInstance()
        : BaseObject()
    {
        mMorphTargets.SetMemoryCategory(EMFX_MEMCATEGORY_GEOMETRY_PMORPHTARGETS);
        Init(nullptr);
    }


    // constructor
    MorphSetupInstance::MorphSetupInstance(MorphSetup* morphSetup)
    {
        Init(morphSetup);
    }


    // destructor
    MorphSetupInstance::~MorphSetupInstance()
    {
    }


    // create
    MorphSetupInstance* MorphSetupInstance::Create()
    {
        return aznew MorphSetupInstance();
    }


    // extended create
    MorphSetupInstance* MorphSetupInstance::Create(MorphSetup* morphSetup)
    {
        return aznew MorphSetupInstance(morphSetup);
    }


    // initialize the setup instance
    void MorphSetupInstance::Init(MorphSetup* morphSetup)
    {
        // don't do anything if we there is no morph setup
        if (morphSetup == nullptr)
        {
            return;
        }

        // allocate the number of morph targets
        const uint32 numMorphTargets = morphSetup->GetNumMorphTargets();
        mMorphTargets.Resize(numMorphTargets);

        // update the ID values
        for (uint32 i = 0; i < numMorphTargets; ++i)
        {
            mMorphTargets[i].SetID(morphSetup->GetMorphTarget(i)->GetID());
        }
    }


    // try to locate the morph target by ID
    uint32 MorphSetupInstance::FindMorphTargetIndexByID(uint32 id) const
    {
        // try to locate the morph target with the given ID
        const uint32 numTargets = mMorphTargets.GetLength();
        for (uint32 i = 0; i < numTargets; ++i)
        {
            if (mMorphTargets[i].GetID() == id)
            {
                return i;
            }
        }

        // there is no such morph target with the given ID
        return MCORE_INVALIDINDEX32;
    }


    MorphSetupInstance::MorphTarget* MorphSetupInstance::FindMorphTargetByID(uint32 id)
    {
        const uint32 index = FindMorphTargetIndexByID(id);
        if (index != MCORE_INVALIDINDEX32)
        {
            return &mMorphTargets[index];
        }
        else
        {
            return nullptr;
        }
    }
} // namespace EMotionFX
