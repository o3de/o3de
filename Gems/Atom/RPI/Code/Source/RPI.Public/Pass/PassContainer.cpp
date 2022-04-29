/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Pass/RenderPassData.h>

#include <Atom/RPI.Public/Pass/PassContainer.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>

namespace AZ::RPI
{
    void PassContainer::RemovePasses()
    {
        m_state = PassContainerState::RemovingPasses;
        AZ_PROFILE_SCOPE(RPI, "PassContainer::RemovePasses");

        if (!m_removePassList.empty())
        {
            PassUtils::SortPassListDescending(m_removePassList);

            for (const Ptr<Pass>& pass : m_removePassList)
            {
                pass->RemoveFromParent();
            }

            m_removePassList.clear();
        }

        m_state = PassContainerState::Idle;
    }

    void PassContainer::BuildPasses()
    {
        m_state = PassContainerState::BuildingPasses;
        AZ_PROFILE_SCOPE(RPI, "PassContainer::BuildPasses");

        m_passesChangedThisFrame = m_passesChangedThisFrame || !m_buildPassList.empty();

        u32 buildCount = 0;

        // While loop is for the event in which passes being built add more pass to m_buildPassList
        while (!m_buildPassList.empty())
        {
            AZ_Assert(m_removePassList.empty(), "Passes shouldn't be queued removal during build attachment process");

            AZStd::vector< Ptr<Pass> > buildListCopy = m_buildPassList;
            m_buildPassList.clear();

            // Erase passes which were removed from pass tree already (which parent is empty)
            auto unused = AZStd::remove_if(buildListCopy.begin(), buildListCopy.end(),
                [](const RHI::Ptr<Pass>& currentPass)
                {
                    return !currentPass->m_flags.m_partOfHierarchy;
                });
            buildListCopy.erase(unused, buildListCopy.end());

            PassUtils::SortPassListAscending(buildListCopy);

            for (const Ptr<Pass>& pass : buildListCopy)
            {
                pass->Reset();
            }
            for (const Ptr<Pass>& pass : buildListCopy)
            {
                pass->Build(true);
                ++buildCount;
            }
        }

        if (buildCount > 0)
        {
            AZ_Assert(!m_initializePassList.empty(), "SHIT! SOMETHING WENT WRONG! WHY DIDN'T WE QUEUE FOR INITIALIZATION??");
        }

        if (m_passesChangedThisFrame)
        {
#if AZ_RPI_ENABLE_PASS_DEBUGGING
            if (!m_isHotReloading)
            {
                AZ_Printf("PassSystem", "\nFinished building passes:\n");
                DebugPrintPassHierarchy();
            }
#endif
        }

        m_state = PassContainerState::Idle;
    }

    void PassContainer::InitializePasses()
    {
        m_state = PassContainerState::InitializingPasses;
        AZ_PROFILE_SCOPE(RPI, "PassContainer::InitializePasses");

        m_passesChangedThisFrame = m_passesChangedThisFrame || !m_initializePassList.empty();

        while (!m_initializePassList.empty())
        {
            AZStd::vector< Ptr<Pass> > initListCopy = m_initializePassList;
            m_initializePassList.clear();

            // Erase passes which were removed from pass tree already (which parent is empty)
            auto unused = AZStd::remove_if(initListCopy.begin(), initListCopy.end(),
                [](const RHI::Ptr<Pass>& currentPass)
                {
                    return !currentPass->m_flags.m_partOfHierarchy;
                });
            initListCopy.erase(unused, initListCopy.end());

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

        m_state = PassContainerState::Idle;
    }

    void PassContainer::Validate()
    {
        m_state = PassContainerState::ValidatingPasses;

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

        m_state = PassContainerState::Idle;
    }

    void PassContainer::ProcessQueuedChanges(bool& outChanged)
    {
        AZ_PROFILE_SCOPE(RPI, "PassContainer::ProcessQueuedChanges");
        RemovePasses();
        BuildPasses();
        InitializePasses();
        Validate();

        outChanged = outChanged || m_passesChangedThisFrame;
        m_passesChangedThisFrame = false;
    }

}
