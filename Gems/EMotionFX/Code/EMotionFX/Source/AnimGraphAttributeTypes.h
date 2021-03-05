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

#include "EMotionFXConfig.h"
#include "ActorInstance.h"
#include "Actor.h"
#include "Pose.h"
#include "AnimGraphPose.h"

#include <MCore/Source/StringIdPool.h>
#include <MCore/Source/Attribute.h>
#include <MCore/Source/AttributeFloat.h>
#include <MCore/Source/AttributeInt32.h>
#include <MCore/Source/AttributeString.h>
#include <MCore/Source/AttributeBool.h>
#include <MCore/Source/AttributeVector2.h>
#include <MCore/Source/AttributeVector3.h>
#include <MCore/Source/AttributeVector4.h>
#include <MCore/Source/AttributeQuaternion.h>
#include <MCore/Source/AttributeColor.h>
#include <MCore/Source/StringConversions.h>


namespace EMotionFX
{
    // forward declaration
    class AnimGraph;
    class AnimGraphNode;

    class EMFX_API AttributePose
        : public MCore::Attribute
    {
        AZ_CLASS_ALLOCATOR_DECL

        friend class AnimGraphManager;
    public:
        enum
        {
            TYPE_ID = 0x00001001
        };

        static AttributePose* Create();
        static AttributePose* Create(AnimGraphPose* pose);

        void SetValue(AnimGraphPose* value)                                { mValue = value; }
        AnimGraphPose* GetValue() const                                    { return mValue; }
        AnimGraphPose* GetValue()                                          { return mValue; }

        // overloaded from the attribute base class
        MCore::Attribute* Clone() const override                            { return Create(mValue); }
        const char* GetTypeString() const override                          { return "Pose"; }
        bool InitFrom(const MCore::Attribute* other) override
        {
            if (other->GetType() != TYPE_ID)
            {
                return false;
            }
            const AttributePose* pose = static_cast<const AttributePose*>(other);
            mValue = pose->GetValue();
            return true;
        }
        bool InitFromString(const AZStd::string& valueString) override      { MCORE_UNUSED(valueString); return false; }    // unsupported
        bool ConvertToString(AZStd::string& outString) const override       { MCORE_UNUSED(outString); return false; }  // unsupported
        uint32 GetClassSize() const override                                { return sizeof(AttributePose); }
        uint32 GetDefaultInterfaceType() const override                     { return MCore::ATTRIBUTE_INTERFACETYPE_DEFAULT; }

    private:
        AnimGraphPose* mValue;

        AttributePose()
            : MCore::Attribute(TYPE_ID)     { mValue = nullptr; }
        AttributePose(AnimGraphPose* pose)
            : MCore::Attribute(TYPE_ID)     { mValue = pose; }
        ~AttributePose() {}

        uint32 GetDataSize() const override                                 { return 0; }
        bool ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version) override   { MCORE_UNUSED(stream); MCORE_UNUSED(streamEndianType); MCORE_UNUSED(version); return false; }  // unsupported
    };


    class EMFX_API AttributeMotionInstance
        : public MCore::Attribute
    {
        AZ_CLASS_ALLOCATOR_DECL

        friend class AnimGraphManager;
    public:
        enum
        {
            TYPE_ID = 0x00001002
        };

        static AttributeMotionInstance* Create();
        static AttributeMotionInstance* Create(MotionInstance* motionInstance);

        void SetValue(MotionInstance* value)                                { mValue = value; }
        MotionInstance* GetValue() const                                    { return mValue; }
        MotionInstance* GetValue()                                          { return mValue; }

        // overloaded from the attribute base class
        MCore::Attribute* Clone() const override                            { return Create(mValue); }
        const char* GetTypeString() const override                          { return "MotionInstance"; }
        bool InitFrom(const MCore::Attribute* other) override
        {
            if (other->GetType() != TYPE_ID)
            {
                return false;
            }
            const AttributeMotionInstance* pose = static_cast<const AttributeMotionInstance*>(other);
            mValue = pose->GetValue();
            return true;
        }
        bool InitFromString(const AZStd::string& valueString) override      { MCORE_UNUSED(valueString); return false; }    // unsupported
        bool ConvertToString(AZStd::string& outString) const override       { MCORE_UNUSED(outString); return false; }  // unsupported
        uint32 GetClassSize() const override                                { return sizeof(AttributeMotionInstance); }
        uint32 GetDefaultInterfaceType() const override                     { return MCore::ATTRIBUTE_INTERFACETYPE_DEFAULT; }

    private:
        MotionInstance* mValue;

        AttributeMotionInstance()
            : MCore::Attribute(TYPE_ID)     { mValue = nullptr; }
        AttributeMotionInstance(MotionInstance* motionInstance)
            : MCore::Attribute(TYPE_ID)     { mValue = motionInstance; }
        ~AttributeMotionInstance() {}

        uint32 GetDataSize() const override                                 { return 0; }
        bool ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version) override   { MCORE_UNUSED(stream); MCORE_UNUSED(streamEndianType); MCORE_UNUSED(version); return false; }  // unsupported
    };

    class AnimGraphPropertyUtils
    {
    public:
        static void ReinitJointIndices(const Actor* actor, const AZStd::vector<AZStd::string>& jointNames, AZStd::vector<AZ::u32>& outJointIndices);
    };
    
} // namespace EMotionFX