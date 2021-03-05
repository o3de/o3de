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
// Original file Copyright Crytek GMBH or its affiliates, used under license.
#include "RenderDll_precompiled.h"
#include "DX12CommandListFence.hpp"
#include "DriverD3D.h"

#define TRACK_RENDERTREAD_WAIT_TIME SScopedExecutionTimeTracker renderThreadWaitTimeTracker(gcpRendD3D->m_fTimeWaitForGPU[gcpRendD3D->m_RP.m_nProcessThreadID]);

struct SScopedExecutionTimeTracker
{
    SScopedExecutionTimeTracker(float& var)
        : trackerVariable(var)
        , startTime(iTimer->GetAsyncTime())
    {}

    ~SScopedExecutionTimeTracker()
    {
        trackerVariable += iTimer->GetAsyncTime().GetDifferenceInSeconds(startTime);
    }

    float& trackerVariable;
    CTimeValue startTime;
};

namespace DX12
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    CommandListFence::CommandListFence(Device* device)
        : m_pDevice(device)
        , m_LastCompletedValue(0)
        , m_CurrentValue(0)
    {
    }

    //---------------------------------------------------------------------------------------------------------------------
    CommandListFence::~CommandListFence()
    {
        m_pFence->Release();
        CloseHandle(m_FenceEvent);
    }

    //---------------------------------------------------------------------------------------------------------------------
    bool CommandListFence::Init()
    {
        ID3D12Fence* fence = NULL;
        if (S_OK != m_pDevice->GetD3D12Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(&fence)))
        {
            DX12_ERROR("Could not create fence object!");
            return false;
        }

        m_pFence = fence;
        m_pFence->Signal(m_LastCompletedValue);
        m_FenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);

        return true;
    }

    void CommandListFence::WaitForFence(AZ::u64 fenceValue)
    {
        if (!IsCompleted(fenceValue))
        {
            DX12_LOG("Waiting CPU for fence %d -> %d", m_pFence->GetCompletedValue(), fenceValue);
            {
                TRACK_RENDERTREAD_WAIT_TIME

                m_pFence->SetEventOnCompletion(fenceValue, m_FenceEvent);

                WaitForSingleObject(m_FenceEvent, INFINITE);
            }
            DX12_LOG("Completed fence value: %d", m_pFence->GetCompletedValue());

            AdvanceCompletion();
        }
    }

    //---------------------------------------------------------------------------------------------------------------------
    CommandListFenceSet::CommandListFenceSet(Device* device)
        : m_pDevice(device)
    {
        m_LastCompletedValues[CMDQUEUE_COPY    ] =
            m_LastCompletedValues[CMDQUEUE_GRAPHICS] = 0;
        m_SubmittedValues[CMDQUEUE_COPY] =
            m_SubmittedValues[CMDQUEUE_GRAPHICS] = 0;
        m_CurrentValues[CMDQUEUE_COPY    ] =
            m_CurrentValues[CMDQUEUE_GRAPHICS] = 0;
    }

    //---------------------------------------------------------------------------------------------------------------------
    CommandListFenceSet::~CommandListFenceSet()
    {
        for (int i = 0; i < CMDQUEUE_NUM; ++i)
        {
            m_pFences[i]->Release();

            CloseHandle(m_FenceEvents[i]);
        }
    }

    //---------------------------------------------------------------------------------------------------------------------
    bool CommandListFenceSet::Init()
    {
        for (int i = 0; i < CMDQUEUE_NUM; ++i)
        {
            ID3D12Fence* fence = NULL;
            if (S_OK != m_pDevice->GetD3D12Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(&fence)))
            {
                DX12_ERROR("Could not create fence object!");
                return false;
            }

            m_pFences[i] = fence;
            m_pFences[i]->Signal(m_LastCompletedValues[i]);
            m_FenceEvents[i] = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
        }

        return true;
    }

    void CommandListFenceSet::WaitForFence(const AZ::u64 fenceValue, const int id) const
    {
        DX12_LOG("Waiting CPU for fence %d -> %d", m_pFences[id]->GetCompletedValue(), fenceValue);

        {
            TRACK_RENDERTREAD_WAIT_TIME

                m_pFences[id]->SetEventOnCompletion(fenceValue, m_FenceEvents[id]);

            WaitForSingleObject(m_FenceEvents[id], INFINITE);
        }

        DX12_LOG("Completed fence value: %d", m_pFences[id]->GetCompletedValue());

        AdvanceCompletion(id);
    }

    void CommandListFenceSet::WaitForFence(const AZ::u64 (&fenceValues)[CMDQUEUE_NUM]) const
    {
        // TODO: the pool which waits for the fence can be omitted (in-order-execution)
        DX12_LOG("Waiting for GPU fences [%d,%d] -> [%d,%d]",
            m_pFences[CMDQUEUE_COPY    ]->GetCompletedValue(),
            m_pFences[CMDQUEUE_GRAPHICS]->GetCompletedValue(),
            fenceValues[CMDQUEUE_COPY    ],
            fenceValues[CMDQUEUE_GRAPHICS]);

        {
            TRACK_RENDERTREAD_WAIT_TIME

            // NOTE: event does ONLY trigger when the value has been set (it'll lock when trying with 0)
            int numObjects = 0;
            for (int id = 0; id < CMDQUEUE_NUM; ++id)
            {
                if (fenceValues[id] && (m_LastCompletedValues[id] < fenceValues[id]))
                {
                    m_pFences[id]->SetEventOnCompletion(fenceValues[id], m_FenceEvents[numObjects++]);
                }
            }

            WaitForMultipleObjects(numObjects, m_FenceEvents, true, INFINITE);
        }

        DX12_LOG("Completed GPU fences [%d,%d]",
            m_pFences[CMDQUEUE_COPY    ]->GetCompletedValue(),
            m_pFences[CMDQUEUE_GRAPHICS]->GetCompletedValue());

        AdvanceCompletion();
    }
}
