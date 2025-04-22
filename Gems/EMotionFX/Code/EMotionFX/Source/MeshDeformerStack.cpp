/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    AZ_CLASS_ALLOCATOR_IMPL(MeshDeformerStack, DeformerAllocator)

    // constructor
    MeshDeformerStack::MeshDeformerStack(Mesh* mesh)
        : MCore::RefCounted()
    {
        m_mesh = mesh;
    }


    // destructor
    MeshDeformerStack::~MeshDeformerStack()
    {
        for (MeshDeformer* deformer : m_deformers)
        {
            deformer->Destroy();
        }

        m_deformers.clear();

        // reset
        m_mesh = nullptr;
    }


    // create
    MeshDeformerStack* MeshDeformerStack::Create(Mesh* mesh)
    {
        return aznew MeshDeformerStack(mesh);
    }


    // returns the mesh
    Mesh* MeshDeformerStack::GetMesh() const
    {
        return m_mesh;
    }


    // update the mesh deformer stack
    void MeshDeformerStack::Update(ActorInstance* actorInstance, Node* node, float timeDelta, bool forceUpdateDisabledDeformers)
    {
        bool firstEnabled = true;

        // iterate through the deformers and update them
        for (MeshDeformer* deformer : m_deformers)
        {
            // if the deformer is enabled
            if (deformer->GetIsEnabled() || forceUpdateDisabledDeformers)
            {
                // if this is the first enabled deformer
                if (firstEnabled)
                {
                    firstEnabled = false;

                    // reset all output vertex data to the original vertex data
                    m_mesh->ResetToOriginalData();
                }

                // update the mesh deformer
                deformer->Update(actorInstance, node, timeDelta);
            }
        }
    }

    // update the mesh deformer stack for only the modifier type specified
    void MeshDeformerStack::UpdateByModifierType(ActorInstance* actorInstance, Node* node, float timeDelta, uint32 typeID, bool resetMesh, bool forceUpdateDisabledDeformers)
    {
        bool resetDone = false;
        for (MeshDeformer* deformer : m_deformers)
        {
            // if the deformer of the correct type and is enabled
            if (deformer->GetType() == typeID && (deformer->GetIsEnabled() || forceUpdateDisabledDeformers))
            {
                // if this is the first enabled deformer
                if (resetMesh && !resetDone)
                {
                    // reset all output vertex data to the original vertex data
                    m_mesh->ResetToOriginalData();
                    resetDone = true;
                }

                // update the mesh deformer
                deformer->Update(actorInstance, node, timeDelta);
            }
        }
    }


    // reinitialize mesh deformers
    void MeshDeformerStack::ReinitializeDeformers(Actor* actor, Node* node, size_t lodLevel)
    {
        // if we have deformers in the stack
        const size_t numDeformers = m_deformers.size();

        const uint16 highestJointIndex = m_mesh->GetHighestJointIndex();

        // iterate through the deformers and reinitialize them
        for (size_t i = 0; i < numDeformers; ++i)
        {
            m_deformers[i]->Reinitialize(actor, node, lodLevel, highestJointIndex);
        }
    }


    void MeshDeformerStack::AddDeformer(MeshDeformer* meshDeformer)
    {
        // add the object into the stack
        m_deformers.emplace_back(meshDeformer);
    }


    void MeshDeformerStack::InsertDeformer(size_t pos, MeshDeformer* meshDeformer)
    {
        // add the object into the stack
        m_deformers.emplace(AZStd::next(begin(m_deformers), pos), meshDeformer);
    }


    bool MeshDeformerStack::RemoveDeformer(MeshDeformer* meshDeformer)
    {
        // delete the object
        if (const auto it = AZStd::find(begin(m_deformers), end(m_deformers), meshDeformer); it != end(m_deformers))
        {
            m_deformers.erase(it);
            return true;
        }
        return false;
    }


    MeshDeformerStack* MeshDeformerStack::Clone(Mesh* mesh)
    {
        // create the clone passing the mesh pointer
        MeshDeformerStack* newStack = aznew MeshDeformerStack(mesh);

        // clone all deformers
        for (const MeshDeformer* deformer : m_deformers)
        {
            newStack->AddDeformer(deformer->Clone(mesh));
        }

        // return a pointer to the clone
        return newStack;
    }


    size_t MeshDeformerStack::GetNumDeformers() const
    {
        return m_deformers.size();
    }


    MeshDeformer* MeshDeformerStack::GetDeformer(size_t nr) const
    {
        MCORE_ASSERT(nr < m_deformers.size());
        return m_deformers[nr];
    }


    // remove all the deformers of a given type
    size_t MeshDeformerStack::RemoveAllDeformersByType(uint32 deformerTypeID)
    {
        size_t numRemoved = 0;
        for (size_t a = 0; a < m_deformers.size(); )
        {
            MeshDeformer* deformer = m_deformers[a];
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
        for (MeshDeformer* deformer : m_deformers)
        {
            // retrieve the current deformer
             // remove the deformer
            RemoveDeformer(deformer);
            deformer->Destroy();
        }
    }


    // enabled or disable all controllers of a given type
    size_t MeshDeformerStack::EnableAllDeformersByType(uint32 deformerTypeID, bool enabled)
    {
        size_t numChanged = 0;
        for (MeshDeformer* deformer : m_deformers)
        {
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
        return AZStd::any_of(begin(m_deformers), end(m_deformers), [deformerTypeID](const MeshDeformer* deformer)
        {
            return deformer->GetType() == deformerTypeID;
        });
    }


    // find a deformer by type ID
    MeshDeformer* MeshDeformerStack::FindDeformerByType(uint32 deformerTypeID, size_t occurrence) const
    {
        const auto foundDeformer = AZStd::find_if(begin(m_deformers), end(m_deformers), [deformerTypeID, iter = occurrence](const MeshDeformer* deformer) mutable
        {
            return deformer->GetType() == deformerTypeID && iter-- == 0;
        });
        return foundDeformer != end(m_deformers) ? *foundDeformer : nullptr;
    }
} // namespace EMotionFX
