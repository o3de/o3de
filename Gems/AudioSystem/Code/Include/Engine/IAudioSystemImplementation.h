/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <ATLEntityData.h>
#include <IAudioInterfacesCommonData.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/XML/rapidxml.h>


namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //! Notifications about the audio system for it and others to respond to.
    //! These notifications are sent from various places in the code for global events like gaining
    //! and losing application focus, mute and unmute, etc.
    class AudioSystemImplementationNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~AudioSystemImplementationNotifications() = default;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides - Single Bus, Multiple Handlers
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        ///////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! This method is called every time the main Game (or Editor) application loses focus.
        virtual void OnAudioSystemLoseFocus() = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! This method is called every time the main Game (or Editor) application receives focus.
        virtual void OnAudioSystemGetFocus() = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! This method is called when the audio output has been muted.
        //! After this call there should be no audio coming from the audio middleware.
        virtual void OnAudioSystemMuteAll() = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! This method is called when the audio output has been unmuted.
        virtual void OnAudioSystemUnmuteAll() = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! This method is called when user initiates a reload/refresh of all the audio data.
        virtual void OnAudioSystemRefresh() = 0;
    };

    using AudioSystemImplementationNotificationBus = AZ::EBus<AudioSystemImplementationNotifications>;


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //! Requests interface for audio middleware implementations.
    //! This is the main interface for interacting with an audio middleware implementation, creating
    //! and destroying objects, event handling, parameter setting, etc.
    class AudioSystemImplementationRequests
        : public AZ::EBusTraits
    {
    public:
        virtual ~AudioSystemImplementationRequests() = default;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides - Single Bus, Single Handler
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        ///////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Update the audio middleware implementation.
        //! Updates all of the internal sub-systems that require regular updates, and pumps the audio
        //! middleware api.
        //! @param updateIntervalMS Time since the last call to Update in milliseconds.
        virtual void Update(float updateIntervalMS) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Initialize all internal components of the audio middleware implementation.
        //! @return eARS_SUCCESS if the initialization was successful, eARS_FAILURE otherwise.
        virtual EAudioRequestStatus Initialize() = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Shuts down all of the internal components of the audio middleware implementation.
        //! After calling ShutDown the system can still be brought back up by calling Initialize.
        //! @return eARS_SUCCESS if the shutdown was successful, eARS_FAILURE otherwise.
        virtual EAudioRequestStatus ShutDown() = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Frees all of the resources used by the audio middleware implementation and destroys it.
        //! This action is not reversible.
        //! @return eARS_SUCCESS if the action was successful, eARS_FAILURE otherwise.
        virtual EAudioRequestStatus Release() = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Stops all currently playing sounds.
        //! Has no effect on anything triggered after this method is called.
        //! @return eARS_SUCCESS if the action was successful, eARS_FAILURE otherwise.
        virtual EAudioRequestStatus StopAllSounds() = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Register an audio object with the audio middleware.
        //! An object needs to be registered in order to set position, execute triggers on it,
        //! or set parameters and switches.
        //! @prarm objectData Implementation-specific audio object data.
        //! @param objectName The name of the audio object to be shown in debug info.
        //! @return eARS_SUCCESS if the object was registered, eARS_FAILURE otherwise.
        virtual EAudioRequestStatus RegisterAudioObject(IATLAudioObjectData* objectData, const char* objectName = nullptr) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Unregister an audio object with the audio middleware.
        //! After this action, executing triggers, setting position, states, or rtpcs no longer have
        //! an effect on the audio object.
        //! @prarm objectData Implementation-specific audio object data
        //! @return eARS_SUCCESS if the object was unregistered, eARS_FAILURE otherwise.
        virtual EAudioRequestStatus UnregisterAudioObject(IATLAudioObjectData* objectData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Clear out the audio object's internal state and reset it.
        //! After this action, the object can be recycled back to the pool of available audio objects.
        //! @param objectData Implementation-specific audio object data.
        //! @return eARS_SUCCESS if the object was reset, eARS_FAILURE otherwise.
        virtual EAudioRequestStatus ResetAudioObject(IATLAudioObjectData* objectData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Performs actions that need to be executed regularly on an audio object.
        //! @param objectData Implementation-specific audio object data.
        //! @return eARS_SUCCESS if the object was updated, eARS_FAILURE otherwise.
        virtual EAudioRequestStatus UpdateAudioObject(IATLAudioObjectData* objectData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Prepare a trigger synchronously for execution.
        //! Loads any metadata and media needed by the audio middleware to execute the trigger.
        //! @param objectData Implementation-specific audio object data.
        //! @param triggerData Implementation-specific trigger data.
        //! @return eARS_SUCCESS if the the trigger was successfully prepared, eARS_FAILURE otherwise.
        virtual EAudioRequestStatus PrepareTriggerSync(
            IATLAudioObjectData* audioObjectData,
            const IATLTriggerImplData* triggerData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Unprepare a trigger synchronously when no longer needed.
        //! The metadata and media associated with the trigger are released.
        //! @param objectData Implementation-specific audio object data.
        //! @param triggerData Implementation-specific trigger data.
        //! @return eARS_SUCCESS if the trigger data was successfully unloaded, eARS_FAILURE otherwise.
        virtual EAudioRequestStatus UnprepareTriggerSync(
            IATLAudioObjectData* objectData,
            const IATLTriggerImplData* triggerData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Prepare a trigger asynchronously for execution.
        //! Loads any metadata and media needed by the audio middleware to execute the trigger.
        //! An event that references eventData is created on the audio object.  The prepare event
        //! callback is called once the loading is done and the trigger is now prepared.
        //! @param objectData Implementation-specific audio object data.
        //! @param triggerData Implementation-specific trigger data.
        //! @param eventData Implementation-specific event data.
        //!     Used to manage the prepare event.
        //! @return eARS_SUCCESS if the trigger prepare event was successfully sent to the audio
        //!     middleware, eARS_FAILURE otherwise.
        virtual EAudioRequestStatus PrepareTriggerAsync(
            IATLAudioObjectData* objectData,
            const IATLTriggerImplData* triggerData,
            IATLEventData* eventData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Unprepare a trigger asynchronously when no longer needed.
        //! The metadata and media associated with the trigger are released.
        //! An event that references eventData is created on the audio object.  The unprepare event
        //! callback is called once the unloading is done and the trigger is unprepared.
        //! @param objectData Implementation-specific audio object data.
        //! @param triggerData Implementation-specific trigger data.
        //! @param eventData Implementation-specific event data.
        //! @return eARS_SUCCESS if the trigger unprepare event was successfully sent to the audio
        //!     middleware, eARS_FAILURE otherwise.
        virtual EAudioRequestStatus UnprepareTriggerAsync(
            IATLAudioObjectData* pAudioObjectData,
            const IATLTriggerImplData* pTriggerData,
            IATLEventData* pEventData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Activate a trigger on an audio object.
        //! @param objectData Implementation-specific audio object data.
        //! @param triggerData Implementation-specific trigger data.
        //! @param eventData Implementation-specific event data.
        //! @return eARS_SUCCESS if the trigger was activated and the event posted to the audio
        //!     middleware, eARS_FAILURE otherwise.
        virtual EAudioRequestStatus ActivateTrigger(
            IATLAudioObjectData* objectData,
            const IATLTriggerImplData* triggerData,
            IATLEventData* tventData,
            const SATLSourceData* sourceData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Stop an event active on an audio object.
        //! @param objectData Implementation-specific audio object data.
        //! @param eventData Implementation-specific event data.
        //! @return eARS_SUCCESS if the event was successfully stopped, eARS_FAILURE otherwise.
        virtual EAudioRequestStatus StopEvent(
            IATLAudioObjectData* objectData,
            const IATLEventData* eventData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Stop all events currently active on an audio object.
        //! @param objectData Implementation-specific audio object data.
        //! @return eARS_SUCCESS if the events were successfully stopped, eARS_FAILURE otherwise.
        virtual EAudioRequestStatus StopAllEvents(
            IATLAudioObjectData* objectData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Set the world position of an audio object.
        //! @param objectData Implementation-specific audio object data.
        //! @param worldPosition The transform to set the audio object to.
        //! @return eARS_SUCCESS if the position was successfully set, eARS_FAILURE otherwise.
        virtual EAudioRequestStatus SetPosition(
            IATLAudioObjectData* objectData,
            const SATLWorldPosition& worldPosition) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Sets multiple world positions of an audio object.
        //! @param objectData Implementation-specific audio object data.
        //! @param multiPositions Position parameter object containing world positions.
        //! @return eARS_SUCCESS if the position's were successfully set, eARS_FAILURE otherwise.
        virtual EAudioRequestStatus SetMultiplePositions(
            IATLAudioObjectData* objectData,
            const MultiPositionParams& multiPositions) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Set an audio rtpc to the specified value on a given audio object.
        //! @param objectData Implementation-specific audio object data.
        //! @param rtpcData Implementation-specific audio rtpc data.
        //! @param value The value to be set, normally in the range [0.0, 1.0].
        //! @return eARS_SUCCESS if the rtpc value was set on the audio object, eARS_FAILURE otherwise.
        virtual EAudioRequestStatus SetRtpc(
            IATLAudioObjectData* objectData,
            const IATLRtpcImplData* rtpcData,
            float value) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Set the audio switchstate on a given audio object.
        //! @param objectData Implementation-specific audio object data.
        //! @param switchStateData Implementation-specific audio switchstate data.
        //! @return eARS_SUCCESS if the audio switchstate has been successfully set, eARS_FAILURE
        //!     otherwise.
        virtual EAudioRequestStatus SetSwitchState(
            IATLAudioObjectData* objectData,
            const IATLSwitchStateImplData* switchStateData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Set the Obstruction and Occlusion amounts on a given audio object.
        //! @param objectData Implementation-specific audio object data.
        //! @param obstruction The amount of obstruction associated with the audio object.
        //!     Obstruction describes direct sound path being blocked but other paths may exist.
        //! @param occlusion The amount of occlusion associated with the audio object.
        //!     Occlusion describes all paths being blocked, direct and environmental reflection paths.
        //! @return eARS_SUCCESS if the values were set, eARS_FAILURE otherwise.
        virtual EAudioRequestStatus SetObstructionOcclusion(
            IATLAudioObjectData* objectData,
            float obstruction,
            float occlusion) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Set the amount of an audio environment associated with an audio object.
        //! @param objectData Implementation-specific audio object data.
        //! @param environmentData Implementation-specific audio environment data.
        //! @param amount The float value to set, in the range [0.0, 1.0].
        //! @return eARS_SUCCESS if the environment amount was set, eARS_FAILURE otherwise.
        virtual EAudioRequestStatus SetEnvironment(
            IATLAudioObjectData* objectData,
            const IATLEnvironmentImplData* environmentData,
            float amount) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Set the world transform of an audio listener.
        //! @param listenerData Implementation-specific audio listener data.
        //! @param newPosition The transform to set the listener to.
        //! @return eARS_SUCCESS if the audio listener's world transform has been successfully set,
        //!     eARS_FAILURE otherwise.
        virtual EAudioRequestStatus SetListenerPosition(
            IATLListenerData* listenerData,
            const SATLWorldPosition& newPosition) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Resets the audio rtpc data to the default state for the provided audio object.
        //! @param objectData Implementation-specific audio object data.
        //! @param rtpcData Implementation-specific audio rtpc data.
        //! @return eARS_SUCCESS if the provided rtpc has been successfully reset, eARS_FAILURE
        //!     otherwise.
        virtual EAudioRequestStatus ResetRtpc(
            IATLAudioObjectData* objectData,
            const IATLRtpcImplData* rtpcData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Inform the audio middleware about the memory location of loaded audio data file.
        //! @param audioFileEntry ATL-specific information describing the in-memory file being
        //!     registered.
        //! @return eARS_SUCCESS if the audio middleware successfully registered the file, eARS_FAILURE
        //!     otherwise.
        virtual EAudioRequestStatus RegisterInMemoryFile(SATLAudioFileEntryInfo* audioFileEntry) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Inform the audio middleware that the memory containing the audio data file should no longer
        //! be used.
        //! @param audioFileEntry ATL-specific information describing the file being invalidated.
        //! @return eARS_SUCCESS if the audio middleware unregistered the file contents, eARS_FAILURE
        //!     otherwise.
        virtual EAudioRequestStatus UnregisterInMemoryFile(SATLAudioFileEntryInfo* audioFileEntry) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Parse the implementation-specific XML node that represents an audio file entry.
        //! Fill the fields of the struct with the data necessary to locate and store the file's
        //! contents in memory.
        //! @param audioFileEntryNode XML node corresponding to information about the file.
        //!     Assumes that strings are null-terminated (i.e. the xml_document has been
        //!     parsed without the 'parse_no_string_terminators' flag).
        //! @param fileEntryInfo Pointer to the struct containing the file entry information.
        //! @return eARS_SUCCESS if the XML node was parsed successfully, eARS_FAILURE otherwise.
        virtual EAudioRequestStatus ParseAudioFileEntry(
            const AZ::rapidxml::xml_node<char>* audioFileEntryNode,
            SATLAudioFileEntryInfo* fileEntryInfo) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Free the memory and resources of the supplied audio file entry data.
        //! @param oldAudioFileEntryData Implementation-specific audio file entry data.
        virtual void DeleteAudioFileEntryData(IATLAudioFileEntryData* oldAudioFileEntryData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Get the full path to the folder containing the file described by fileEntryInfo.
        //! @param fileEntryInfo ATL-specific information describing the file whose location is being
        //!     queried.
        //! @return A zero-terminated C-string containing the path to the file.
        virtual const char* const GetAudioFileLocation(SATLAudioFileEntryInfo* fileEntryInfo) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Parse the implementation-specific XML node that represents an audio trigger.
        //! @param audioTriggerNode XML node corresponding to the new audio trigger object to be
        //!     created.
        //!     Assumes that strings are null-terminated (i.e. the xml_document has been
        //!     parsed without the 'parse_no_string_terminators' flag).
        //! @return Pointer to the newly created audio trigger object, or nullptr if it was not created.
        virtual IATLTriggerImplData* NewAudioTriggerImplData(const AZ::rapidxml::xml_node<char>* audioTriggerNode) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Free the memory and resources of the supplied audio trigger object.
        //! @param oldTriggerData Implementation-specific audio trigger data.
        virtual void DeleteAudioTriggerImplData(IATLTriggerImplData* oldTriggerData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Parse the implementation-specific XML node that represents an audio rtpc.
        //! @param audioRtpcNode XML node corresponding to the new audio rtpc object to be created.
        //!     Assumes that strings are null-terminated (i.e. the xml_document has been
        //!     parsed without the 'parse_no_string_terminators' flag).
        //! @return Pointer to the newly created audio rtpc object, or nullptr if it was not created.
        virtual IATLRtpcImplData* NewAudioRtpcImplData(const AZ::rapidxml::xml_node<char>* audioRtpcNode) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Free the memory and resources of the supplied audio rtpc object.
        //! @param oldRtpcData Implementation-specific audio rtpc data.
        virtual void DeleteAudioRtpcImplData(IATLRtpcImplData* oldRtpcData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Parse the implementation-specific XML node that represents an audio switchstate.
        //! @param audioSwitchStateNode XML node corresponding to the new audio switchstate object to
        //!     be created.
        //!     Assumes that strings are null-terminated (i.e. the xml_document has been
        //!     parsed without the 'parse_no_string_terminators' flag).
        //! @return Pointer to the newly created audio switchstate object, or nullptr if it was not
        //!     created.
        virtual IATLSwitchStateImplData* NewAudioSwitchStateImplData(const AZ::rapidxml::xml_node<char>* audioSwitchStateNode) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Free the memory and resources of the supplied audio switchstate object.
        //! @param oldAudioSwitchStateData Implementation-specific audio switchstate data.
        virtual void DeleteAudioSwitchStateImplData(IATLSwitchStateImplData* oldAudioSwitchStateData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Parse the implementation-specific XML node that represents an audio environment.
        //! @param audioEnvironmentNode XML node corresponding to the new audio environment object to
        //!     be created.
        //!     Assumes that strings are null-terminated (i.e. the xml_document has been
        //!     parsed without the 'parse_no_string_terminators' flag).
        //! @return Pointer to the newly created audio environment object, or nullptr if it was not
        //!     created.
        virtual IATLEnvironmentImplData* NewAudioEnvironmentImplData(const AZ::rapidxml::xml_node<char>* audioEnvironmentNode) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Free the memory and resources of the supplied audio environment object.
        //! @param oldEnvironmentData Implementation-specific audio environment data.
        virtual void DeleteAudioEnvironmentImplData(IATLEnvironmentImplData* oldEnvironmentData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Create an implementation-specific global audio object.
        //! @param objectId Unique ID to assign to the global audio object.
        //! @return Pointer to the newly created global audio object, or nullptr if it was not created.
        virtual IATLAudioObjectData* NewGlobalAudioObjectData(TAudioObjectID objectId) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Create an implementation-specific audio object.
        //! @param objectId Unique ID of the audio object.
        //! @return Pointer to the newly created audio object, or nullptr if it was not created.
        virtual IATLAudioObjectData* NewAudioObjectData(TAudioObjectID objectId) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Free the memory and resources of the supplied audio object data.
        //! @param oldObjectData Implementation-specific audio object data.
        virtual void DeleteAudioObjectData(IATLAudioObjectData* oldObjectData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Create an implementation-specific listener object data that will be the default listener.
        //! @param objectId Unique ID of the default listener.
        //! @return Pointer to the newly created default listener object, or nullptr if it was not
        //!     created.
        virtual IATLListenerData* NewDefaultAudioListenerObjectData(TATLIDType objectId) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Create an implementation-specific listener object data.
        //! @param objectId Unique ID of the listener.
        //! @return Pointer to the newly created listener object, or nullptr if it was not created.
        virtual IATLListenerData* NewAudioListenerObjectData(TATLIDType objectId) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Free the memory and resources of the supplied listener object.
        //! @param oldListenerData Implementation-specific listener object.
        virtual void DeleteAudioListenerObjectData(IATLListenerData* oldListenerData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Create an implementation-specific event object data.
        //! @param eventId Unique ID for the event.
        //! @return Pointer to the newly created event object, or nullptr if it was not created.
        virtual IATLEventData* NewAudioEventData(TAudioEventID eventID) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Free the memory and resources of the supplied event object.
        //! @param oldEventData Implementation-specific event object.
        virtual void DeleteAudioEventData(IATLEventData* oldEventData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Reset all the members of an audio event instance without releasing the memory.
        //! This is used so the event object can be recycled back to the pool.
        //! @param eventData Implementation-specific event data.
        virtual void ResetAudioEventData(IATLEventData* eventData) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Set the language used by the audio middleware.
        //! Informs the audio middleware that the localized sound banks and streamed files need to
        //! use a different language.  This function does not unload or reload the currently
        //! loaded audio files.
        //! @param language A zero-terminated C-string representing the language.
        virtual void SetLanguage(const char* language) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Get the canonical subfolder for this audio middleware implementation.
        //! Used for locating audio data in the game assets folder.
        //! @return A zero-terminated C-string with the subfolder this implementation uses.
        virtual const char* const GetImplSubPath() const = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Get the name of the audio middleware implementation.
        //! This string can be displayed on screen.
        //! @return A zero-terminated C-string with the name of the audio middleware implementation.
        virtual const char* const GetImplementationNameString() const = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Obtain information describing the current memory usage of this audio middleware
        //! implementation.
        //! This data can be displayed on screen.
        //! param memoryInfo A reference to a SAudioImplMemoryInfo struct.
        virtual void GetMemoryInfo(SAudioImplMemoryInfo& memoryInfo) const = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Retrieve information about memory pools active in the audio middleware.
        //! @return Vector of AudioImplMemoryPoolInfo.
        virtual AZStd::vector<AudioImplMemoryPoolInfo> GetMemoryPoolInfo() = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Create an Audio Source as specified by a configuration.
        //! @param sourceConfig Configuration information specifying the format of the source.
        virtual bool CreateAudioSource(const SAudioInputConfig& sourceConfig) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Destroys a managed Audio Source.
        //! @param sourceId ID of the Audio Source.
        virtual void DestroyAudioSource(TAudioSourceId sourceId) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        //! Set the panning mode for the audio middleware.
        //! @param mode The PanningMode to use.
        virtual void SetPanningMode(PanningMode mode) = 0;
    };

    using AudioSystemImplementationRequestBus = AZ::EBus<AudioSystemImplementationRequests>;


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //! This interface is used by the AudioTranslationLayer to interact with an audio middleware
    //! implementation.
    class AudioSystemImplementation
        : public AudioSystemImplementationNotificationBus::Handler
        , public AudioSystemImplementationRequestBus::Handler
    {
    public:
        ~AudioSystemImplementation() override = default;
    };

} // namespace Audio
