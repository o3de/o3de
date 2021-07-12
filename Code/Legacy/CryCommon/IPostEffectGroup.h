/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/variant.h>

typedef AZStd::variant<float, Vec4, AZStd::string> PostEffectGroupParam;

// A prioritized group of postprocessing effect parameters.
// These are defined in XML files and can be enabled or disabled using flow graph or Lua scripts.
// Effect groups can also optionally specify blend curves to smoothly transition between effects, whether to stay enabled until explicitly disabled,
// and whether to make effect strength based on distance from the camera.
class IPostEffectGroup
{
public:
    virtual ~IPostEffectGroup(){}

    virtual const char* GetName() const = 0;
    virtual void SetEnable(bool enable) = 0;
    virtual bool GetEnable() const = 0;
    virtual unsigned int GetPriority() const = 0;
    virtual bool GetHold() const = 0;
    virtual float GetFadeDistance() const = 0;
    virtual void SetParam(const char* name, const PostEffectGroupParam& value) = 0;
    virtual PostEffectGroupParam* GetParam(const char* name) = 0;
    virtual void ClearParams() = 0;
    // Increases the strength of the effects based on distance from the camera each time it's called. The effect strength is cleared each frame.
    // Only applies to effect groups with the fadeDistance attribute set.
    virtual void ApplyAtPosition(const Vec3& position) = 0;
};

typedef AZStd::list<IPostEffectGroup*> PostEffectGroupList;

class IPostEffectGroupManager
{
public:
    virtual ~IPostEffectGroupManager(){}

    virtual IPostEffectGroup* GetGroup(const char* name) = 0;
    virtual IPostEffectGroup* GetGroup(const unsigned int index) = 0;
    virtual const unsigned int GetGroupCount() = 0;
    // Returns a list of IPostEffectGroups who had their Enabled state toggled this frame
    virtual const PostEffectGroupList& GetGroupsToggledThisFrame() = 0;
};
