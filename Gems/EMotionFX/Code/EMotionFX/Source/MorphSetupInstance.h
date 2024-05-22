/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "EMotionFXConfig.h"
#include <MCore/Source/RefCounted.h>
#include <AzCore/std/containers/vector.h>


namespace EMotionFX
{
    // forward declarations
    class MorphSetup;

    /**
     * The  morph setup instance.
     * This contains all unique information per actor instance, which for example allows each actor instance to have different
     * weight values per morph target, while still the real morph targets are being shared between all actor instances.
     */
    class EMFX_API MorphSetupInstance
        : public MCore::RefCounted
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        /**
         * The unique settings per morph target for each actor instance.
         * These values are used in all geometry LOD levels.
         */
        class EMFX_API MorphTarget
        {
        public:
            /**
             * The constructor.
             */
            MorphTarget()
                : m_id(MCORE_INVALIDINDEX32)
                , m_weight(0.0f)
                , m_isInManualMode(false) {}

            /**
             * The destructor.
             */
            ~MorphTarget() {}

            /**
             * Get the ID of this morph target. This is based on its name.
             * This ID links the MorphTarget class with this local morph target class.
             * @result The ID of this morph target.
             */
            MCORE_INLINE uint32 GetID() const                       { return m_id; }

            /**
             * Get the weight value of the morph target.
             * @result The weight value.
             */
            float GetWeight() const                                 { return m_weight; }

            /**
             * Check if we are in manual mode or not.
             * Manual mode means that we can manually control the weight value. If this is disabled
             * then the motion system will overwrite the weight values.
             * @result Returns true when we are in manual mode, otherwise false is returned.
             */
            bool GetIsInManualMode() const                          { return m_isInManualMode; }

            /**
             * Set the ID of this morph target.
             * This ID links the MorphTarget class with this local morph target class.
             * @param id The ID to use.
             */
            void SetID(uint32 id)                                   { m_id = id; }

            /**
             * Set the weight value of the morph target.
             * @param weight The weight value.
             */
            void SetWeight(float weight)                            { m_weight = weight; }

            /**
             * Enable or disable manual mode.
             * Manual mode means that we can manually control the weight value. If this is disabled
             * then the motion system will overwrite the weight values.
             * @param enabled Set to true if you wish to enable manual mode on this morph target. Otherwise set to false.
             */
            void SetManualMode(bool enabled)                        { m_isInManualMode = enabled; }

        private:
            uint32  m_id;                    /**< The ID, which is based on the weight. */
            float   m_weight;                /**< The weight for this morph target. */
            bool    m_isInManualMode;        /**< The flag if we are in manual weight update mode or not. */
        };


        /**
         * The default creation method.
         * Be sure to call the Init method later on or use the extended constructor.
         */
        static MorphSetupInstance* Create();

        /**
         * The extended creation method.
         * This automatically calls the Init method.
         */
        static MorphSetupInstance* Create(MorphSetup* morphSetup);

        /**
         * Initialize the morph setup instance and link it to a given morph setup.
         * This will automatically create the morph targets locally stored inside this class.
         * @param morphSetup The morph setup to initialize from.
         */
        void Init(MorphSetup* morphSetup);

        /**
         * Get the number of morph targets inside this class.
         * This should always be equal to the number of morph targets in the highest detail.
         * @result The number of morph targets.
         */
        MCORE_INLINE size_t GetNumMorphTargets() const                      { return m_morphTargets.size(); }

        /**
         * Get a specific morph target.
         * @param nr The morph target number, which must be in range of [0..GetNumMorphTargets()-1].
         * @result A pointer to the morph target inside this class.
         */
        MCORE_INLINE MorphTarget* GetMorphTarget(size_t nr)                 { return &m_morphTargets[nr]; }

        MCORE_INLINE const MorphTarget* GetMorphTarget(size_t nr) const     { return &m_morphTargets[nr]; }

        /**
         * Find a given morph target number by its ID.
         * @param id The ID value to search for.
         * @result Returns the morph target number in range of [0..GetNumMorphTargets()-1], or InvalidIndex when not found.
         */
        size_t FindMorphTargetIndexByID(uint32 id) const;

        /**
         * Find the morph target by its ID.
         * @param id The ID value of the morph target to search for.
         * @result Returns a pointer to the morph target with the given ID, or nullptr when not found.
         */
        MorphTarget* FindMorphTargetByID(uint32 id);

    private:
        AZStd::vector<MorphTarget>   m_morphTargets;  /**< The unique morph target information. */

        /**
         * The default constructor.
         * Be sure to call the Init method later on or use the extended constructor.
         */
        MorphSetupInstance();

        /**
         * The extended constructor.
         * This automatically calls the Init method.
         */
        MorphSetupInstance(MorphSetup* morphSetup);

        /**
         * The destructor.
         */
        ~MorphSetupInstance();
    };
} // namespace EMotionFX
