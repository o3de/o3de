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
#include "Cry3DEngine_precompiled.h"
#include "PostEffectGroup.h"
#include <AzCore/Asset/AssetManagerBus.h>

class BlendVisitor
{
private:
    bool m_enable;
    float m_blendAmount;

public:
    void Init(bool enable, float blendAmount)
    {
        m_enable = enable;
        m_blendAmount = blendAmount;
    }

    PostEffectGroupParam operator()(float base, float blend)
    {
        return base * (1.f - m_blendAmount) + blend * m_blendAmount;
    }

    PostEffectGroupParam operator()(const Vec4& base, const Vec4& blend)
    {
        return base * (1.f - m_blendAmount) + blend * m_blendAmount;
    }

    PostEffectGroupParam operator()(const AZStd::string& base, const AZStd::string& blend)
    {
        return m_enable ? blend : base;
    }

    template<typename T, typename U>
    PostEffectGroupParam operator()(const T&, const U&)
    {
        CRY_ASSERT(false);
        return 0.f;
    }
};

class SyncVisitor
{
private:
    const AZStd::string* m_name;

public:
    void SetName(const AZStd::string& name)
    {
        m_name = &name;
    }

    void operator()(float param)
    {
        if (gEnv && gEnv->p3DEngine)
        {
            gEnv->p3DEngine->SetPostEffectParam(m_name->c_str(), param);
        }
    }

    void operator()(const Vec4& param)
    {
        if (gEnv && gEnv->p3DEngine)
        {
            gEnv->p3DEngine->SetPostEffectParamVec4(m_name->c_str(), param);
        }
    }

    void operator()(const AZStd::string& param)
    {
        if (gEnv && gEnv->p3DEngine)
        {
            gEnv->p3DEngine->SetPostEffectParamString(m_name->c_str(), param.c_str());
        }
    }
};

PostEffectGroup::PostEffectGroup(class PostEffectGroupManager* manager, const char* name, GroupPriority priority, bool hold, float fadeDistance)
    : m_manager(manager)
    , m_name(name)
    , m_enable(false)
    , m_priority(priority)
    , m_hold(hold)
    , m_fadeDistance(fadeDistance)
    , m_lastUpdateFrame(gEnv->nMainFrameID)
    , m_enableDuration(0.f)
    , m_disableDuration(1000.f)
    , m_strength(0.f)
{
    EBUS_EVENT_RESULT(m_id, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, m_name.c_str(), AZ::AzTypeInfo<PostEffectGroup>::Uuid(), true);
}

void PostEffectGroup::SetEnable(bool enable)
{
    if (m_enable == enable)
    {
        return;
    }
    m_enable = enable;
    if (m_enable)
    {
        m_lastUpdateFrame = gEnv->nMainFrameID;
        m_enableDuration = 0.f;
        m_manager->Sort();
    }
    else
    {
        m_disableDuration = 0.f;
    }

    // Our m_enable state has changed.  Alert the PostEffectGroupManager of this.
    m_manager->SetGroupToggledThisFrame(this);
}

void PostEffectGroup::StopEffect()
{
    m_enable = false;
    m_enableDuration = 0.f;
    m_disableDuration = m_blendOut.GetKeyRangeEnd();
}

void PostEffectGroup::SetParam(const char* name, const PostEffectGroupParam& value)
{
    m_params[name] = value;
    if (m_enable)
    {
        m_lastUpdateFrame = gEnv->nMainFrameID;
        m_manager->Sort();
    }
}

void PostEffectGroup::ClearParams()
{
    m_params.clear();
    if (m_enable)
    {
        m_lastUpdateFrame = gEnv->nMainFrameID;
        m_manager->Sort();
    }
}

void PostEffectGroup::ApplyAtPosition(const Vec3& position)
{
    float distance = (position - gEnv->pSystem->GetViewCamera().GetPosition()).len();
    if (distance < m_fadeDistance)
    {
        m_strength += 1.f - distance / m_fadeDistance;
    }
}

void PostEffectGroup::BlendWith(AZStd::unordered_map<AZStd::string, PostEffectGroupParam>& paramMap)
{
    if (!m_enable && m_disableDuration >= m_blendOut.GetKeyRangeEnd())
    {
        return;
    }
    m_enableDuration += gEnv->pTimer->GetFrameTime();
    if (!m_enable)
    {
        m_disableDuration += gEnv->pTimer->GetFrameTime();
    }
    if (m_enable && !m_hold && m_enableDuration >= m_blendIn.GetKeyRangeEnd())
    {
        m_enable = false;
        m_disableDuration = m_enableDuration - m_blendIn.GetKeyRangeEnd();
    }
    for (auto& param : m_params)
    {
        if (paramMap.find(param.first) == paramMap.end())
        {
            paramMap[param.first] = param.second;
        }
        else
        {
            BlendVisitor blendVisitor;
            float blendInAmount = 1.f, blendOutAmount = 1.f;
            if (!m_blendIn.empty() && m_enableDuration < m_blendIn.GetKeyRangeEnd())
            {
                m_blendIn.InterpolateFloat(m_enableDuration, blendInAmount);
            }
            if (!m_enable) // m_blendOut.empty() is already checked above
            {
                m_blendOut.InterpolateFloat(m_disableDuration, blendOutAmount);
            }
            blendVisitor.Init(m_enable, blendInAmount * blendOutAmount * (m_fadeDistance ? m_strength : 1.f));
            paramMap[param.first] = AZStd::visit(blendVisitor, paramMap[param.first], param.second);
        }
    }
}

void PostEffectGroup::ClearSplines()
{
    m_blendIn.Clear();
    m_blendOut.Clear();
}

PostEffectGroupManager::PostEffectGroupManager()
{
    // Note that the priority of the Base group is 1, this is to prevent invalid ordering with the "Default" group.
    PostEffectGroup* base = new PostEffectGroup(this, "Base", PostEffectGroup::kPriorityBase, true, 0.f);
    base->SetEnable(true);
    m_groups.push_back(std::unique_ptr<PostEffectGroup>(base));
    gEnv->pRenderer->RegisterSyncWithMainListener(this);
    if (gEnv->IsEditor())
    {
        //  Only monitor assets in the editor.
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
    }
}

PostEffectGroupManager::~PostEffectGroupManager()
{
    gEnv->pRenderer->RemoveSyncWithMainListener(this);
    if (gEnv->IsEditor())
    {
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
    }
}

IPostEffectGroup* PostEffectGroupManager::GetGroup(const char* name)
{
    for (auto& group : m_groups)
    {
        if (strcmp(group->GetName(), name) == 0)
        {
            return group.get();
        }
    }
    
    // Group not loaded, so try to load it from XML
    // Load on demand instead of at startup so that users can place the XML file in any CryPak path
    return LoadGroup(name);
}

IPostEffectGroup* PostEffectGroupManager::GetGroup(const unsigned int index)
{
    if (index < m_groups.size())
    {
        return m_groups[index].get();
    }

    return nullptr;
}

const unsigned int PostEffectGroupManager::GetGroupCount()
{
    return m_groups.size();
}

IPostEffectGroup* PostEffectGroupManager::LoadGroup(const char* name, PostEffectGroup* groupToLoadInto)
{
    XmlNodeRef root = GetISystem()->LoadXmlFromFile(name);
    unsigned int priority;
    bool hold = false;
    float fadeDistance = 0.f;
    if (!root)
    {
        CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_ERROR, "Can't open post effect group '%s'.", name);
        return nullptr;
    }
    if (!root->isTag("PostEffectGroup"))
    {
        CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_ERROR, "Post effect group '%s' is missing 'PostEffectGroup' root tag", name);
        return nullptr;
    }
    if (!root->getAttr("priority", priority))
    {
        CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_ERROR, "Post effect group '%s' is missing 'priority' attribute", name);
        return nullptr;
    }
    root->getAttr("hold", hold);
    root->getAttr("fadeDistance", fadeDistance);
    PostEffectGroup* group = groupToLoadInto;
    if (!group)
    {
        //  No pointer was sent in, so create a new PostEffectGroup and add it to the list.
        group = new PostEffectGroup(this, name, static_cast<PostEffectGroup::GroupPriority>(priority), hold, fadeDistance);
        m_groups.push_back(std::unique_ptr<PostEffectGroup>(group));
    }
    else
    {
        //  Existing PostEffectGroup was sent in, reset its properties so that we can load onto it again.
        //  This is only called from OnCatalogAssetChanged, so should only happen in the editor.
        group->StopEffect();
        group->SetPriority(static_cast<PostEffectGroup::GroupPriority>(priority));
        group->SetHold(hold);
        group->SetFadeDistance(fadeDistance);
        group->ClearSplines();
    }
    
    for (int i = 0; i < root->getChildCount(); i++)
    {
        XmlNodeRef node = root->getChild(i);
        const char* effectName;
        if (node->isTag("Effect") && node->getAttr("name", &effectName))
        {
            // Set effect params
            for (int j = 0; j < node->getChildCount(); j++)
            {
                XmlNodeRef paramNode = node->getChild(j);
                const char* paramName;
                if (paramNode->isTag("Param") && paramNode->getAttr("name", &paramName))
                {
                    std::string paramFullName = std::string(effectName) + "_" + paramName;
                    float floatValue;
                    Vec4 vec4Value;
                    const char* stringValue;
                    if (paramNode->getAttr("floatValue", floatValue))
                    {
                        group->SetParam(paramFullName.c_str(), floatValue);
                    }
                    else if (paramNode->getAttr("vec4Value", vec4Value))
                    {
                        group->SetParam(paramFullName.c_str(), vec4Value);
                    }
                    else if (paramNode->getAttr("colorValue", vec4Value))
                    {
                        group->SetParam(("clr_" + paramFullName).c_str(), vec4Value);
                    }
                    else if (paramNode->getAttr("stringValue", &stringValue))
                    {
                        group->SetParam(paramFullName.c_str(), stringValue);
                    }
                    else if (paramNode->getAttr("textureValue", &stringValue))
                    {
                        group->SetParam(("tex_" + paramFullName).c_str(), stringValue);
                    }
                    else
                    {
                        CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "Post effect group '%s' effect '%s' param '%s' needs either a floatValue, vec4Value, colorValue, stringValue, or textureValue attribute",
                            name, effectName, paramName);
                    }
                }
                else
                {
                    CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "Post effect group '%s' effect '%s' must contain Param tags with a name attribute", name, effectName);
                }
            }
            continue;
        }
        ISplineInterpolator* spline =
            node->isTag("BlendIn") ? group->GetBlendIn() :
            node->isTag("BlendOut") ? group->GetBlendOut() :
            nullptr;
        if (spline)
        {
            // Add blend spline keys
            for (int j = 0; j < node->getChildCount(); j++)
            {
                XmlNodeRef keyNode = node->getChild(j);
                float time, value;
                if (keyNode->isTag("Key") && keyNode->getAttr("time", time) && keyNode->getAttr("value", value))
                {
                    spline->InsertKeyFloat(time, value);
                }
                else
                {
                    CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "Post effect group '%s' blend spline key must be of form <Key time=\"time\" value=\"value\"/>", name);
                }
            }
            // Set blend curve type
            const char* curveAttr = node->getAttr("curve");
            ESplineKeyTangentType curveType = SPLINE_KEY_TANGENT_NONE;
            if (strcmp(curveAttr, "linear") == 0)
            {
                curveType = SPLINE_KEY_TANGENT_LINEAR;
            }
            else if (strcmp(curveAttr, "step") == 0)
            {
                curveType = SPLINE_KEY_TANGENT_STEP;
            }
            else if (strcmp(curveAttr, "") != 0 && strcmp(curveAttr, "smooth") != 0)
            {
                CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "Post effect group '%s' %s spline has unrecognized curve '%s'. Expecting 'smooth', 'linear', or 'step'.",
                    name, node->getTag(), curveAttr);
            }
            for (int j = 0; j < spline->GetKeyCount(); j++)
            {
                spline->SetKeyFlags(j, (curveType << SPLINE_KEY_TANGENT_IN_SHIFT) | (curveType << SPLINE_KEY_TANGENT_OUT_SHIFT));
            }
            continue;
        }
        CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "Unrecognized XML tag in post effect group '%s'", name);
    }
    return group;
}

void PostEffectGroupManager::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
{
    for (auto& group : m_groups)
    {
        if (group->GetAssetId() == assetId)
        {
            //  Reload this asset. Sending in the pointer will cause LoadGroup to load in place.
            LoadGroup(group->GetName(), group.get());
            return;
        }
    }
}

void PostEffectGroupManager::SyncMainWithRender()
{
    // Blend postprocessing params
    m_paramCache.clear();

    // Flip our buffers and clear
    m_fillThreadIndex = (m_fillThreadIndex + 1) & 1;
    m_groupsToggledThisFrame[m_fillThreadIndex].clear();

    for (auto& group : m_groups)
    {
        group->BlendWith(m_paramCache);
        group->ResetStrength();
    }
    SyncVisitor syncVisitor;
    for (auto& param : m_paramCache)
    {
        syncVisitor.SetName(param.first);
        AZStd::visit(syncVisitor, param.second);
    }
    // Swap buffers in lower level system
    gEnv->pRenderer->SyncPostEffects();
}

void PostEffectGroupManager::Sort()
{
    // Sort effect groups by priority then time updated, so that they'll be blended in that order
    std::sort(m_groups.begin(), m_groups.end(), [](const std::unique_ptr<PostEffectGroup>& a, const std::unique_ptr<PostEffectGroup>& b)
        {
            if (a->GetPriority() == b->GetPriority())
            {
                return a->GetLastUpdateFrame() < b->GetLastUpdateFrame();
            }
            return a->GetPriority() < b->GetPriority();
        });
}

const PostEffectGroupList& PostEffectGroupManager::GetGroupsToggledThisFrame()
{
    unsigned int processThreadIndex = (m_fillThreadIndex + 1) & 1;
    return m_groupsToggledThisFrame[processThreadIndex];
}

void PostEffectGroupManager::SetGroupToggledThisFrame( IPostEffectGroup* group )
{
    m_groupsToggledThisFrame[m_fillThreadIndex].push_back(group);
}
