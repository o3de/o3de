/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "MorphSetupInstance.h"
#include "MorphSetup.h"
#include <EMotionFX/Source/Allocators.h>
#include <MCore/Source/RefCounted.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MorphSetupInstance, DeformerAllocator)


    // default constructor
    MorphSetupInstance::MorphSetupInstance()
        : MCore::RefCounted()
    {
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
        const size_t numMorphTargets = morphSetup->GetNumMorphTargets();
        m_morphTargets.resize(numMorphTargets);

        // update the ID values
        for (uint32 i = 0; i < numMorphTargets; ++i)
        {
            m_morphTargets[i].SetID(morphSetup->GetMorphTarget(i)->GetID());
        }
    }


    // try to locate the morph target by ID
    size_t MorphSetupInstance::FindMorphTargetIndexByID(uint32 id) const
    {
        // try to locate the morph target with the given ID
        const auto foundElement = AZStd::find_if(m_morphTargets.begin(), m_morphTargets.end(), [id](const MorphTarget& morphTarget)
        {
            return morphTarget.GetID() == id;
        });
        return foundElement != m_morphTargets.end() ? AZStd::distance(m_morphTargets.begin(), foundElement) : InvalidIndex;
    }


    MorphSetupInstance::MorphTarget* MorphSetupInstance::FindMorphTargetByID(uint32 id)
    {
        const size_t index = FindMorphTargetIndexByID(id);
        if (index != InvalidIndex)
        {
            return &m_morphTargets[index];
        }
        else
        {
            return nullptr;
        }
    }
} // namespace EMotionFX
