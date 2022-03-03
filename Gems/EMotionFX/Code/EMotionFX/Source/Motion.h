/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <MCore/Source/StringIdPool.h>
#include <MCore/Source/Distance.h>
#include "EMotionFXConfig.h"
#include "EMotionFXManager.h"
#include "PlayBackInfo.h"
#include "BaseObject.h"
#include <EMotionFX/Source/MotionData/MotionDataSampleSettings.h>

namespace EMotionFX
{
    // forward declarations
    class Node;
    class Actor;
    class MotionInstance;
    class Pose;
    class Transform;
    class MotionEventTable;
    class MotionData;

    /**
     * The motion base class.
     * The unified motion processing system requires all motions to have a base class.
     * This base class is the Motion class (so this one). Different types of motions can be for example
     * skeletal motions (body motions) or facial motions. The main function inside this base class is the method
     * named Update, which will output the resulting transformations into a Pose object.
     */
    class EMFX_API Motion
        : public BaseObject
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(Motion, "{CCC21150-37F5-477A-9EBF-B5E71C0B5D71}", BaseObject)

        Motion(const char* name);
        virtual ~Motion();

        /**
         * Set the name of the motion.
         * @param name The name of the motion.
         */
        void SetName(const char* name);

        /**
         * Returns the name of the motion.
         * @result The name of the motion.
         */
        const char* GetName() const;

        /**
         * Returns the name of the motion, as a AZStd::string object.
         * @result The name of the motion.
         */
        const AZStd::string& GetNameString() const;

        /**
         * Set the filename of the motion.
         * @param[in] filename The filename of the motion.
         */
        void SetFileName(const char* filename);

        /**
         * Get the filename of the motion.
         * @result The filename of the motion.
         */
        const char* GetFileName() const;

        /**
         * Returns the filename of the motion, as a AZStd::string object.
         * @result The filename of the motion.
         */
        const AZStd::string& GetFileNameString() const;

        /**
         * Set the unique identification number for the motion.
         * @param[in] id The unique identification number.
         */
        void SetID(uint32 id);

        /**
         * Get the unique identification number for the motion.
         * @return The unique identification number.
         */
        uint32 GetID() const;

        /**
         * Calculates the node transformation of the given node for this motion.
         * @param instance The motion instance that contains the motion links to this motion.
         * @param outTransform The node transformation that will be the output of this function.
         * @param actor The actor to apply the motion to.
         * @param node The node to apply the motion to.
         * @param timeValue The time value.
         * @param enableRetargeting Set to true if you like to enable motion retargeting, otherwise set to false.
         */
        void CalcNodeTransform(const MotionInstance* instance, Transform* outTransform, Actor* actor, Node* node, float timeValue, bool enableRetargeting);

        /**
         * Get the event table.
         * This event table stores all motion events and can execute them as well.
         * @result The event table, which stores the motion events.
         */
        MotionEventTable* GetEventTable() const;

        /**
         * Set the event table.
         * @param newTable The new motion event table for the Motion to use.
         */
        void SetEventTable(AZStd::unique_ptr<MotionEventTable> eventTable);

        /**
         * Set the motion framerate.
         * @param motionFPS The number of keyframes per second.
         */
        void SetMotionFPS(float motionFPS);

        /**
         * Get the motion framerate.
         * @return The number of keyframes per second.
         */
        float GetMotionFPS() const;

        /**
         * The main update method, which outputs the result for a given motion instance into a given output local pose.
         * @param inputPose The current pose, as it is currently in the blending pipeline.
         * @param outputPose The output pose, which this motion will modify and write its output into.
         * @param instance The motion instance to calculate the pose for.
         */
        void Update(const Pose* inputPose, Pose* outputPose, MotionInstance* instance);

        void SamplePose(Pose* outputPose, const MotionDataSampleSettings& sampleSettings);

        /**
         * Specify the actor to use as retargeting source.
         * This would be the actor from which the motion was originally exported.
         * So if you would play a human motion on a dwarf, you would pass the actor that represents the human as parameter.
         * @param actor The actor to use as retarget source.
         */
        virtual void SetRetargetSource(Actor* actor)                            { MCORE_UNUSED(actor); }

        /**
         * Set the pointer to some custom data that you'd like to associate with this motion object.
         * Please keep in mind you are responsible yourself for deleting any allocations you might do
         * inside this custom data object. You can use the motion event system to detect when a motion gets
         * deleted to help you find out when to delete allocated custom data.
         * @param dataPointer The pointer to your custom data. This can also be nullptr.
         */
        void SetCustomData(void* dataPointer);

        /**
         * Get the pointer to some custom data that you'd like to associate with this motion object.
         * Please keep in mind you are responsible yourself for deleting any allocations you might do
         * inside this custom data object. You can use the motion event system to detect when a motion gets
         * deleted to help you find out when to delete allocated custom data.
         * @result A pointer to the custom data linked with this motion object, or nullptr when it has not been set.
         */
        void* GetCustomData() const;

        /**
         * Set the default playback info to the given playback info. In case the default playback info hasn't been created yet this function will automatically take care of that.
         * @param playBackInfo The new playback info which will be copied over to this motion's default playback info.
         */
        void SetDefaultPlayBackInfo(const PlayBackInfo& playBackInfo);

        /**
         * Get the default playback info of this motion.
         */
        PlayBackInfo* GetDefaultPlayBackInfo();
        const PlayBackInfo* GetDefaultPlayBackInfo() const;

        /**
         * Get the motion extraction flags.
         * @result The motion extraction flags, which can be a combination of the values inside the specific enum.
         */
        EMotionExtractionFlags GetMotionExtractionFlags() const;

        /**
         * Set the motion extraction flags.
         * @param mask The motion extraction flags to set.
         */
        void SetMotionExtractionFlags(EMotionExtractionFlags flags);

        /**
         * Set the dirty flag which indicates whether the user has made changes to the motion. This indicator should be set to true
         * when the user changed something like adding a motion event. When the user saves the motion, the indicator is usually set to false.
         * @param dirty The dirty flag.
         */
        void SetDirtyFlag(bool dirty);

        /**
         * Get the dirty flag which indicates whether the user has made changes to the motion. This indicator is set to true
         * when the user changed something like adding a motion event. When the user saves the motion, the indicator is usually set to false.
         * @return The dirty flag.
         */
        bool GetDirtyFlag() const;

        /**
         * Set if we want to automatically unregister this motion from the motion manager when we delete this motion.
         * On default this is set to true.
         * @param enabled Set to true when you wish to automatically have the motion unregistered, otherwise set it to false.
         */
        void SetAutoUnregister(bool enabled);

        /**
         * Check if this motion is automatically being unregistered from the motion manager when this motion gets deleted or not.
         * @result Returns true when it will get automatically deleted, otherwise false is returned.
         */
        bool GetAutoUnregister() const;

        /**
         * Marks the object as used by the engine runtime, as opposed to the tool suite.
         */
        void SetIsOwnedByRuntime(bool isOwnedByRuntime);
        bool GetIsOwnedByRuntime() const;

        void SetUnitType(MCore::Distance::EUnitType unitType);
        MCore::Distance::EUnitType GetUnitType() const;

        void SetFileUnitType(MCore::Distance::EUnitType unitType);
        MCore::Distance::EUnitType GetFileUnitType() const;

        /**
         * Scale all motion data.
         * This is a very slow operation and is used to convert between different unit systems (cm, meters, etc).
         * @param scaleFactor The scale factor to scale the current data by.
         */
        void Scale(float scaleFactor);

        /**
         * Scale to a given unit type.
         * This method does nothing if the motion is already in this unit type.
         * You can check what the current unit type is with the GetUnitType() method.
         * @param targetUnitType The unit type to scale into (meters, centimeters, etc).
         */
        void ScaleToUnitType(MCore::Distance::EUnitType targetUnitType);

        void UpdateDuration();
        float GetDuration() const;

        const MotionData* GetMotionData() const;
        MotionData* GetMotionData();
        void SetMotionData(MotionData* motionData, bool delOldFromMem=true);

    protected:
        MotionData*                 m_motionData = nullptr; /**< The motion data, which can in theory be any data representation/compression. */
        AZStd::string               m_fileName;              /**< The filename of the motion. */
        PlayBackInfo                m_defaultPlayBackInfo;  /**< The default/fallback motion playback info which will be used when no playback info is passed to the Play() function. */
        AZStd::unique_ptr<MotionEventTable> m_eventTable;   /**< The event table, which contains all events, and will make sure events get executed. */
        MCore::Distance::EUnitType  m_unitType;              /**< The type of units used. */
        MCore::Distance::EUnitType  m_fileUnitType;          /**< The type of units used, inside the file that got loaded. */
        void*                       m_customData = nullptr; /**< A pointer to custom user data that is linked with this motion object. */
        float                       m_motionFps = 30.0f; /**< The number of keyframes per second. */
        uint32                      m_nameId = MCORE_INVALIDINDEX32; /**< The ID represention the name or description of this motion. */
        uint32                      m_id = MCORE_INVALIDINDEX32; /**< The unique identification number for the motion. */
        EMotionExtractionFlags      m_extractionFlags;       /**< The motion extraction flags, which define behavior of the motion extraction system when applied to this motion. */
        bool                        m_dirtyFlag = false; /**< The dirty flag which indicates whether the user has made changes to the motion since the last file save operation. */
        bool                        m_autoUnregister = true; /**< Automatically unregister the motion from the motion manager when this motion gets deleted? Default is true. */

#if defined(EMFX_DEVELOPMENT_BUILD)
        bool                        m_isOwnedByRuntime;
#endif // EMFX_DEVELOPMENT_BUILD
    };
} // namespace EMotionFX
