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
#include <EMotionFX/CommandSystem/Source/CommandSystemConfig.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace EMotionFX
{
    class Motion;
    class Actor;
};

namespace MCore
{
    class Command;
}

namespace CommandSystem
{

    class COMMANDSYSTEM_API MetaData
    {
    public:
        /**
         * Constructs a list of commands representing the changes the user did on the source asset and returns it as a string.
         * @param motion The motion to read the changes from.
         * @result A string containing a list of commands.
         */
        static AZStd::vector<MCore::Command*> GenerateMotionMetaData(EMotionFX::Motion* motion);

        /**
         * Use the given list , prepare it for the given motion and apply the meta data.
         * @param motion The motion to apply the meta data on.
         * @result True in case the meta data got applied correctly, false if something failed.
         */
        static bool ApplyMetaDataOnMotion(EMotionFX::Motion* motion, const AZStd::vector<MCore::Command*>& metaDataCommands);

        /**
         * Constructs a list of commands representing the changes the user did on the source asset and returns it as a string.
         * @param actor The actor to read the changes from.
         * @result A string containing a list of commands.
         */
        static AZStd::string GenerateActorMetaData(EMotionFX::Actor* actor);

        /**
         * Use the given meta data string, prepare it for the given actor and apply the meta data.
         * @param actor The actor to apply the meta data on.
         * @result True in case the meta data got applied correctly, false if something failed.
         */
        static bool ApplyMetaDataOnActor(EMotionFX::Actor* actor, const AZStd::string& metaDataString);

    private:
        /**
         * Use the given meta data string, prepare it for the given runtime object and apply the meta data.
         * @param objectId The runtime object id (e.g. motion->GetID() or actor->GetID()).
         * @param objectIdKeyword The placeholder for the runtime object id inside the meta data (e.g. $(MOTIONID)).
         * @param metaDataString The meta data as string.
         * @result True in case the meta data got applied correctly, false if something failed.
         */
        static bool ApplyMetaData(AZ::u32 objectId, const char* objectIdKeyword, AZStd::string metaDataString);


        /**
         * Execute the list of Commands that have been deserialized from a MetaDataRule
         */
        static bool ApplyMetaData(const AZStd::vector<MCore::Command*>& metaDataCommands);

        static void GenerateNodeGroupMetaData(EMotionFX::Actor* actor, AZStd::string& outMetaDataString);
        static void GeneratePhonemeMetaData(EMotionFX::Actor* actor, AZStd::string& outMetaDataString);
        static void GenerateAttachmentMetaData(EMotionFX::Actor* actor, AZStd::string& outMetaDataString);
        static void GenerateMotionExtractionMetaData(EMotionFX::Actor* actor, AZStd::string& outMetaDataString);
        static void GenerateRetargetRootMetaData(EMotionFX::Actor* actor, AZStd::string& outMetaDataString);
        static void GenerateMirrorSetupMetaData(EMotionFX::Actor* actor, AZStd::string& outMetaDataString);
    };

} // namespace CommandSystemlt
