/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

        void SetValue(AnimGraphPose* value)                                { m_value = value; }
        AnimGraphPose* GetValue() const                                    { return m_value; }
        AnimGraphPose* GetValue()                                          { return m_value; }

        // overloaded from the attribute base class
        MCore::Attribute* Clone() const override                            { return Create(m_value); }
        const char* GetTypeString() const override                          { return "Pose"; }
        bool InitFrom(const MCore::Attribute* other) override
        {
            if (other->GetType() != TYPE_ID)
            {
                return false;
            }
            const AttributePose* pose = static_cast<const AttributePose*>(other);
            m_value = pose->GetValue();
            return true;
        }
        bool InitFromString(const AZStd::string& valueString) override      { MCORE_UNUSED(valueString); return false; }    // unsupported
        bool ConvertToString(AZStd::string& outString) const override       { MCORE_UNUSED(outString); return false; }  // unsupported
        size_t GetClassSize() const override                                { return sizeof(AttributePose); }
        AZ::u32 GetDefaultInterfaceType() const override                     { return MCore::ATTRIBUTE_INTERFACETYPE_DEFAULT; }

    private:
        AnimGraphPose* m_value;

        AttributePose()
            : MCore::Attribute(TYPE_ID)     { m_value = nullptr; }
        AttributePose(AnimGraphPose* pose)
            : MCore::Attribute(TYPE_ID)     { m_value = pose; }
        ~AttributePose() {}
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

        void SetValue(MotionInstance* value)                                { m_value = value; }
        MotionInstance* GetValue() const                                    { return m_value; }
        MotionInstance* GetValue()                                          { return m_value; }

        // overloaded from the attribute base class
        MCore::Attribute* Clone() const override                            { return Create(m_value); }
        const char* GetTypeString() const override                          { return "MotionInstance"; }
        bool InitFrom(const MCore::Attribute* other) override
        {
            if (other->GetType() != TYPE_ID)
            {
                return false;
            }
            const AttributeMotionInstance* pose = static_cast<const AttributeMotionInstance*>(other);
            m_value = pose->GetValue();
            return true;
        }
        bool InitFromString(const AZStd::string& valueString) override      { MCORE_UNUSED(valueString); return false; }    // unsupported
        bool ConvertToString(AZStd::string& outString) const override       { MCORE_UNUSED(outString); return false; }  // unsupported
        size_t GetClassSize() const override                                { return sizeof(AttributeMotionInstance); }
        AZ::u32 GetDefaultInterfaceType() const override                     { return MCore::ATTRIBUTE_INTERFACETYPE_DEFAULT; }

    private:
        MotionInstance* m_value;

        AttributeMotionInstance()
            : MCore::Attribute(TYPE_ID)     { m_value = nullptr; }
        AttributeMotionInstance(MotionInstance* motionInstance)
            : MCore::Attribute(TYPE_ID)     { m_value = motionInstance; }
        ~AttributeMotionInstance() {}
    };

    class AnimGraphPropertyUtils
    {
    public:
        static void ReinitJointIndices(const Actor* actor, const AZStd::vector<AZStd::string>& jointNames, AZStd::vector<size_t>& outJointIndices);
    };
    
} // namespace EMotionFX
