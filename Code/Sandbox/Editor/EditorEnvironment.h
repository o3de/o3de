/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <SandboxAPI.h>
#include <AzCore/Module/Environment.h>

SANDBOX_API void  SetEditorEnvironment(struct SSystemGlobalEnvironment* pEnv);

SANDBOX_API void AttachEditorAZEnvironment(AZ::EnvironmentInstance azEnv);

SANDBOX_API void DetachEditorAZEnvironment();
