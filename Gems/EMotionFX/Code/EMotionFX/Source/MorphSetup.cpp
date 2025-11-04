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
    AZ_CLASS_ALLOCATOR_IMPL(MorphSetup, DeformerAllocator)


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
        m_morphTargets.emplace_back(morphTarget);
    }


    // remove a morph target
    void MorphSetup::RemoveMorphTarget(size_t nr, bool delFromMem)
    {
        if (delFromMem)
        {
            m_morphTargets[nr]->Destroy();
        }

        m_morphTargets.erase(AZStd::next(begin(m_morphTargets), nr));
    }


    // remove a morph target
    void MorphSetup::RemoveMorphTarget(MorphTarget* morphTarget, bool delFromMem)
    {
        const auto* foundMorphTarget = AZStd::find(begin(m_morphTargets), end(m_morphTargets), morphTarget);
        if (foundMorphTarget != end(m_morphTargets))
        {
            m_morphTargets.erase(foundMorphTarget);
        }

        if (delFromMem)
        {
            morphTarget->Destroy();
        }
    }


    // remove all morph targets
    void MorphSetup::RemoveAllMorphTargets()
    {
        for (MorphTarget*& morphTarget : m_morphTargets)
        {
            morphTarget->Destroy();
        }

        m_morphTargets.clear();
    }


    // get a morph target by ID
    MorphTarget* MorphSetup::FindMorphTargetByID(uint32 id) const
    {
        const auto foundMorphTarget = AZStd::find_if(begin(m_morphTargets), end(m_morphTargets), [id](const MorphTarget* morphTarget)
        {
            return morphTarget->GetID() == id;
        });
        return foundMorphTarget != end(m_morphTargets) ? *foundMorphTarget : nullptr;
    }


    // get a morph target number by ID
    size_t MorphSetup::FindMorphTargetNumberByID(uint32 id) const
    {
        const auto foundMorphTarget = AZStd::find_if(begin(m_morphTargets), end(m_morphTargets), [id](const MorphTarget* morphTarget)
        {
            return morphTarget->GetID() == id;
        });
        return foundMorphTarget != end(m_morphTargets) ? AZStd::distance(begin(m_morphTargets), foundMorphTarget) : InvalidIndex;
    }


    size_t MorphSetup::FindMorphTargetIndexByName(const char* name) const
    {
        const auto foundMorphTarget = AZStd::find_if(begin(m_morphTargets), end(m_morphTargets), [name](const MorphTarget* morphTarget)
        {
            return morphTarget->GetNameString() == name;
        });
        return foundMorphTarget != end(m_morphTargets) ? AZStd::distance(begin(m_morphTargets), foundMorphTarget) : InvalidIndex;
    }


    size_t MorphSetup::FindMorphTargetIndexByNameNoCase(const char* name) const
    {
        const auto foundMorphTarget = AZStd::find_if(begin(m_morphTargets), end(m_morphTargets), [name](const MorphTarget* morphTarget)
        {
            return AzFramework::StringFunc::Equal(morphTarget->GetNameString().c_str(), name, false /* no case */);
        });
        return foundMorphTarget != end(m_morphTargets) ? AZStd::distance(begin(m_morphTargets), foundMorphTarget) : InvalidIndex;
    }


    // find a morph target by name (case sensitive)
    MorphTarget* MorphSetup::FindMorphTargetByName(const char* name) const
    {
        const auto foundMorphTarget = AZStd::find_if(begin(m_morphTargets), end(m_morphTargets), [name](const MorphTarget* morphTarget)
        {
            return morphTarget->GetNameString() == name;
        });
        return foundMorphTarget != end(m_morphTargets) ?  *foundMorphTarget : nullptr;
    }


    // find a morph target by name (not case sensitive)
    MorphTarget* MorphSetup::FindMorphTargetByNameNoCase(const char* name) const
    {
        const auto foundMorphTarget = AZStd::find_if(begin(m_morphTargets), end(m_morphTargets), [name](const MorphTarget* morphTarget)
        {
            return AzFramework::StringFunc::Equal(morphTarget->GetNameString().c_str(), name, false /* no case */);
        });
        return foundMorphTarget != end(m_morphTargets) ? *foundMorphTarget : nullptr;
    }


    // clone the morph setup
    MorphSetup* MorphSetup::Clone() const
    {
        // create the clone
        MorphSetup* clone = MorphSetup::Create();

        // clone all morph targets
        for (const MorphTarget* morphTarget : m_morphTargets)
        {
            clone->AddMorphTarget(morphTarget->Clone());
        }

        // return the cloned morph setup
        return clone;
    }


    void MorphSetup::ReserveMorphTargets(size_t numMorphTargets)
    {
        m_morphTargets.reserve(numMorphTargets);
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
        for (MorphTarget* morphTarget : m_morphTargets)
        {
            morphTarget->Scale(scaleFactor);
        }
    }
} // namespace EMotionFX
