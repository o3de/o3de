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

// Description : IOverloadSceneManager interface declaration.


#ifndef CRYINCLUDE_CRYCOMMON_IOVERLOADSCENEMANAGER_H
#define CRYINCLUDE_CRYCOMMON_IOVERLOADSCENEMANAGER_H
#pragma once

//==================================================================================================
// Name: COverloadSceneManager
// Desc: Manages overload values (eg CPU,GPU etc)
//           1.0="everything is ok"  0.0="very bad frame rate"
//           various systems can use this information and control what is currently in the scene
// Author: James Chilvers
//==================================================================================================
struct IOverloadSceneManager
{
public:
    // <interfuscator:shuffle>
    virtual ~IOverloadSceneManager() {}

    virtual void Reset() = 0;
    virtual void Update() = 0;

    // Override auto-calculated scale to reach targetfps.
    // frameScale is clamped to internal min/max values,
    // dt is the length of time in seconds to transition
    virtual void OverrideScale(float frameScale, float dt) = 0;

    // Go back to auto-calculated scale from an overridden scale
    virtual void ResetScale(float dt) = 0;
    // </interfuscator:shuffle>
};//------------------------------------------------------------------------------------------------

#endif // CRYINCLUDE_CRYCOMMON_IOVERLOADSCENEMANAGER_H
