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
//#include "Mesh.h"
//#include "SoftSkinDeformer.h"


namespace EMotionFX
{
    // forward declarations
    class Actor;
    class Mesh;
    class SoftSkinDeformer;

    /**
     * The softskin manager.
     * This manager allows to create optimized softskin deformers, which will run as fast as possible
     * on the hardware of the user. In other words, specialised version of softskin deformers can be
     * created using this class. For example: if the hardware supports SSE, an SSE optimized softskin deformer
     * will be returned, instead of the normal C++ version.
     */
    class EMFX_API SoftSkinManager
        : public MCore::RefCounted

    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        /**
         * The constructor.
         * When constructed, the class checks if SSE is available on the hardware.
         */
        static SoftSkinManager* Create();

        /**
         * Creates the softskin deformer, by looking at the hardware capabilities.
         * If SSE is present, an SSE optimized softskinner will be returned, otherwise a normal
         * C++ version.
         * @param mesh The mesh that will be deformed.
         * @result A pointer to the created skin deformer.
         */
        SoftSkinDeformer* CreateDeformer(Mesh* mesh);

    private:
        /**
         * The constructor.
         * When constructed, the class checks if SSE is available on the hardware.
         */
        SoftSkinManager();
    };
} // namespace EMotionFX
