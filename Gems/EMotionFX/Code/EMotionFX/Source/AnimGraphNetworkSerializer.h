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

namespace MCore
{
    class Attribute;
};

namespace EMotionFX
{
    class AnimGraphSnapshot;

    namespace Network
    {
        /*
         * Inherit AnimGraphSnapshotSerializer if you want to serialize the entire snapshot object.
         * 
         */
        class AnimGraphSnapshotSerializer
        {
        public:
            virtual ~AnimGraphSnapshotSerializer() = default;

            // Serialize happen on the server side snapshot.
            virtual void Serialize(const AnimGraphSnapshot& snapshot) = 0;

            // Deserialize happen on the client side snapshot.
            // For GridMate, this function only called once after client side snapshot connected to server.
            // After the connection, snapshot gets update per dataset through a callback function.
            virtual void Deserialize(AnimGraphSnapshot& snapshot) = 0;
        };

        /*
        * Inherit AnimGraphSnapshotSerializer if you want to serialize smaller chunk of snapshot data. 
        * 
        */
        class AnimGraphSnapshotChunkSerializer
        {
        public:
            virtual ~AnimGraphSnapshotChunkSerializer() = default;

            virtual void Serialize(bool& value, const char* context) = 0;
            virtual void Serialize(AZ::u32& value, const char* context) = 0;
            virtual void Serialize(AZ::s32& value, const char* context) = 0;
            virtual void Serialize(float& value, const char*  context) = 0;
            virtual void Serialize(AZStd::string& value, const char*  context) = 0;
            virtual void Serialize(AZ::Vector2& value, const char*  context) = 0;
            virtual void Serialize(AZ::Vector3& value, const char*  context) = 0;
            virtual void Serialize(AZ::Vector4& value, const char*  context) = 0;

            // Function to call the Network Serialize function on the attribute class.
            void Serialize(MCore::Attribute& attribute, const char* context);
        };
    }
}