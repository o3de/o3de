/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Jobs/JobContext.h>

#include <AzCore/Jobs/Job.h>
#include <AzCore/Jobs/JobManager.h>

namespace AZ
{
    static EnvironmentVariable<JobContext*> s_globalJobContext;
    static const char* s_globalJobContextName = "GlobalJobContext";

    void JobContext::SetGlobalContext(JobContext* context)
    {
        if (!s_globalJobContext)
        {
            s_globalJobContext = Environment::CreateVariable<JobContext*>(s_globalJobContextName);
        }
        else if (context && *s_globalJobContext)
        {
            AZ_Error("JobContext", false, "JobContext::SetGlobalContext was called without first destroying the old context and setting it to nullptr");
        }

        s_globalJobContext.Set(context);
    }

    JobContext* JobContext::GetGlobalContext()
    {
        if (!s_globalJobContext)
        {
            s_globalJobContext = Environment::FindVariable<JobContext*>(s_globalJobContextName);
        }
        AZ_Assert(s_globalJobContext, "JobContext::GetGlobalContext called before JobContext::SetGlobalContext()");

        return *s_globalJobContext;
    }

    JobContext* JobContext::GetParentContext()
    {
        JobContext* globalContext = GetGlobalContext();
        AZ_Assert(globalContext, "Unable to get a parent context unless a global context has been assigned with JobContext::SetGlobalContext");
        Job* currentJob = globalContext->GetJobManager().GetCurrentJob();
        return currentJob ? currentJob->GetContext() : globalContext;
    }
}
