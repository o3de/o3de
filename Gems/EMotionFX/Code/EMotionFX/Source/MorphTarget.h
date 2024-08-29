/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include <AzCore/Math/Quaternion.h>
#include "EMotionFXConfig.h"
#include <MCore/Source/RefCounted.h>
#include "EMotionFXManager.h"
#include <MCore/Source/StringIdPool.h>


namespace EMotionFX
{
    // forward declarations
    class Actor;
    class ActorInstance;
    class Node;
    class Pose;


    /**
     * The  morph target base class.
     * Morph targets apply additive modifications to nodes or meshes or anything else.
     */
    class EMFX_API MorphTarget
        : public MCore::RefCounted

    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        /**
         * The phoneme sets, used for lip-sync.
         * If you modify this enum, be sure to also modify the value returned by: MorphTarget::GetNumAvailablePhonemeSets().
         */
        enum EPhonemeSet
        {
            PHONEMESET_NONE                                 = 0,
            PHONEMESET_NEUTRAL_POSE                         = 1 << 0,
            PHONEMESET_M_B_P_X                              = 1 << 1,
            PHONEMESET_AA_AO_OW                             = 1 << 2,
            PHONEMESET_IH_AE_AH_EY_AY_H                     = 1 << 3,
            PHONEMESET_AW                                   = 1 << 4,
            PHONEMESET_N_NG_CH_J_DH_D_G_T_K_Z_ZH_TH_S_SH    = 1 << 5,
            PHONEMESET_IY_EH_Y                              = 1 << 6,
            PHONEMESET_UW_UH_OY                             = 1 << 7,
            PHONEMESET_F_V                                  = 1 << 8,
            PHONEMESET_L_EL                                 = 1 << 9,
            PHONEMESET_W                                    = 1 << 10,
            PHONEMESET_R_ER                                 = 1 << 11
        };

        /**
         * Apply a transformation to the given position, rotation and scale, on such a way that this
         * morph target adjusts the given transformation data. On this way we can accumulate the effects of
         * different morph targets to the same node.
         * @param actorInstance The actor instance to apply the transform to.
         * @param nodeIndex The node where the given transform info belongs to, so the node which we are adjusting.
         *                  However the node itself will not be modified by this method.
         * @param position This must contain the initial position, and will be modified inside this method as well.
         * @param rotation This must contain the initial rotation, and will be modified inside this method as well.
         * @param scale This must contain the initial scale, and will be modified inside this method as well.
         * @param weight The absolute weight value.
         */
        virtual void ApplyTransformation(ActorInstance* actorInstance, size_t nodeIndex, AZ::Vector3& position, AZ::Quaternion& rotation, AZ::Vector3& scale, float weight) = 0;

        /**
         * Get the unique ID of this morph target.
         * Just like in the Node class, the ID is generated based on the name (a string).
         * Every string containing the same text will have the same ID. With this we can reduce from expensive
         * name compares to simple integer compares.
         * @result The unique ID of the morph target.
         */
        MCORE_INLINE uint32 GetID() const                                   { return m_nameId; }

        /**
         * Get the unique name of the morph target.
         * @result The name of the morph target.
         */
        const char* GetName() const;

        /**
         * Get the unique name of the morph target, in form of a AZStd::string object.
         * @result The name of the morph target.
         */
        const AZStd::string& GetNameString() const;

        /**
         * Set the minimum weight range value of this morph target.
         * On default this value is zero.
         * The equations used are: new = current + delta * weight.
         * Delta is the difference between the original pose and the pose passed to InitFromPose or the extended
         * constructor. This means that normally the value for weight has a range of [0..1].
         * However, this can be changed. The range is used inside LM Studio only.
         * @param rangeMin The minimum weight value.
         */
        void SetRangeMin(float rangeMin);

        /**
         * Set the maximum weight range value of this morph target.
         * On default this value is one.
         * For more information about what exactly this 'range' is, see the method SetRangeMin().
         * @param rangeMax The maximum weight value.
         * @see SetRangeMin
         */
        void SetRangeMax(float rangeMax);

        /**
         * Get the minimum weight range value of this morph target.
         * On default this value is zero.
         * For more information about what exactly this 'range' is, see the method SetRangeMin().
         * @result The minumim weight range value.
         * @see SetRangeMin
         */
        float GetRangeMin() const;

        /**
         * Get the maximum weight range value of this morph target.
         * On default this value is one.
         * For more information about what exactly this 'range' is, see the method SetRangeMin().
         * @result The maximum weight range value.
         * @see SetRangeMin
         */
        float GetRangeMax() const;

        /**
         * Get the type of morph target.
         * You can have different types of morph targets, such as morph targets which work
         * with bones, or which work with morphing or other techniques.
         * @result The unique type ID of the morph target.
         */
        virtual uint32 GetType() const = 0;

        /**
         * Change the name of the morph target.
         * This will also automatically update the ID, which is returned by GetID().
         * @param name The new name of the morph target.
         */
        void SetName(const char* name);

        /**
         * Set the phoneme sets. This is used for lipsync generation.
         * It allows us to link the visual representation of a phoneme with one detected in the audio file/stream.
         * @param phonemeSets The phoneme sets that this phoneme represents. This can be a combination of any of the phoneme sets in the enum.
         */
        void SetPhonemeSets(EPhonemeSet phonemeSets);

        /**
         * Get the phoneme sets. This is used for lipsync generation.
         * It allows us to link the visual representation of a phoneme with one detected in the audio file/stream.
         * @result The phoneme sets represented by this morph target.
         */
        EPhonemeSet GetPhonemeSets() const;

        /**
         * Link or unlink this morph target with a given phoneme set.
         * @param set The phoneme set to link or unlink this morph target with.
         * @param enabled Set to true to link it with the given set, or false to unlink it.
         */
        void EnablePhonemeSet(EPhonemeSet set, bool enabled);

        /**
         * Check if this morph target represents a given phoneme set or not.
         * @result Returns true when this morph target represents the specified phoneme set. Otherwise false is returned.
         */
        bool GetIsPhonemeSetEnabled(EPhonemeSet set) const;

        /**
         * Convert the given phoneme name to a phoneme set. Searches all phoneme sets and checks if the passed phoneme
         * name is part of a phoneme set.
         * @param phonemeName The name of the phoneme (e.g. "UW", TH", "EY").
         * @result The corresponding phoneme set. If the phoneme set has not been found or if it is empty the PHONEMESET_NEUTRAL_POSE value will be returned.
         */
        static EPhonemeSet FindPhonemeSet(const AZStd::string& phonemeName);

        /**
         * Get the name of a phoneme set from the given phoneme set type.
         * This is used to get phoneme morph targets names.
         * NOTE: If this morph target represents multiple phoneme sets, it will be separated with a comma character.
         *       An example of a returned string: "PHONEMESET_L_EL,PHONEMESET_W".
         * @param phonemeSet The phoneme set value.
         * @return String describing the given phoneme set.
         */
        static AZStd::string GetPhonemeSetString(const EPhonemeSet phonemeSet);

        /**
         * Get the number of available phoneme sets inside the enum EPhonemeSet.
         * The PHONEMESET_NONE enum value is not included in this amount.
         * @result Amount of available phoneme sets.
         */
        static uint32 GetNumAvailablePhonemeSets();

        /**
         * Initializes this expresion part from a given actor representing the pose.
         * The morph target will filter out all data which is changed compared to the base pose and
         * store this information on a specific way so it can be used to accumulate multiple morph targets
         * together and apply them to the actor to which this morph target is attached to.
         * @param captureTransforms Set this to true if you want this morph target to capture rigid transformations (changes in pos/rot/scale).
         * @param neutralPose The actor that contains the neutral pose.
         * @param targetPose The actor representing the pose of the character when the weight value would equal 1.
         */
        virtual void InitFromPose(bool captureTransforms, Actor* neutralPose, Actor* targetPose) = 0;

        /**
         * Checks if this morph target would influence a given node.
         * @param nodeIndex The node number to perform the check on.
         * @result Returns true if the given node will be modified by this morph target, otherwise false is returned.
         */
        virtual bool Influences(size_t nodeIndex) const = 0;

        /**
         * Calculate the range based weight value from a normalized weight value given by a facial animation key frame.
         * @param weight The normalized weight value, which must be in range of [0..1].
         * @return The weight value to be used in calculations. The returned value will be in range of [GetRangeMin()..GetRangeMax()]
         */
        float CalcRangedWeight(float weight) const;

        /**
         * Calculates the normalized weight value that is in range of [0..1], on which this morph target would have no influence.
         * A normalized weight of zero doesn't mean that this morph target has no influence. It is possible that the minimum range value
         * of the slider is for example -1, while the maximum range would be 1. In that case a weight value of 0, would mean a un-normalized (ranged) weight
         * of -1. The normalized weight that has a ranged (unnormalized) weight of zero would be 0.5 in this case.
         * This method calculates the normalized weight value that is in range of [0..1] which would result in a ranged (un-normalized) weight of zero.
         * @result The normalized weight value that will result in a ranged weight of zero, which would mean no influence.
         */
        float CalcZeroInfluenceWeight() const;

        /**
         * Calculate a normalized weight, in range of [0..1], based on the current weight, and the currently setup min and max range
         * of the morph target.
         * @param rangedWeight The weight, which is in range of [minRange..maxRange].
         * @result The weight value in range of [0..1].
         */
        float CalcNormalizedWeight(float rangedWeight) const;

        /**
         * Check if this morph target acts as phoneme or not.
         * A morph target is marked as phoneme if the phoneme set is set to something different than PHONEMESET_NONE.
         * On default, after constructing the morph target, the value is set to PHONEMESET_NONE, which means that on default
         * the morph target is not a phoneme.
         * @result Returns true if this morph target represents a phoneme, false is returned in the other case.
         */
        bool GetIsPhoneme() const;

        /**
         * Apply the relative deformations for this morph target to the given actor instance.
         * @param actorInstance The actor instance to apply the deformations on.
         * @param weight The absolute weight of the morph target.
         */
        virtual void Apply(ActorInstance* actorInstance, float weight) = 0;

        /**
         * Creates an exact clone of this  morph target.
         * @result Returns a pointer to an exact clone of this morph target.
         */
        virtual MorphTarget* Clone() const = 0;

        /**
         * Copy the morph target base class members over to another morph target.
         * This can be used when implementing your own Clone method for your own morph target.
         * @param target The morph target to copy the data from.
         */
        void CopyBaseClassMemberValues(MorphTarget* target) const;

        /**
         * Scale all transform and positional data.
         * This is a very slow operation and is used to convert between different unit systems (cm, meters, etc).
         * @param scaleFactor The scale factor to scale the current data by.
         */
        virtual void Scale(float scaleFactor) = 0;

    protected:
        uint32          m_nameId;        /**< The unique ID of the morph target, calculated from the name. */
        float           m_rangeMin;      /**< The minimum range of the weight. */
        float           m_rangeMax;      /**< The maximum range of the weight. */
        EPhonemeSet     m_phonemeSets;   /**< The phoneme sets in case this morph target is used as a phoneme. */

        /**
         * The constructor.
         * @param name The unique name of the morph target.
         */
        MorphTarget(const char* name);
    };
} // namespace EMotionFX
