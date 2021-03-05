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

// Description : Interface for LocalMemoryUsage


#ifndef CRYINCLUDE_CRYCOMMON_ILOCALMEMORYUSAGE_H
#define CRYINCLUDE_CRYCOMMON_ILOCALMEMORYUSAGE_H
#pragma once


class CCamera;
struct IRenderer;

struct ILocalMemoryUsage
{
    virtual ~ILocalMemoryUsage(){}
    virtual void OnRender(IRenderer* pRenderer, const CCamera* camera) = 0;
    virtual void OnUpdate() = 0;
    virtual void DeleteGlobalData() = 0;
};

#endif // CRYINCLUDE_CRYCOMMON_ILOCALMEMORYUSAGE_H
