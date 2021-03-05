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
#include "MorphSetup.h"
#include "MorphTarget.h"
#include <MCore/Source/StringConversions.h>
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MorphSetup, DeformerAllocator, 0)


    // constructor
    MorphSetup::MorphSetup()
        : BaseObject()
    {
        mMorphTargets.SetMemoryCategory(EMFX_MEMCATEGORY_GEOMETRY_PMORPHTARGETS);
    }


    // destructor
    MorphSetup::~MorphSetup()
    {
        RemoveAllMorphTargets();
    }


    // create
    MorphSetup* MorphSetup::Create()
    {
        return aznew MorphSetup();
    }


    // add a morph target
    void MorphSetup::AddMorphTarget(MorphTarget* morphTarget)
    {
        mMorphTargets.Add(morphTarget);
    }


    // remove a morph target
    void MorphSetup::RemoveMorphTarget(uint32 nr, bool delFromMem)
    {
        if (delFromMem)
        {
            mMorphTargets[nr]->Destroy();
        }

        mMorphTargets.Remove(nr);
    }


    // remove a morph target
    void MorphSetup::RemoveMorphTarget(MorphTarget* morphTarget, bool delFromMem)
    {
        mMorphTargets.RemoveByValue(morphTarget);

        if (delFromMem)
        {
            morphTarget->Destroy();
        }
    }


    // remove all morph targets
    void MorphSetup::RemoveAllMorphTargets()
    {
        const uint32 numTargets = mMorphTargets.GetLength();
        for (uint32 i = 0; i < numTargets; ++i)
        {
            mMorphTargets[i]->Destroy();
        }

        mMorphTargets.Clear();
    }


    // get a morph target by ID
    MorphTarget* MorphSetup::FindMorphTargetByID(uint32 id) const
    {
        // linear search, and check IDs
        const uint32 numTargets = mMorphTargets.GetLength();
        for (uint32 i = 0; i < numTargets; ++i)
        {
            if (mMorphTargets[i]->GetID() == id)
            {
                return mMorphTargets[i];
            }
        }

        // nothing found
        return nullptr;
    }


    // get a morph target number by ID
    uint32 MorphSetup::FindMorphTargetNumberByID(uint32 id) const
    {
        // linear search, and check IDs
        const uint32 numTargets = mMorphTargets.GetLength();
        for (uint32 i = 0; i < numTargets; ++i)
        {
            if (mMorphTargets[i]->GetID() == id)
            {
                return i;
            }
        }

        // nothing found
        return MCORE_INVALIDINDEX32;
    }


    uint32 MorphSetup::FindMorphTargetIndexByName(const char* name) const
    {
        const uint32 numTargets = mMorphTargets.GetLength();
        for (uint32 i = 0; i < numTargets; ++i)
        {
            if (mMorphTargets[i]->GetNameString() == name)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    uint32 MorphSetup::FindMorphTargetIndexByNameNoCase(const char* name) const
    {
        const uint32 numTargets = mMorphTargets.GetLength();
        for (uint32 i = 0; i < numTargets; ++i)
        {
            if (AzFramework::StringFunc::Equal(mMorphTargets[i]->GetNameString().c_str(), name, false /* no case */))
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find a morph target by name (case sensitive)
    MorphTarget* MorphSetup::FindMorphTargetByName(const char* name) const
    {
        const uint32 numTargets = mMorphTargets.GetLength();
        for (uint32 i = 0; i < numTargets; ++i)
        {
            if (mMorphTargets[i]->GetNameString() == name)
            {
                return mMorphTargets[i];
            }
        }

        return nullptr;
    }


    // find a morph target by name (not case sensitive)
    MorphTarget* MorphSetup::FindMorphTargetByNameNoCase(const char* name) const
    {
        const uint32 numTargets = mMorphTargets.GetLength();
        for (uint32 i = 0; i < numTargets; ++i)
        {
            if (AzFramework::StringFunc::Equal(mMorphTargets[i]->GetNameString().c_str(), name, false /* no case */))
            {
                return mMorphTargets[i];
            }
        }

        return nullptr;
    }


    // clone the morph setup
    MorphSetup* MorphSetup::Clone() const
    {
        // create the clone
        MorphSetup* clone = MorphSetup::Create();

        // clone all morph targets
        const uint32 numMorphTargets = mMorphTargets.GetLength();
        for (uint32 i = 0; i < numMorphTargets; ++i)
        {
            clone->AddMorphTarget(mMorphTargets[i]->Clone());
        }

        // return the cloned morph setup
        return clone;
    }


    void MorphSetup::ReserveMorphTargets(uint32 numMorphTargets)
    {
        mMorphTargets.Reserve(numMorphTargets);
    }


    // scale the morph data
    void MorphSetup::Scale(float scaleFactor)
    {
        // if we don't need to adjust the scale, do nothing
        if (MCore::Math::IsFloatEqual(scaleFactor, 1.0f))
        {
            return;
        }

        // scale the morph targets
        const uint32 numMorphTargets = mMorphTargets.GetLength();
        for (uint32 i = 0; i < numMorphTargets; ++i)
        {
            mMorphTargets[i]->Scale(scaleFactor);
        }
    }
} // namespace EMotionFX
