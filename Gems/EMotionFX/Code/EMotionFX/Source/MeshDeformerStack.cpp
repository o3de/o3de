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
#include "EMotionFXConfig.h"
#include "MeshDeformerStack.h"
#include "Mesh.h"
#include "Actor.h"
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MeshDeformerStack, DeformerAllocator, 0)

    // constructor
    MeshDeformerStack::MeshDeformerStack(Mesh* mesh)
        : BaseObject()
    {
        mMesh = mesh;
        mDeformers.SetMemoryCategory(EMFX_MEMCATEGORY_GEOMETRY_DEFORMERS);
    }


    // destructor
    MeshDeformerStack::~MeshDeformerStack()
    {
        const uint32 numDeformers = mDeformers.GetLength();
        for (uint32 i = 0; i < numDeformers; ++i)
        {
            mDeformers[i]->Destroy();
        }

        mDeformers.Clear();

        // reset
        mMesh = nullptr;
    }


    // create
    MeshDeformerStack* MeshDeformerStack::Create(Mesh* mesh)
    {
        return aznew MeshDeformerStack(mesh);
    }


    // returns the mesh
    Mesh* MeshDeformerStack::GetMesh() const
    {
        return mMesh;
    }


    // update the mesh deformer stack
    void MeshDeformerStack::Update(ActorInstance* actorInstance, Node* node, float timeDelta, bool forceUpdateDisabledDeformers)
    {
        // if we have deformers in the stack
        const uint32 numDeformers = mDeformers.GetLength();
        if (numDeformers > 0)
        {
            bool firstEnabled = true;

            // iterate through the deformers and update them
            for (uint32 i = 0; i < numDeformers; ++i)
            {
                // if the deformer is enabled
                if (mDeformers[i]->GetIsEnabled() || forceUpdateDisabledDeformers)
                {
                    // if this is the first enabled deformer
                    if (firstEnabled)
                    {
                        firstEnabled = false;

                        // reset all output vertex data to the original vertex data
                        mMesh->ResetToOriginalData();
                    }

                    // update the mesh deformer
                    mDeformers[i]->Update(actorInstance, node, timeDelta);
                }
            }
        }
    }

    // update the mesh deformer stack for only the modifier type specified
    void MeshDeformerStack::UpdateByModifierType(ActorInstance* actorInstance, Node* node, float timeDelta, uint32 typeID, bool resetMesh, bool forceUpdateDisabledDeformers)
    {
        bool resetDone = false;
        // if we have deformers in the stack
        const uint32 numDeformers = mDeformers.GetLength();
        // iterate through the deformers and update them
        for (uint32 i = 0; i < numDeformers; ++i)
        {
            // if the deformer of the correct type and is enabled
            if (mDeformers[i]->GetType() == typeID && (mDeformers[i]->GetIsEnabled() || forceUpdateDisabledDeformers))
            {
                // if this is the first enabled deformer
                if (resetMesh && !resetDone)
                {
                    // reset all output vertex data to the original vertex data
                    mMesh->ResetToOriginalData();
                    resetDone = true;
                }

                // update the mesh deformer
                mDeformers[i]->Update(actorInstance, node, timeDelta);
            }
        }
    }


    // reinitialize mesh deformers
    void MeshDeformerStack::ReinitializeDeformers(Actor* actor, Node* node, uint32 lodLevel)
    {
        // if we have deformers in the stack
        const uint32 numDeformers = mDeformers.GetLength();

        // iterate through the deformers and reinitialize them
        for (uint32 i = 0; i < numDeformers; ++i)
        {
            mDeformers[i]->Reinitialize(actor, node, lodLevel);
        }
    }


    void MeshDeformerStack::AddDeformer(MeshDeformer* meshDeformer)
    {
        // add the object into the stack
        mDeformers.Add(meshDeformer);
    }


    void MeshDeformerStack::InsertDeformer(uint32 pos, MeshDeformer* meshDeformer)
    {
        // add the object into the stack
        mDeformers.Insert(pos, meshDeformer);
    }


    bool MeshDeformerStack::RemoveDeformer(MeshDeformer* meshDeformer)
    {
        // delete the object
        return mDeformers.RemoveByValue(meshDeformer);
    }


    MeshDeformerStack* MeshDeformerStack::Clone(Mesh* mesh)
    {
        // create the clone passing the mesh pointer
        MeshDeformerStack* newStack = aznew MeshDeformerStack(mesh);

        // clone all deformers
        const uint32 numDeformers = mDeformers.GetLength();
        for (uint32 i = 0; i < numDeformers; ++i)
        {
            newStack->AddDeformer(mDeformers[i]->Clone(mesh));
        }

        // return a pointer to the clone
        return newStack;
    }


    uint32 MeshDeformerStack::GetNumDeformers() const
    {
        return mDeformers.GetLength();
    }


    MeshDeformer* MeshDeformerStack::GetDeformer(uint32 nr) const
    {
        MCORE_ASSERT(nr < mDeformers.GetLength());
        return mDeformers[nr];
    }


    // remove all the deformers of a given type
    uint32 MeshDeformerStack::RemoveAllDeformersByType(uint32 deformerTypeID)
    {
        uint32 numRemoved = 0;
        for (uint32 a = 0; a < mDeformers.GetLength(); )
        {
            MeshDeformer* deformer = mDeformers[a];
            if (deformer->GetType() == deformerTypeID)
            {
                RemoveDeformer(deformer);
                deformer->Destroy();
                numRemoved++;
            }
            else
            {
                a++;
            }
        }

        return numRemoved;
    }


    // remove all the deformers
    void MeshDeformerStack::RemoveAllDeformers()
    {
        for (uint32 i = 0; i < mDeformers.GetLength(); ++i)
        {
            // retrieve the current deformer
            MeshDeformer* deformer = mDeformers[i];

            // remove the deformer
            RemoveDeformer(deformer);
            deformer->Destroy();
        }
    }


    // enabled or disable all controllers of a given type
    uint32 MeshDeformerStack::EnableAllDeformersByType(uint32 deformerTypeID, bool enabled)
    {
        uint32 numChanged = 0;
        const uint32 numDeformers = mDeformers.GetLength();
        for (uint32 a = 0; a < numDeformers; ++a)
        {
            MeshDeformer* deformer = mDeformers[a];
            if (deformer->GetType() == deformerTypeID)
            {
                deformer->SetIsEnabled(enabled);
                numChanged++;
            }
        }

        return numChanged;
    }


    // check if the stack contains a deformer of a specified type
    bool MeshDeformerStack::CheckIfHasDeformerOfType(uint32 deformerTypeID) const
    {
        const uint32 numDeformers = mDeformers.GetLength();
        for (uint32 a = 0; a < numDeformers; ++a)
        {
            if (mDeformers[a]->GetType() == deformerTypeID)
            {
                return true;
            }
        }

        return false;
    }


    // find a deformer by type ID
    MeshDeformer* MeshDeformerStack::FindDeformerByType(uint32 deformerTypeID, uint32 occurrence) const
    {
        uint32 count = 0;

        // for all deformers
        const uint32 numDeformers = mDeformers.GetLength();
        for (uint32 a = 0; a < numDeformers; ++a)
        {
            // if this is a deformer of the type we search for
            if (mDeformers[a]->GetType() == deformerTypeID)
            {
                // if its the one we want
                if (count == occurrence)
                {
                    return mDeformers[a];
                }
                else
                {
                    count++;
                }
            }
        }

        // none found
        return nullptr;
    }
} // namespace EMotionFX
