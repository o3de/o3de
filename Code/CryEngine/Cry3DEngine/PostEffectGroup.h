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
#pragma once

#include "IPostEffectGroup.h"
#include "ISplines.h"
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

class PostEffectGroup : public IPostEffectGroup
{
public:
    // PostEffectGroups are assets, but have no handler yet.  For now, we will use this UUID to refer to them.
    AZ_TYPE_INFO(PostEffectGroup, "{BDDCFCE8-6E4E-4816-AE1C-ED98B02DA75D}"); 

    // Priority to assign to any group. Note that a higher value relates to a higher priority.
    enum GroupPriority
    {
        kPriorityDefault = 0, // Used for the default effect settings.
        kPriorityBase    = 1, // Base group edited by user/code. Should always be higher priority than kDefalt.
        // Add any future priorities here.
    };

    PostEffectGroup(class PostEffectGroupManager* manager, const char* name, GroupPriority priority, bool hold, float fadeDistance);

    const char* GetName() const override { return m_name.c_str(); }
    void SetEnable(bool enable) override;
    bool GetEnable() const override { return m_enable; }
    unsigned int GetPriority() const override { return m_priority; }
    bool GetHold() const override { return m_hold; }
    float GetFadeDistance() const override { return m_fadeDistance; }
    void SetParam(const char* name, const PostEffectGroupParam& value) override;
    PostEffectGroupParam* GetParam(const char* name) override { return &m_params[name]; }
    void ClearParams() override;
    void ApplyAtPosition(const Vec3& position) override;
    ISplineInterpolator* GetBlendIn() { return &m_blendIn; }
    ISplineInterpolator* GetBlendOut() { return &m_blendOut; }
    uint32 GetLastUpdateFrame() const { return m_lastUpdateFrame; }
    void BlendWith(AZStd::unordered_map<AZStd::string, PostEffectGroupParam>& paramMap);
    void ResetStrength() { m_strength = 0.f; }
    AZ::Data::AssetId GetAssetId() const { return m_id; }
    //  Functions for changing an effect that has already been loaded. Not in the interface because they are intended to be used internally during hot reloads in the editor.
    void StopEffect();
    void ClearSplines();
    void SetPriority(GroupPriority priority) { m_priority = priority; }
    void SetHold(bool hold) { m_hold = hold; }
    void SetFadeDistance(float fadeDistance) { m_fadeDistance = fadeDistance; }

private:
    class BlendSpline : public spline::CBaseSplineInterpolator < float, spline::BezierSpline<float> >
    {
    public:
        float GetKeyRangeEnd()
        {
            return empty() ? 0.f : GetKeyTime(num_keys() - 1);
        }

        void Clear()
        {
            RemoveKeysInRange(0.0f, GetKeyRangeEnd());
        }

        void SerializeSpline([[maybe_unused]] XmlNodeRef &node, [[maybe_unused]] bool bLoading){}
    };

    class PostEffectGroupManager* m_manager;
    AZStd::string m_name;
    bool m_enable;
    GroupPriority m_priority;
    bool m_hold;
    float m_fadeDistance;
    AZStd::unordered_map<AZStd::string, PostEffectGroupParam> m_params;
    BlendSpline m_blendIn;
    BlendSpline m_blendOut;
    uint32 m_lastUpdateFrame;
    float m_enableDuration;
    float m_disableDuration;
    float m_strength;
    AZ::Data::AssetId m_id;
};

class PostEffectGroupManager
    : public IPostEffectGroupManager
    , public ISyncMainWithRenderListener
    , private AzFramework::AssetCatalogEventBus::Handler
{
public:
    PostEffectGroupManager();
    ~PostEffectGroupManager() override;

    IPostEffectGroup* GetGroup(const char* name) override;
    IPostEffectGroup* GetGroup(const unsigned int index) override;
    const unsigned int GetGroupCount() override;
    void SyncMainWithRender() override;
    void Sort();

    // Returns a list of PostEFfectGroups that had their enabled state changed this frame
    const PostEffectGroupList& GetGroupsToggledThisFrame() override;
    void SetGroupToggledThisFrame( IPostEffectGroup* group );
private:
    //  Sending in a PostEffectGroup* to LoadGroup will cause a load in place.
    //  Sending in a nullptr to LoadGroup will construct a new PostEffectGroup.
    IPostEffectGroup* LoadGroup(const char* name, PostEffectGroup* groupToLoadInto = nullptr);
    // =============== AssetCatalogEventBus Functions ==============
    void OnCatalogAssetChanged(const AZ::Data::AssetId& assetId) override;

    std::vector<std::unique_ptr<PostEffectGroup>> m_groups;
    AZStd::unordered_map<AZStd::string, PostEffectGroupParam> m_paramCache;

    // A list of PostEffectGroups that had their Enabled state changed this frame
    // Double buffered for render thread/main thread.
    PostEffectGroupList m_groupsToggledThisFrame[2];

    // The renderer version of the fill/process thread IDs is not available to us here.
    unsigned int m_fillThreadIndex = 0; 
};
