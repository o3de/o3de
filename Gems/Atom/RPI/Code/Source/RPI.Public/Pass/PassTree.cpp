/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Pass/PassTree.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>

namespace AZ::RPI
{
    void PassTree::EraseFromLists(AZStd::function<bool(const RHI::Ptr<Pass>&)> predicate)
    {
        AZStd::erase_if(m_removePassList, predicate);
        AZStd::erase_if(m_buildPassList, predicate);
        AZStd::erase_if(m_initializePassList, predicate);
    }

    void PassTree::ClearQueues()
    {
        m_removePassList.clear();
        m_buildPassList.clear();
        m_initializePassList.clear();
    }

    void PassTree::RemovePasses()
    {
        AZ_PROFILE_SCOPE(RPI, "PassTree::RemovePasses");
        m_state = PassTreeState::RemovingPasses;

        if (!m_removePassList.empty())
        {
            PassUtils::SortPassListDescending(m_removePassList);

            for (const Ptr<Pass>& pass : m_removePassList)
            {
                pass->RemoveFromParent();
            }

            m_removePassList.clear();
        }
        m_state = PassTreeState::Idle;
    }

    void PassTree::BuildPasses()
    {
        AZ_PROFILE_SCOPE(RPI, "PassTree::BuildPasses");
        m_state = PassTreeState::BuildingPasses;

        m_passesChangedThisFrame = m_passesChangedThisFrame || !m_buildPassList.empty();

        // While loop is for the event in which passes being built add more pass to m_buildPassList
        while (!m_buildPassList.empty())
        {
            AZ_Assert(m_removePassList.empty(), "Passes shouldn't be queued removal during build attachment process");

            // Copy the list and clear it, so that further passes can be queue during this phase
            AZStd::vector< Ptr<Pass> > buildListCopy = m_buildPassList;
            m_buildPassList.clear();

            // Erase passes which were removed from pass tree already (which parent is empty)
            AZStd::erase_if(buildListCopy, [](const RHI::Ptr<Pass>& currentPass) {
                    return !currentPass->m_flags.m_partOfHierarchy;
                });

            PassUtils::SortPassListAscending(buildListCopy);

            for (const Ptr<Pass>& pass : buildListCopy)
            {
                pass->Reset();
            }
            for (const Ptr<Pass>& pass : buildListCopy)
            {
                pass->Build(true);
            }
        }

        if (m_passesChangedThisFrame)
        {
#if AZ_RPI_ENABLE_PASS_DEBUGGING
            AZ_Printf("PassTree", "\nFinished building passes:\n");
            m_rootPass->DebugPrint();
#endif
        }
        m_state = PassTreeState::Idle;
    }

    void PassTree::InitializePasses()
    {
        AZ_PROFILE_SCOPE(RPI, "PassTree::InitializePasses");
        m_state = PassTreeState::InitializingPasses;

        m_passesChangedThisFrame = m_passesChangedThisFrame || !m_initializePassList.empty();

        while (!m_initializePassList.empty())
        {
            // Copy the list and clear it, so that further passes can be queue during this phase
            AZStd::vector< Ptr<Pass> > initListCopy = m_initializePassList;
            m_initializePassList.clear();

            // Erase passes which were removed from pass tree already (which parent is empty)
            AZStd::erase_if(initListCopy, [](const RHI::Ptr<Pass>& currentPass) {
                    return !currentPass->m_flags.m_partOfHierarchy;
                });

            PassUtils::SortPassListAscending(initListCopy);

            for (const Ptr<Pass>& pass : initListCopy)
            {
                pass->Initialize();
            }
        }

        if (m_passesChangedThisFrame)
        {
            // Signal all passes that we have finished initialization
            m_rootPass->OnInitializationFinished();
        }
        m_state = PassTreeState::Idle;
    }

    void PassTree::Validate()
    {
        m_state = PassTreeState::ValidatingPasses;

        if (PassValidation::IsEnabled())
        {
            if (!m_passesChangedThisFrame)
            {
                return;
            }

            AZ_PROFILE_SCOPE(RPI, "PassSystem: Validate");

            PassValidationResults validationResults;
            m_rootPass->Validate(validationResults);
            validationResults.PrintValidationIfError();
        }
        m_state = PassTreeState::Idle;
    }

    bool PassTree::ProcessQueuedChanges()
    {
        RemovePasses();
        BuildPasses();
        InitializePasses();
        Validate();

        bool changedThisFrame = m_passesChangedThisFrame;
        m_passesChangedThisFrame = false;
        return changedThisFrame;
    }

}
