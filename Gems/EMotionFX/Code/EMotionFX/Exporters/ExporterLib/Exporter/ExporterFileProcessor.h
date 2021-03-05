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
