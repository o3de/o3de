/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "MorphSetup.h"
#include "MorphTarget.h"
#include <MCore/Source/StringConversions.h>
#include <MCore/Source/FastMath.h>
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MorphSetup, DeformerAllocator, 0)


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
        mMorphTargets.emplace_back(morphTarget);
    }


    // remove a morph target
    void MorphSetup::RemoveMorphTarget(size_t nr, bool delFromMem)
    {
        if (delFromMem)
        {
            mMorphTargets[nr]->Destroy();
        }

        mMorphTargets.erase(AZStd::next(begin(mMorphTargets), nr));
    }


    // remove a morph target
    void MorphSetup::RemoveMorphTarget(MorphTarget* morphTarget, bool delFromMem)
    {
        const auto* foundMorphTarget = AZStd::find(begin(mMorphTargets), end(mMorphTargets), morphTarget);
        if (foundMorphTarget != end(mMorphTargets))
        {
            mMorphTargets.erase(foundMorphTarget);
        }

        if (delFromMem)
        {
            morphTarget->Destroy();
        }
    }


    // remove all morph targets
    void MorphSetup::RemoveAllMorphTargets()
    {
        for (MorphTarget*& mMorphTarget : mMorphTargets)
        {
            mMorphTarget->Destroy();
        }

        mMorphTargets.clear();
    }


    // get a morph target by ID
    MorphTarget* MorphSetup::FindMorphTargetByID(uint32 id) const
    {
        const auto foundMorphTarget = AZStd::find_if(begin(mMorphTargets), end(mMorphTargets), [id](const MorphTarget* morphTarget)
        {
            return morphTarget->GetID() == id;
        });
        return foundMorphTarget != end(mMorphTargets) ? *foundMorphTarget : nullptr;
    }


    // get a morph target number by ID
    size_t MorphSetup::FindMorphTargetNumberByID(uint32 id) const
    {
        const auto foundMorphTarget = AZStd::find_if(begin(mMorphTargets), end(mMorphTargets), [id](const MorphTarget* morphTarget)
        {
            return morphTarget->GetID() == id;
        });
        return foundMorphTarget != end(mMorphTargets) ? AZStd::distance(begin(mMorphTargets), foundMorphTarget) : InvalidIndex;
    }


    size_t MorphSetup::FindMorphTargetIndexByName(const char* name) const
    {
        const auto foundMorphTarget = AZStd::find_if(begin(mMorphTargets), end(mMorphTargets), [name](const MorphTarget* morphTarget)
        {
            return morphTarget->GetNameString() == name;
        });
        return foundMorphTarget != end(mMorphTargets) ? AZStd::distance(begin(mMorphTargets), foundMorphTarget) : InvalidIndex;
    }


    size_t MorphSetup::FindMorphTargetIndexByNameNoCase(const char* name) const
    {
        const auto foundMorphTarget = AZStd::find_if(begin(mMorphTargets), end(mMorphTargets), [name](const MorphTarget* morphTarget)
        {
            return AzFramework::StringFunc::Equal(morphTarget->GetNameString().c_str(), name, false /* no case */);
        });
        return foundMorphTarget != end(mMorphTargets) ? AZStd::distance(begin(mMorphTargets), foundMorphTarget) : InvalidIndex;
    }


    // find a morph target by name (case sensitive)
    MorphTarget* MorphSetup::FindMorphTargetByName(const char* name) const
    {
        const auto foundMorphTarget = AZStd::find_if(begin(mMorphTargets), end(mMorphTargets), [name](const MorphTarget* morphTarget)
        {
            return morphTarget->GetNameString() == name;
        });
        return foundMorphTarget != end(mMorphTargets) ?  *foundMorphTarget : nullptr;
    }


    // find a morph target by name (not case sensitive)
    MorphTarget* MorphSetup::FindMorphTargetByNameNoCase(const char* name) const
    {
        const auto foundMorphTarget = AZStd::find_if(begin(mMorphTargets), end(mMorphTargets), [name](const MorphTarget* morphTarget)
        {
            return AzFramework::StringFunc::Equal(morphTarget->GetNameString().c_str(), name, false /* no case */);
        });
        return foundMorphTarget != end(mMorphTargets) ? *foundMorphTarget : nullptr;
    }


    // clone the morph setup
    MorphSetup* MorphSetup::Clone() const
    {
        // create the clone
        MorphSetup* clone = MorphSetup::Create();

        // clone all morph targets
        for (const MorphTarget* morphTarget : mMorphTargets)
        {
            clone->AddMorphTarget(morphTarget->Clone());
        }

        // return the cloned morph setup
        return clone;
    }


    void MorphSetup::ReserveMorphTargets(size_t numMorphTargets)
    {
        mMorphTargets.reserve(numMorphTargets);
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
        for (MorphTarget* mMorphTarget : mMorphTargets)
        {
            mMorphTarget->Scale(scaleFactor);
        }
    }
} // namespace EMotionFX
