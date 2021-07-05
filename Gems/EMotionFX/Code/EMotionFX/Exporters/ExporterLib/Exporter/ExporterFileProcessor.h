/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <MCore/Source/Endian.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <MCore/Source/MemoryFile.h>
#include <EMotionFX/Source/BaseObject.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionSet.h>


namespace ExporterLib
{
    class Exporter
        : public EMotionFX::BaseObject
    {
        MCORE_MEMORYOBJECTCATEGORY(Exporter, EMFX_DEFAULT_ALIGNMENT, EMotionFX::EMFX_MEMCATEGORY_FILEPROCESSORS);

    public:
        static Exporter* Create();

        // actor
        bool SaveActor(MCore::MemoryFile* file, const EMotionFX::Actor* actor, MCore::Endian::EEndianType targetEndianType);
        bool SaveActor(AZStd::string filenameWithoutExtension, const EMotionFX::Actor* actor, MCore::Endian::EEndianType targetEndianType);

        // motion
        bool SaveMotion(MCore::MemoryFile* file, EMotionFX::Motion* motion, MCore::Endian::EEndianType targetEndianType);
        bool SaveMotion(AZStd::string filenameWithoutExtension, EMotionFX::Motion* motion, MCore::Endian::EEndianType targetEndianType);

    private:
        void ResetMemoryFile(MCore::MemoryFile* file);

        Exporter();
        virtual ~Exporter();

        void Delete() override;
    };
} // namespace ExporterLib
