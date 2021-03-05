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

// include the required headers
#include "EMotionFXConfig.h"
#include "Node.h"
#include "MorphTarget.h"
#include <MCore/Source/StringConversions.h>
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MorphTarget, DeformerAllocator, 0)


    // the constructor
    MorphTarget::MorphTarget(const char* name)
        : BaseObject()
    {
        mRangeMin       = 0.0f;
        mRangeMax       = 1.0f;
        mPhonemeSets    = PHONEMESET_NONE;

        // set the name
        SetName(name);
    }


    // destructor
    MorphTarget::~MorphTarget()
    {
    }


    // convert the given phoneme name to a phoneme set
    MorphTarget::EPhonemeSet MorphTarget::FindPhonemeSet(const AZStd::string& phonemeName)
    {
        // return neutral pose if the phoneme name is empty
        if (phonemeName.empty() || phonemeName == "x")
        {
            return PHONEMESET_NEUTRAL_POSE;
        }

        // AW
        if (AzFramework::StringFunc::Equal(phonemeName.c_str(), "AW", false /* no case */))
        {
            return PHONEMESET_AW;
        }

        // UW_UH_OY
        if (AzFramework::StringFunc::Equal(phonemeName.c_str(), "UW", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "UH", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "OY", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "UW_UH_OY", false /* no case */))
        {
            return PHONEMESET_UW_UH_OY;
        }

        // AA_AO_OW
        if (AzFramework::StringFunc::Equal(phonemeName.c_str(), "AA", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "AO", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "OW", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "AA_AO_OW", false /* no case */))
        {
            return PHONEMESET_AA_AO_OW;
        }

        // IH_AE_AH_EY_AY_H
        if (AzFramework::StringFunc::Equal(phonemeName.c_str(), "IH", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "AE", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "AH", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "EY", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "AY", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "H", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "IH_AE_AH_EY_AY_H", false /* no case */))
        {
            return PHONEMESET_IH_AE_AH_EY_AY_H;
        }

        // IY_EH_Y
        if (AzFramework::StringFunc::Equal(phonemeName.c_str(), "IY", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "EH", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "Y", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "IY_EH_Y", false /* no case */))
        {
            return PHONEMESET_IY_EH_Y;
        }

        // L_EL
        if (AzFramework::StringFunc::Equal(phonemeName.c_str(), "", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "E", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "L_E", false /* no case */))
        {
            return PHONEMESET_L_EL;
        }

        // N_NG_CH_J_DH_D_G_T_K_Z_ZH_TH_S_SH
        if (AzFramework::StringFunc::Equal(phonemeName.c_str(), "N", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "NG", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "CH", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "J", false /* no case */)  || AzFramework::StringFunc::Equal(phonemeName.c_str(), "DH", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "D", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "G", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "T", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "K", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "Z", false /* no case */)  || AzFramework::StringFunc::Equal(phonemeName.c_str(), "ZH", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "TH", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "S", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "SH", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "N_NG_CH_J_DH_D_G_T_K_Z_ZH_TH_S_SH", false /* no case */))
        {
            return PHONEMESET_N_NG_CH_J_DH_D_G_T_K_Z_ZH_TH_S_SH;
        }

        // R_ER
        if (AzFramework::StringFunc::Equal(phonemeName.c_str(), "R", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "ER", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "R_ER", false /* no case */))
        {
            return PHONEMESET_R_ER;
        }

        // M_B_P_X
        if (AzFramework::StringFunc::Equal(phonemeName.c_str(), "M", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "B", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "P", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "X", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "M_B_P_X", false /* no case */))
        {
            return PHONEMESET_M_B_P_X;
        }

        // F_V
        if (AzFramework::StringFunc::Equal(phonemeName.c_str(), "F", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "V", false /* no case */) || AzFramework::StringFunc::Equal(phonemeName.c_str(), "F_V", false /* no case */))
        {
            return PHONEMESET_F_V;
        }

        // W
        if (AzFramework::StringFunc::Equal(phonemeName.c_str(), "W", false /* no case */))
        {
            return PHONEMESET_W;
        }

        return PHONEMESET_NONE;
    }


    // get the string/name for a given phoneme set
    AZStd::string MorphTarget::GetPhonemeSetString(const EPhonemeSet phonemeSet)
    {
        AZStd::string result;

        // build the string
        if (phonemeSet & PHONEMESET_NEUTRAL_POSE)
        {
            result += ",NEUTRAL_POSE";
        }
        if (phonemeSet & PHONEMESET_M_B_P_X)
        {
            result += ",M_B_P_X";
        }
        if (phonemeSet & PHONEMESET_AA_AO_OW)
        {
            result += ",AA_AO_OW";
        }
        if (phonemeSet & PHONEMESET_IH_AE_AH_EY_AY_H)
        {
            result += ",IH_AE_AH_EY_AY_H";
        }
        if (phonemeSet & PHONEMESET_AW)
        {
            result += ",AW";
        }
        if (phonemeSet & PHONEMESET_N_NG_CH_J_DH_D_G_T_K_Z_ZH_TH_S_SH)
        {
            result += ",N_NG_CH_J_DH_D_G_T_K_Z_ZH_TH_S_SH";
        }
        if (phonemeSet & PHONEMESET_IY_EH_Y)
        {
            result += ",IY_EH_Y";
        }
        if (phonemeSet & PHONEMESET_UW_UH_OY)
        {
            result += ",UW_UH_OY";
        }
        if (phonemeSet & PHONEMESET_F_V)
        {
            result += ",F_V";
        }
        if (phonemeSet & PHONEMESET_L_EL)
        {
            result += ",L_E";
        }
        if (phonemeSet & PHONEMESET_W)
        {
            result += ",W";
        }
        if (phonemeSet & PHONEMESET_R_ER)
        {
            result += ",R_ER";
        }

        // remove any leading commas
        AzFramework::StringFunc::Strip(result, MCore::CharacterConstants::comma, true /* case sensitive */, true /* beginning */, false /* ending */);

        // return the resulting string
        return result;
    }


    // calculate the weight in range of 0..1
    float MorphTarget::CalcNormalizedWeight(float rangedWeight) const
    {
        const float range = mRangeMax - mRangeMin;
        if (MCore::Math::Abs(range) > 0.0f)
        {
            return (rangedWeight - mRangeMin) / range;
        }
        else
        {
            return 0.0f;
        }
    }


    // there are 12 possible phoneme sets
    uint32 MorphTarget::GetNumAvailablePhonemeSets()
    {
        return 12;
    }


    // enable or disable a given phoneme set
    void MorphTarget::EnablePhonemeSet(EPhonemeSet set, bool enabled)
    {
        if (enabled)
        {
            mPhonemeSets = (EPhonemeSet)((uint32)mPhonemeSets | (uint32)set);
        }
        else
        {
            mPhonemeSets = (EPhonemeSet)((uint32)mPhonemeSets & (uint32) ~set);
        }
    }


    // copy the base class members to the target class
    void MorphTarget::CopyBaseClassMemberValues(MorphTarget* target)
    {
        target->mNameID         = mNameID;
        target->mRangeMin       = mRangeMin;
        target->mRangeMax       = mRangeMax;
        target->mPhonemeSets    = mPhonemeSets;
    }


    const char* MorphTarget::GetName() const
    {
        return MCore::GetStringIdPool().GetName(mNameID).c_str();
    }


    const AZStd::string& MorphTarget::GetNameString() const
    {
        return MCore::GetStringIdPool().GetName(mNameID);
    }


    void MorphTarget::SetRangeMin(float rangeMin)
    {
        mRangeMin = rangeMin;
    }


    void MorphTarget::SetRangeMax(float rangeMax)
    {
        mRangeMax = rangeMax;
    }


    float MorphTarget::GetRangeMin() const
    {
        return mRangeMin;
    }


    float MorphTarget::GetRangeMax() const
    {
        return mRangeMax;
    }


    void MorphTarget::SetName(const char* name)
    {
        mNameID = MCore::GetStringIdPool().GenerateIdForString(name);
    }


    void MorphTarget::SetPhonemeSets(EPhonemeSet phonemeSets)
    {
        mPhonemeSets = phonemeSets;
    }


    MorphTarget::EPhonemeSet MorphTarget::GetPhonemeSets() const
    {
        return mPhonemeSets;
    }


    bool MorphTarget::GetIsPhonemeSetEnabled(EPhonemeSet set) const
    {
        return (mPhonemeSets & set) != 0;
    }


    float MorphTarget::CalcRangedWeight(float weight) const
    {
        return mRangeMin + (weight * (mRangeMax - mRangeMin));
    }


    float MorphTarget::CalcZeroInfluenceWeight() const
    {
        return MCore::Math::Abs(mRangeMin) / MCore::Math::Abs(mRangeMax - mRangeMin);
    }


    bool MorphTarget::GetIsPhoneme() const
    {
        return (mPhonemeSets != PHONEMESET_NONE);
    }
} // namespace EMotionFX
