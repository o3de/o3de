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
#include <AzCore/std/string/string.h>


namespace EMotionFX
{
    // forward declarations
    class Node;
    class Mesh;
    class Actor;

    /**
     * The vertex attribute layer base class.
     * Each mesh can have a set of custom vertex attribute layers.
     * Each layer must be inherited from this base class and must store its custom data, such as texture coordinates
     * or vertex color or softskinning information.
     */
    class EMFX_API VertexAttributeLayer
        : public MCore::RefCounted
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        /**
         * Get the unique layer type.
         * This identifies what type of attributes are stored internally.
         * An example could be the type ID of an UV attribute layer, or a layer with colors or one which
         * identifies a layer that contains softskinning information.
         * @result The unique type ID, which identifies what type of data is stored inside this layer. Each class inherited from
         *         the VertexAttributeLayer class requires a unique type ID.
         */
        virtual uint32 GetType() const = 0;

        /**
         * Get the description of the vertex attributes or layer.
         * You most likely want this to be the class name.
         * @result A pointer to the string containing the name or description of the type of vertex attributes of this layer.
         */
        virtual const char* GetTypeString() const = 0;

        /**
         * Clone the vertex attribute layer.
         * @result A clone of this layer.
         */
        virtual VertexAttributeLayer* Clone() = 0;

        /**
         * Get a pointer to the data. You have to typecast the data yourself.
         * @result A pointer to the vertex data.
         */
        virtual void* GetData()             { return nullptr; }

        /**
         * Get a pointer to the original data, as it is stored in the base pose, before any mesh deformers have been applied.
         * @result A pointer to the original vertex data.
         */
        virtual void* GetOriginalData()     { return nullptr; }

        /**
         * Get the number of attributes inside this layer.
         * @result The number of attributes.
         */
        MCORE_INLINE uint32 GetNumAttributes() const        { return m_numAttributes; }

        /**
         * Check if this class also stores original vertex data or not.
         * This will store twice as many attributes in memory and is used for vertex data that can be deformed, such
         * as positions, normals and tangents. Before applying deformations to the data returned by GetData() the
         * current vertex data returned by this method will be initialized to its original data as it was before any deformations.
         * The initialization to the original data happens inside the ResetToOriginalData method.
         * @result Returns true when this class also stores the original (undeformed) data, next to the current (deformed) data.
         */
        MCORE_INLINE bool GetKeepOriginals() const          { return m_keepOriginals; }

        /**
         * Reset the layer data to it's original data.
         * This is used to restore for example skinned data back into the data as it is in the base pose.
         * The mesh deformers will use this as a starting point then.
         */
        virtual void ResetToOriginalData() = 0;

        /**
         * Swap the data for two attributes.
         * You specify two attribute numbers, the data for them should be swapped.
         * This is used by the geometry LOD system and will be called by Mesh::SwapVertex as well.
         * @param attribA The first attribute number.
         * @param attribB The second attribute number.
         */
        virtual void SwapAttributes(uint32 attribA, uint32 attribB) = 0;

        /**
         * Remove a range of attributes.
         * @param startAttributeNr The start attribute number.
         * @param endAttributeNr The end attribute number, which will also be removed.
         */
        virtual void RemoveAttributes(uint32 startAttributeNr, uint32 endAttributeNr) = 0;

        /**
         * Returns true when this is the VertexAttributeLayerAbstractData class.
         * On default this returns false and you don't have to overload this. It is just an internal thing that was needed
         * to make the whole abstract data idea work.
         * @result Returns true when this is the VertexAttributeLayerAbstractData class. Otherwise false is returned.
         */
        virtual bool GetIsAbstractDataClass() const;

        /**
         * Scale all vertex data (should scale positional data and maybe more).
         * This is a slow operation and is used to convert between different unit systems (cm, meters, etc).
         * @param scaleFactor The scale factor to scale the current data by.
         */
        virtual void Scale(float scaleFactor)   { MCORE_UNUSED(scaleFactor); }

        void SetName(const char* name);
        const char* GetName() const;
        const AZStd::string& GetNameString() const;
        uint32 GetNameID() const;


    protected:
        uint32  m_numAttributes;     /**< The number of attributes inside this layer. */
        uint32  m_nameId;            /**< The name ID. */
        bool    m_keepOriginals;     /**< Should we store a copy of the original data as well? */

        /**
         * The constructor.
         * @param numAttributes The number of attributes to store inside this layer.
         * @param keepOriginals Specifies whether a copy of the original data should be stored as well. The current data values wil then be restored
         *                      to their original values every frame, before passing them through any mesh deformers.
         */
        VertexAttributeLayer(uint32 numAttributes, bool keepOriginals = false);

        /**
         * The destructor, which should delete all allocated attributes from memory.
         */
        virtual ~VertexAttributeLayer();
    };
} // namespace EMotionFX
