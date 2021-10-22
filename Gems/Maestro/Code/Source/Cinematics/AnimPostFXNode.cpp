/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/allocator_stateless.h>
#include "AnimPostFXNode.h"
#include "AnimSplineTrack.h"
#include "CompoundSplineTrack.h"
#include "BoolTrack.h"
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimParamType.h"
#include "Maestro/Types/AnimValueType.h"

//////////////////////////////////////////////////////////////////////////
//!
class CFXNodeDescription
    : public _i_reference_target_t
{
public:
    //-----------------------------------------------------------------------------
    //!
    class CControlParamBase
        : public _i_reference_target_t
    {
    public:
        CControlParamBase(){}

        virtual void SetDefault(float val) = 0;
        virtual void SetDefault(bool val) = 0;
        virtual void SetDefault(Vec4 val) = 0;

        virtual void GetDefault(float& val) const = 0;
        virtual void GetDefault(bool& val) const = 0;
        virtual void GetDefault(Vec4& val) const = 0;

        AZStd::basic_string<char, AZStd::char_traits<char>, AZStd::stateless_allocator> m_name;

    protected:
        virtual ~CControlParamBase(){}
    };

    //-----------------------------------------------------------------------------
    //!
    template<typename T>
    class TControlParam
        : public CControlParamBase
    {
    public:
        virtual void SetDefault([[maybe_unused]] float val) { assert(0); }
        virtual void SetDefault([[maybe_unused]] bool val) { assert(0); }
        virtual void SetDefault([[maybe_unused]] Vec4 val) { assert(0); }

        virtual void GetDefault([[maybe_unused]] float& val) const { assert(0); }
        virtual void GetDefault([[maybe_unused]] bool& val) const { assert(0); }
        virtual void GetDefault([[maybe_unused]] Vec4& val) const { assert(0); }

    protected:
        virtual ~TControlParam(){}

        T m_defaultValue;
    };

    //-----------------------------------------------------------------------------
    //!
    CFXNodeDescription() {}

    //-----------------------------------------------------------------------------
    //!
    template<typename T>
    void AddSupportedParam(const char* sName, AnimValueType eValueType, const char* sControlName, T defaultValue)
    {
        CAnimNode::SParamInfo param;
        param.name = sName;
        param.paramType = static_cast<AnimParamType>(static_cast<int>(AnimParamType::User) + static_cast<int>(m_nodeParams.size()));
        param.valueType = eValueType;
        m_nodeParams.push_back(param);

        TControlParam<T>* control = new TControlParam<T>;
        control->m_name = sControlName;
        control->SetDefault(defaultValue);

        m_controlParams.push_back(control);
    }

    //-----------------------------------------------------------------------------
    //!
    StaticInstance<std::vector<CAnimNode::SParamInfo>> m_nodeParams;
    StaticInstance<std::vector< _smart_ptr<CControlParamBase> >> m_controlParams;
};

//-----------------------------------------------------------------------------
template<>
void CFXNodeDescription::TControlParam<float>::SetDefault(float val)
{
    m_defaultValue = val;
}
template<>
void CFXNodeDescription::TControlParam<bool>::SetDefault(bool val)
{
    m_defaultValue = val;
}
template<>
void CFXNodeDescription::TControlParam<Vec4>::SetDefault(Vec4 val)
{
    m_defaultValue = val;
}
template<>
void CFXNodeDescription::TControlParam<float>::GetDefault(float& val) const
{
    val = m_defaultValue;
}
template<>
void CFXNodeDescription::TControlParam<bool>::GetDefault(bool& val) const
{
    val = m_defaultValue;
}
template<>
void CFXNodeDescription::TControlParam<Vec4>::GetDefault(Vec4& val) const
{
    val = m_defaultValue;
}

//-----------------------------------------------------------------------------
StaticInstance<CAnimPostFXNode::FxNodeDescriptionMap> CAnimPostFXNode::s_fxNodeDescriptions;
bool CAnimPostFXNode::s_initialized = false;


//-----------------------------------------------------------------------------
CAnimPostFXNode::CAnimPostFXNode()
    : CAnimPostFXNode(0, AnimNodeType::Invalid, nullptr)
{
}

//-----------------------------------------------------------------------------
CAnimPostFXNode::CAnimPostFXNode(const int id, AnimNodeType nodeType, CFXNodeDescription* pDesc)
    : CAnimNode(id, nodeType)
    , m_pDescription(pDesc)
{
}

//-----------------------------------------------------------------------------
void CAnimPostFXNode::Initialize()
{
    if (!s_initialized)
    {
        s_initialized = true;

        //////////////////////////////////////////////////////////////////////////
        //! Radial Blur
        {
            CFXNodeDescription* pDesc = new CFXNodeDescription();
            s_fxNodeDescriptions[AnimNodeType::RadialBlur] = pDesc;
            pDesc->m_nodeParams.reserve(4);
            pDesc->m_controlParams.reserve(4);
            pDesc->AddSupportedParam<float>("Amount", AnimValueType::Float, "FilterRadialBlurring_Amount", 0.0f);
            pDesc->AddSupportedParam<float>("ScreenPosX", AnimValueType::Float, "FilterRadialBlurring_ScreenPosX", 0.5f);
            pDesc->AddSupportedParam<float>("ScreenPosY", AnimValueType::Float, "FilterRadialBlurring_ScreenPosY", 0.5f);
            pDesc->AddSupportedParam<float>("BlurringRadius", AnimValueType::Float, "FilterRadialBlurring_Radius", 1.0f);
        }

        //////////////////////////////////////////////////////////////////////////
        //! Color Correction
        {
            CFXNodeDescription* pDesc = new CFXNodeDescription();
            s_fxNodeDescriptions[AnimNodeType::ColorCorrection] = pDesc;
            pDesc->m_nodeParams.reserve(8);
            pDesc->m_controlParams.reserve(8);
            pDesc->AddSupportedParam<float>("Cyan", AnimValueType::Float, "Global_User_ColorC", 0.0f);
            pDesc->AddSupportedParam<float>("Magenta", AnimValueType::Float, "Global_User_ColorM", 0.0f);
            pDesc->AddSupportedParam<float>("Yellow", AnimValueType::Float, "Global_User_ColorY", 0.0f);
            pDesc->AddSupportedParam<float>("Luminance", AnimValueType::Float, "Global_User_ColorK", 0.0f);
            pDesc->AddSupportedParam<float>("Brightness", AnimValueType::Float, "Global_User_Brightness", 1.0f);
            pDesc->AddSupportedParam<float>("Contrast", AnimValueType::Float, "Global_User_Contrast", 1.0f);
            pDesc->AddSupportedParam<float>("Saturation", AnimValueType::Float, "Global_User_Saturation", 1.0f);
            pDesc->AddSupportedParam<float>("Hue", AnimValueType::Float, "Global_User_ColorHue", 0.0f);
        }


        //////////////////////////////////////////////////////////////////////////
        //! Depth of Field
        {
            CFXNodeDescription* pDesc = new CFXNodeDescription();
            s_fxNodeDescriptions[AnimNodeType::DepthOfField] = pDesc;
            pDesc->m_nodeParams.reserve(4);
            pDesc->m_controlParams.reserve(4);
            pDesc->AddSupportedParam<bool>("Enable", AnimValueType::Bool, "Dof_User_Active", false);
            pDesc->AddSupportedParam<float>("FocusDistance", AnimValueType::Float, "Dof_User_FocusDistance", 3.5f);
            pDesc->AddSupportedParam<float>("FocusRange", AnimValueType::Float, "Dof_User_FocusRange", 5.0f);
            pDesc->AddSupportedParam<float>("BlurAmount", AnimValueType::Float, "Dof_User_BlurAmount", 1.0f);
        }

        //////////////////////////////////////////////////////////////////////////
        //! Shadow setup - expose couple shadow controls to cinematics
        {
            CFXNodeDescription* pDesc = new CFXNodeDescription();
            s_fxNodeDescriptions[AnimNodeType::ShadowSetup] = pDesc;
            pDesc->m_nodeParams.reserve(1);
            pDesc->m_controlParams.reserve(1);
            pDesc->AddSupportedParam<float>("GSMCache", AnimValueType::Bool, "GSMCacheParam", true);
        }
    }
}

//-----------------------------------------------------------------------------
/*static*/ CFXNodeDescription* CAnimPostFXNode::GetFXNodeDescription(AnimNodeType nodeType)
{
    CFXNodeDescription* retDescription = nullptr;

    CAnimPostFXNode::Initialize();

    FxNodeDescriptionMap::iterator itr = s_fxNodeDescriptions.find(nodeType);

    if (itr != s_fxNodeDescriptions.end())
    {
        retDescription = itr->second;
    }

    return retDescription;
}

//-----------------------------------------------------------------------------
CAnimNode* CAnimPostFXNode::CreateNode(const int id, AnimNodeType nodeType)
{
    CAnimNode* retNode = nullptr;

    CFXNodeDescription* pDesc = GetFXNodeDescription(nodeType);

    if (pDesc)
    {
        retNode = aznew CAnimPostFXNode(id, nodeType, pDesc);
        static_cast<CAnimPostFXNode*>(retNode)->m_nodeType = nodeType;
    }

    return retNode;
}

//-----------------------------------------------------------------------------
void CAnimPostFXNode::InitPostLoad(IAnimSequence* sequence)
{
    CAnimNode::InitPostLoad(sequence);

    // For AZ::Serialization, m_nodeType will have be reflected and deserialized. Find the appropriate FXNodeDescription for it
    // and store the pointer
    m_pDescription = GetFXNodeDescription(m_nodeType);
    if (!m_pDescription)
    {
        // This is not ideal - we should never get here unless someone is tampering with data. We can't remove the node at this point,
        // we can't use a defatult description without crashing later, so we simply assert.
        AZ_Assert(false, "Unrecognized PostFX nodeType in Track View node %s. Please remove this node from the sequence.", m_name.c_str());
    }
}

//-----------------------------------------------------------------------------
void CAnimPostFXNode::SerializeAnims(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    if (bLoading)
    {
        int paramIdVersion = 0;
        xmlNode->getAttr("ParamIdVersion", paramIdVersion);

        // Fix old param types
        if (paramIdVersion <= 2)
        {
            int num = xmlNode->getChildCount();
            for (int i = 0; i < num; ++i)
            {
                XmlNodeRef trackNode = xmlNode->getChild(i);

                CAnimParamType paramType;
                paramType.Serialize(trackNode, true);
                // Don't use APARAM_USER because it could change in newer versions
                // CAnimNode::SerializeAnims will then take care of that
                paramType = static_cast<AnimParamType>(static_cast<int>(paramType.GetType()) + OLD_APARAM_USER);
                paramType.Serialize(trackNode, false);
            }
        }
    }

    CAnimNode::SerializeAnims(xmlNode, bLoading, bLoadEmptyTracks);
}

//-----------------------------------------------------------------------------
unsigned int CAnimPostFXNode::GetParamCount() const
{
    return static_cast<unsigned int>(m_pDescription->m_nodeParams.size());
}

//-----------------------------------------------------------------------------
CAnimParamType CAnimPostFXNode::GetParamType(unsigned int nIndex) const
{
    if (nIndex < m_pDescription->m_nodeParams.size())
    {
        return m_pDescription->m_nodeParams[nIndex].paramType;
    }

    return AnimParamType::Invalid;
}

//-----------------------------------------------------------------------------
bool CAnimPostFXNode::GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const
{
    for (size_t i = 0; i < m_pDescription->m_nodeParams.size(); ++i)
    {
        if (m_pDescription->m_nodeParams[i].paramType == paramId)
        {
            info = m_pDescription->m_nodeParams[i];
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
void CAnimPostFXNode::CreateDefaultTracks()
{
    for (size_t i = 0; i < m_pDescription->m_nodeParams.size(); ++i)
    {
        IAnimTrack* pTrack = CreateTrackInternal(m_pDescription->m_nodeParams[i].paramType,
                eAnimCurveType_BezierFloat, m_pDescription->m_nodeParams[i].valueType);

        //Setup default value
        AnimValueType valueType = m_pDescription->m_nodeParams[i].valueType;
        if (valueType == AnimValueType::Float)
        {
            C2DSplineTrack* pFloatTrack = static_cast<C2DSplineTrack*>(pTrack);
            float val(0);
            m_pDescription->m_controlParams[i]->GetDefault(val);
            pFloatTrack->SetDefaultValue(Vec2(0, val));
        }
        else if (valueType == AnimValueType::Bool)
        {
            CBoolTrack* pBoolTrack = static_cast<CBoolTrack*>(pTrack);
            bool val = false;
            m_pDescription->m_controlParams[i]->GetDefault(val);
            pBoolTrack->SetDefaultValue(val);
        }
        else if (valueType == AnimValueType::Vector4)
        {
            CCompoundSplineTrack* pCompoundTrack = static_cast<CCompoundSplineTrack*>(pTrack);
            Vec4 val(0.0f, 0.0f, 0.0f, 0.0f);
            m_pDescription->m_controlParams[i]->GetDefault(val);
            pCompoundTrack->SetValue(0.0f, val, true);
        }
    }
}

//-----------------------------------------------------------------------------
void CAnimPostFXNode::Animate(SAnimContext& ac)
{
    for (size_t i = 0; i < m_tracks.size(); ++i)
    {
        IAnimTrack* pTrack = m_tracks[i].get();
        assert(pTrack);
        size_t paramIndex = (size_t)static_cast<int>(m_tracks[i]->GetParameterType().GetType()) - static_cast<int>(AnimParamType::User);
        assert(paramIndex < m_pDescription->m_nodeParams.size());

        if (pTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled)
        {
            continue;
        }

        if (pTrack->IsMasked(ac.trackMask))
        {
            continue;
        }

        AnimValueType valueType = m_pDescription->m_nodeParams[paramIndex].valueType;

        // sorry: quick & dirty solution for c2 shipping - custom type handling for shadows - make this properly after shipping
        if (GetType() == AnimNodeType::ShadowSetup && valueType == AnimValueType::Bool)
        {
            bool val(false);
            pTrack->GetValue(ac.time, val);
        }
        else
        if (valueType == AnimValueType::Float)
        {
            float val(0);
            pTrack->GetValue(ac.time, val);
        }
        else if (valueType == AnimValueType::Bool)
        {
            bool val(false);
            pTrack->GetValue(ac.time, val);
        }
        else if (valueType == AnimValueType::Vector4)
        {
            Vec4 val(0.0f, 0.0f, 0.0f, 0.0f);
            static_cast<CCompoundSplineTrack*>(pTrack)->GetValue(ac.time, val);
        }
    }
}

//-----------------------------------------------------------------------------
void CAnimPostFXNode::OnReset()
{
    CAnimNode::OnReset();

    // Reset each postFX param to its default.
    for (size_t i = 0; i < m_tracks.size(); ++i)
    {
        assert(m_tracks[i].get());
        size_t paramIndex = (size_t)static_cast<int>(m_tracks[i]->GetParameterType().GetType()) - static_cast<int>(AnimParamType::User);
        assert(paramIndex < m_pDescription->m_nodeParams.size());

        AnimValueType valueType = m_pDescription->m_nodeParams[paramIndex].valueType;

        // sorry: quick & dirty solution for c2 shipping - custom type handling for shadows - make this properly after shipping
        if (GetType() == AnimNodeType::ShadowSetup && valueType == AnimValueType::Bool)
        {
            bool val(false);
            m_pDescription->m_controlParams[paramIndex]->GetDefault(val);
        }
        else
        if (valueType == AnimValueType::Float)
        {
            float val(0);
            m_pDescription->m_controlParams[paramIndex]->GetDefault(val);
        }
        else if (valueType == AnimValueType::Bool)
        {
            bool val(false);
            m_pDescription->m_controlParams[paramIndex]->GetDefault(val);
        }
        else if (valueType == AnimValueType::Vector4)
        {
            Vec4 val(0.0f, 0.0f, 0.0f, 0.0f);
            m_pDescription->m_controlParams[paramIndex]->GetDefault(val);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimPostFXNode::Reflect(AZ::ReflectContext* context)
{
    if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<CAnimPostFXNode, CAnimNode>()
            ->Version(1);
    }
}
