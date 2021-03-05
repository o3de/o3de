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

// include the required headers
#include "EMotionFXConfig.h"
#include "BaseObject.h"
#include "MorphTarget.h"


namespace EMotionFX
{
    /**
     * The  morph setup class.
     * This class contains a collection of  morph targets.
     * Each LOD in an Actor object can have a morph setup.
     */
    class EMFX_API MorphSetup
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        static MorphSetup* Create();

        /**
         * Pre-allocate space for a given amount of morph targets.
         * This does not influence the return value of GetNumMorphTargets().
         * @param numMorphTargets The number of morph targets to pre-allocate space for.
         */
        void ReserveMorphTargets(uint32 numMorphTargets);

        /**
         * Get the number of morph targets inside this  morph setup.
         * @result The number of morph targets.
         */
        MCORE_INLINE uint32 GetNumMorphTargets() const                      { return mMorphTargets.GetLength(); }

        /**
         * Get a given morph target.
         * @param nr The morph target number, must be in range of [0..GetNumMorphTargets()-1].
         * @result A pointer to the morph target.
         */
        MCORE_INLINE MorphTarget* GetMorphTarget(uint32 nr) const           { return mMorphTargets[nr]; }

        /**
         * Add a morph target to this  morph setup.
         * @param morphTarget The morph target to add.
         */
        void AddMorphTarget(MorphTarget* morphTarget);

        /**
         * Remove a given morph target, by index.
         * @param nr The morph target number to remove. This value must be in range of [0..GetNumMorphTargets()-1].
         * @param delFromMem When set to true, the morph target will be deleted from memory as well. When false, it will
         *                   only be removed from the array of morph targets inside this class.
         */
        void RemoveMorphTarget(uint32 nr, bool delFromMem = true);

        /**
         * Remove a given morph target.
         * @param morphTarget A pointer to the morph target to remove. This morph target must be part of this  morph setup.
         * @param delFromMem When set to true, the morph target will be deleted from memory as well. When false, it will
         *                   only be removed from the array of morph targets inside this class.
         */
        void RemoveMorphTarget(MorphTarget* morphTarget, bool delFromMem = true);

        /**
         * Remove all morph targets inside this setup.
         * NOTE: They will also be deleted from memory.
         */
        void RemoveAllMorphTargets();

        /**
         * Find a morph target by its unique ID, which has been calculated based on its name.
         * All morph targets with the same ID will also have the same name.
         * @param id The ID to search for.
         * @result A pointer to the morph target that has the specified ID, or nullptr when none could be found.
         */
        MorphTarget* FindMorphTargetByID(uint32 id) const;

        /**
         * Find a morph target index by its unique ID, which has been calculated based on its name.
         * All morph targets with the same ID will also have the same name.
         * @param id The ID to search for.
         * @result The morph target number, or MCORE_INVALIDINDEX32 when not found. You can use the returned number with the method
         *         GetMorphTarget(nr) in order to convert it into a direct pointer to the morph target.
         */
        uint32 FindMorphTargetNumberByID(uint32 id) const;

        /**
         * Find a morph target index by its name.
         * Please remember that this is case sensitive.
         * @result The index of the morph target that you can pass to GetMorphTarget(index).
         */
        uint32 FindMorphTargetIndexByName(const char* name) const;

        /**
         * Find a morph target index by its name.
         * Please remember that this is case insensitive.
         * @result The index of the morph target that you can pass to GetMorphTarget(index).
         */
        uint32 FindMorphTargetIndexByNameNoCase(const char* name) const;

        /**
         * Find a morph target by its name.
         * Please remember that this is case sensitive.
         * @result A pointer to the morph target, or nullptr when not found.
         */
        MorphTarget* FindMorphTargetByName(const char* name) const;

        /**
         * Find a morph target by its name.
         * Please remember that this is NOT case sensitive.
         * @result A pointer to the morph target, or nullptr when not found.
         */
        MorphTarget* FindMorphTargetByNameNoCase(const char* name) const;

        /**
         * Clone the morph setup, and return the clone.
         * @result Returns an exact clone of this morph setup.
         */
        MorphSetup* Clone() const;

        /**
         * Scale all transform and positional data.
         * This is a very slow operation and is used to convert between different unit systems (cm, meters, etc).
         * @param scaleFactor The scale factor to scale the current data by.
         */
        void Scale(float scaleFactor);


    protected:
        MCore::Array<MorphTarget*>  mMorphTargets;  /**< The collection of morph targets. */

        /**
         * The constructor.
         */
        MorphSetup();

        /**
         * The destructor. Automatically removes all morph targets from memory.
         */
        ~MorphSetup();
    };
} // namespace EMotionFX
