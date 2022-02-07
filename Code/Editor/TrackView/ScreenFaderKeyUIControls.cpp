/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"


// CryCommon
#include <CryCommon/Maestro/Types/AnimParamType.h>

// Editor
#include "TrackViewKeyPropertiesDlg.h"


//-----------------------------------------------------------------------------
//!
class CScreenFaderKeyUIControls
    : public CTrackViewKeyUIControls
{
    CSmartVariableArray     mv_table;
    CSmartVariable<float>   mv_fadeTime;
    CSmartVariable<Vec3>    mv_fadeColor;
    CSmartVariable<QString> mv_strTexture;
    CSmartVariable<bool>    mv_bUseCurColor;
    CSmartVariableEnum<int> mv_fadeType;
    CSmartVariableEnum<int> mv_fadechangeType;

public:
    //-----------------------------------------------------------------------------
    //!
    bool SupportTrackType(const CAnimParamType& paramType, [[maybe_unused]] EAnimCurveType trackType, [[maybe_unused]] AnimValueType valueType) const override
    {
        return paramType == AnimParamType::ScreenFader;
    }

    //-----------------------------------------------------------------------------
    //!
    void OnCreateVars() override
    {
        AddVariable(mv_table, "Key Properties");

        mv_fadeType->SetEnumList(nullptr);
        mv_fadeType->AddEnumItem("FadeIn", IScreenFaderKey::eFT_FadeIn);
        mv_fadeType->AddEnumItem("FadeOut", IScreenFaderKey::eFT_FadeOut);
        AddVariable(mv_table, mv_fadeType, "Type");

        mv_fadechangeType->SetEnumList(nullptr);
        mv_fadechangeType->AddEnumItem("Linear", IScreenFaderKey::eFCT_Linear);
        mv_fadechangeType->AddEnumItem("Square", IScreenFaderKey::eFCT_Square);
        mv_fadechangeType->AddEnumItem("Cubic Square", IScreenFaderKey::eFCT_CubicSquare);
        mv_fadechangeType->AddEnumItem("Square Root", IScreenFaderKey::eFCT_SquareRoot);
        mv_fadechangeType->AddEnumItem("Sin", IScreenFaderKey::eFCT_Sin);
        AddVariable(mv_table, mv_fadechangeType, "ChangeType");

        AddVariable(mv_table, mv_fadeColor, "Color", IVariable::DT_COLOR);

        mv_fadeTime->SetLimits(0.f, 100.f);
        AddVariable(mv_table, mv_fadeTime, "Duration");
        AddVariable(mv_table, mv_strTexture, "Texture", IVariable::DT_TEXTURE);
        AddVariable(mv_table, mv_bUseCurColor, "Use Current Color");
    }

    //-----------------------------------------------------------------------------
    //!
    bool OnKeySelectionChange(CTrackViewKeyBundle& keys) override;

    //-----------------------------------------------------------------------------
    //!
    void OnUIChange(IVariable* pVar, CTrackViewKeyBundle& keys) override;

    unsigned int GetPriority() const override { return 1; }

    static const GUID& GetClassID()
    {
        // {FBBC2407-C36B-45b2-9A54-0CF9CD3908FD}
        static const GUID guid =
        {
            0xfbbc2407, 0xc36b, 0x45b2, { 0x9a, 0x54, 0xc, 0xf9, 0xcd, 0x39, 0x8, 0xfd }
        };
        return guid;
    }
};

//-----------------------------------------------------------------------------
bool CScreenFaderKeyUIControls::OnKeySelectionChange(CTrackViewKeyBundle& keys)
{
    if (!keys.AreAllKeysOfSameType())
    {
        return false;
    }

    bool bAssigned = false;
    if (keys.GetKeyCount() == 1)
    {
        const CTrackViewKeyHandle& keyHandle = keys.GetKey(0);

        CAnimParamType paramType = keyHandle.GetTrack()->GetParameterType();
        if (paramType == AnimParamType::ScreenFader)
        {
            IScreenFaderKey screenFaderKey;
            keyHandle.GetKey(&screenFaderKey);

            mv_fadeTime = screenFaderKey.m_fadeTime;
            mv_fadeColor = Vec3(screenFaderKey.m_fadeColor.GetR(), screenFaderKey.m_fadeColor.GetG(), screenFaderKey.m_fadeColor.GetB());
            mv_strTexture = screenFaderKey.m_strTexture.c_str();
            mv_bUseCurColor = screenFaderKey.m_bUseCurColor;
            mv_fadeType = (int)screenFaderKey.m_fadeType;
            mv_fadechangeType = (int)screenFaderKey.m_fadeChangeType;

            bAssigned = true;
        }
    }
    return bAssigned;
}

//-----------------------------------------------------------------------------
void CScreenFaderKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
{
    if (!selectedKeys.AreAllKeysOfSameType())
    {
        return;
    }

    for (size_t keyIndex = 0, num = selectedKeys.GetKeyCount(); keyIndex < num; ++keyIndex)
    {
        CTrackViewKeyHandle selectedKey = selectedKeys.GetKey(static_cast<unsigned int>(keyIndex));

        CAnimParamType paramType = selectedKey.GetTrack()->GetParameterType();
        if (paramType == AnimParamType::ScreenFader)
        {
            IScreenFaderKey screenFaderKey;
            selectedKey.GetKey(&screenFaderKey);

            SyncValue(mv_fadeTime, screenFaderKey.m_fadeTime, false, pVar);

            SyncValue(mv_bUseCurColor, screenFaderKey.m_bUseCurColor, false, pVar);

            if (pVar == mv_fadeTime.GetVar())
            {
                screenFaderKey.m_fadeTime = MAX((float)mv_fadeTime, 0.f);
            }
            else if (pVar == mv_strTexture.GetVar())
            {
                QString sTexture = mv_strTexture;
                screenFaderKey.m_strTexture = sTexture.toUtf8().data();
            }
            else if (pVar == mv_fadeType.GetVar())
            {
                screenFaderKey.m_fadeType = IScreenFaderKey::EFadeType((int)mv_fadeType);
            }
            else if (pVar == mv_fadechangeType.GetVar())
            {
                screenFaderKey.m_fadeChangeType = IScreenFaderKey::EFadeChangeType((int)mv_fadechangeType);
            }
            else if (pVar == mv_fadeColor.GetVar())
            {
                Vec3 color = mv_fadeColor;
                screenFaderKey.m_fadeColor = AZ::Color(color.x, color.y, color.z, screenFaderKey.m_fadeType == IScreenFaderKey::eFT_FadeIn ? 1.f : 0.f);
            }

            selectedKey.SetKey(&screenFaderKey);
        }
    }
}

REGISTER_QT_CLASS_DESC(CScreenFaderKeyUIControls, "TrackView.KeyUI.ScreenFader", "TrackViewKeyUI");
