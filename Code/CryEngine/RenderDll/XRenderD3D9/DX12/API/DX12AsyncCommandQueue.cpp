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
#include "DX12AsyncCommandQueue.hpp"
#include "DX12CommandList.hpp"
#include "DriverD3D.h"

#include <AzCore/Debug/EventTraceDrillerBus.h>

#define TRACK_RENDERTREAD_WAIT_TIME SScopedExecutionTimeTracker renderThreadWaitTimeTracker(gcpRendD3D->m_fTimeWaitForGPU[gcpRendD3D->m_RP.m_nProcessThreadID]);

struct SScopedExecutionTimeTracker
{
    SScopedExecutionTimeTracker(float& var)
        : trackerVariable(var)
        , startTime(iTimer ? iTimer->GetAsyncTime() : 0.0f)
    {}

    ~SScopedExecutionTimeTracker()
    {
        trackerVariable += iTimer ? iTimer->GetAsyncTime().GetDifferenceInSeconds(startTime) : 0.0f;
    }

    float& trackerVariable;
    CTimeValue startTime;
};

namespace DX12
{
    void AsyncCommandQueue::SExecuteCommandlist::Process(const STaskArgs& args)
    {
        AZ_TRACE_METHOD_NAME("CommandQueue ExecuteCommandLists");
        args.pCommandQueue->ExecuteCommandLists(1, &pCommandList);
    }

    void AsyncCommandQueue::SResetCommandlist::Process([[maybe_unused]] const STaskArgs& args)
    {
        AZ_TRACE_METHOD_NAME("CommandList Reset");
        pCommandList->Reset();
    }

    void AsyncCommandQueue::SSignalFence::Process(const STaskArgs& args)
    {
        AZ_TRACE_METHOD_NAME("Fence Signal");
        args.pCommandQueue->Signal(pFence, FenceValue);
    }

    void AsyncCommandQueue::SWaitForFence::Process(const STaskArgs& args)
    {
        AZ_TRACE_METHOD_NAME("Fence Wait");
        args.pCommandQueue->Wait(pFence, FenceValue);
    }

    void AsyncCommandQueue::SWaitForFences::Process(const STaskArgs& args)
    {
        AZ_TRACE_METHOD_NAME("Fence MultiWait");
        if (FenceValues[CMDQUEUE_COPY    ])
        {
            args.pCommandQueue->Wait(pFences[CMDQUEUE_COPY    ], FenceValues[CMDQUEUE_COPY    ]);
        }
        if (FenceValues[CMDQUEUE_GRAPHICS])
        {
            args.pCommandQueue->Wait(pFences[CMDQUEUE_GRAPHICS], FenceValues[CMDQUEUE_GRAPHICS]);
        }
    }

    AsyncCommandQueue::AsyncCommandQueue()
        : m_pCmdListPool(NULL)
        , m_QueuedFramesCounter(0)
        , m_bStopRequested(false)
    {
    }

    AsyncCommandQueue::~AsyncCommandQueue()
    {
        SignalStop();
        Flush();

        GetISystem()->GetIThreadManager()->JoinThread(this, eJM_Join);
    }

    void AsyncCommandQueue::Init(CommandListPool* pCommandListPool)
    {
        m_pCmdListPool = pCommandListPool;
        m_QueuedFramesCounter = 0;
        m_QueuedTasksCounter = 0;
        m_bStopRequested = false;

        GetISystem()->GetIThreadManager()->SpawnThread(this, "DX12 AsyncCommandQueue");
    }


    void AsyncCommandQueue::ExecuteCommandLists(UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists)
    {
        for (int i = 0; i < NumCommandLists; ++i)
        {
            SSubmissionTask task;
            ZeroStruct(task);

            task.type = eTT_ExecuteCommandList;
            task.Data.ExecuteCommandList.pCommandList = ppCommandLists[i];

            AddTask<SExecuteCommandlist>(task);
        }
    }

    void AsyncCommandQueue::ResetCommandList(CommandList* pCommandList)
    {
        SSubmissionTask task;
        ZeroStruct(task);

        task.type = eTT_ResetCommandList;
        task.Data.ResetCommandList.pCommandList = pCommandList;

        AddTask<SResetCommandlist>(task);
    }

    void AsyncCommandQueue::Signal(ID3D12Fence* pFence, const UINT64 FenceValue)
    {
        SSubmissionTask task;
        ZeroStruct(task);

        task.type = eTT_SignalFence;
        task.Data.SignalFence.pFence = pFence;
        task.Data.SignalFence.FenceValue = FenceValue;

        AddTask<SSignalFence>(task);
    }

    void AsyncCommandQueue::Wait(ID3D12Fence* pFence, const UINT64 FenceValue)
    {
        SSubmissionTask task;
        ZeroStruct(task);

        task.type = eTT_WaitForFence;
        task.Data.WaitForFence.pFence = pFence;
        task.Data.WaitForFence.FenceValue = FenceValue;

        AddTask<SWaitForFence>(task);
    }

    void AsyncCommandQueue::Wait(ID3D12Fence** pFences, const UINT64 (&FenceValues)[CMDQUEUE_NUM])
    {
        SSubmissionTask task;
        ZeroStruct(task);

        task.type = eTT_WaitForFences;
        task.Data.WaitForFences.pFences = pFences;
        task.Data.WaitForFences.FenceValues[CMDQUEUE_COPY    ] = FenceValues[CMDQUEUE_COPY    ];
        task.Data.WaitForFences.FenceValues[CMDQUEUE_GRAPHICS] = FenceValues[CMDQUEUE_GRAPHICS];

        AddTask<SWaitForFences>(task);
    }

    void AsyncCommandQueue::Flush(UINT64 lowerBoundFenceValue)
    {
        TRACK_RENDERTREAD_WAIT_TIME

        if (lowerBoundFenceValue != (~0ULL))
        {
            while (m_QueuedTasksCounter > 0)
            {
                if (lowerBoundFenceValue <= m_pCmdListPool->GetLastCompletedFenceValue())
                {
                    break;
                }

                Sleep(0);
            }
        }
        else
        {
            while (m_QueuedTasksCounter > 0)
            {
                Sleep(0);
            }
        }
    }

    void AsyncCommandQueue::ThreadEntry()
    {
        EBUS_EVENT(AZ::Debug::EventTraceDrillerSetupBus, SetThreadName, AZStd::this_thread::get_id(), "AsyncCommandQueue");

        while (!m_bStopRequested)
        {
            SSubmissionTask task;
            STaskArgs taskArgs = { m_pCmdListPool->GetD3D12CommandQueue(), &m_QueuedFramesCounter };

            if (m_TaskQueue.dequeue(task))
            {
                switch (task.type)
                {
                case eTT_ExecuteCommandList:
                    task.Process<SExecuteCommandlist>(taskArgs);
                    break;
                case eTT_ResetCommandList:
                    task.Process<SResetCommandlist>(taskArgs);
                    break;
                case eTT_SignalFence:
                    task.Process<SSignalFence>(taskArgs);
                    break;
                case eTT_WaitForFence:
                    task.Process<SWaitForFence>(taskArgs);
                    break;
                case eTT_WaitForFences:
                    task.Process<SWaitForFences>(taskArgs);
                    break;
                }

                CryInterlockedDecrement(&m_QueuedTasksCounter);
            }
            else
            {
                Sleep(0);
            }
        }
    }
}
