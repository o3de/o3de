/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/std/any.h>
#include <Range.h>

namespace AzToolsFramework
{
    /**
    * This bus can be used to send commands to the track view.
    */
    class EditorLayerTrackViewRequests
        : public AZ::ComponentBus
    {
    public:

        /**
        * Gets the number of sequences.
        */
        virtual int GetNumSequences() = 0;

        /**
        * Creates a new sequence of the given type(0 = Object Entity Sequence(Legacy), 1 = Component Entity Sequence(PREVIEW)) with the given name.
        */
        virtual void NewSequence(const char* name, int sequenceType) = 0;

        /**
        * Plays the current sequence in TrackView.
        */
        virtual void PlaySequence() = 0;

        /**
        * Stops any sequence currently playing in TrackView.
        */
        virtual void StopSequence() = 0;

        /**
        * Sets the time of the sequence currently playing in TrackView.
        */
        virtual void SetSequenceTime(float time) = 0;

        /**
        * Adds an entity node(s) from viewport selection to the current sequence.
        */
        virtual void AddSelectedEntities() = 0;

        /**
        * Adds a layer node from the current layer to the current sequence.
        */
        virtual void AddLayerNode() = 0;

        /**
        * Adds a track of the given parameter ID to the node.
        */
        virtual void AddTrack(const char* paramName, const char* nodeName, const char* parentDirectorName) = 0;

        /**
        * Deletes a track of the given parameter ID (in the given index in case of a multi-track) from the node.
        */
        virtual void DeleteTrack(const char* paramName, uint32 index, const char* nodeName, const char* parentDirectorName) = 0;

        /**
        * Gets number of keys of the specified track.
        */
        virtual int GetNumTrackKeys(const char* paramName, int trackIndex, const char* nodeName, const char* parentDirectorName) = 0;

        /**
        * Activates/deactivates TrackView recording mode.
        */
        virtual void SetRecording(bool bRecording) = 0;

        /**
        * Deletes the specified sequence.
        */
        virtual void DeleteSequence(const char* name) = 0;

        /**
        * Sets the specified sequence as a current one in TrackView.
        */
        virtual void SetCurrentSequence(const char* name) = 0;

        /**
        * Gets the name of a sequence by its index.
        */
        virtual AZStd::string GetSequenceName(unsigned int index) = 0;

        /**
        * Gets the time range of a sequence as a pair.
        */
        virtual TRange<float> GetSequenceTimeRange(const char* name) = 0;

        /**
        * Adds a new node with the given type & name to the current sequence.
        */
        virtual void AddNode(const char* nodeTypeString, const char* nodeName) = 0;

        /**
        * Deletes the specified node from the current sequence.
        */
        virtual void DeleteNode(AZStd::string_view nodeName, AZStd::string_view parentDirectorName) = 0;

        /**
        * Gets the number of nodes.
        */
        virtual int GetNumNodes(AZStd::string_view parentDirectorName) = 0;

        /**
        * Gets the name of a sequence by its index.
        */
        virtual AZStd::string GetNodeName(int index, AZStd::string_view parentDirectorName) = 0;

        /**
        * Gets the value of the specified key.
        */
        virtual AZStd::any GetKeyValue(const char* paramName, int trackIndex, int keyIndex, const char* nodeName, const char* parentDirectorName) = 0;

        /**
        * Gets the interpolated value of a track at the specified time.
        */
        virtual AZStd::any GetInterpolatedValue(const char* paramName, int trackIndex, float time, const char* nodeName, const char* parentDirectorName) = 0;

        /**
        * Sets the time range of a sequence.
        */
        virtual void SetSequenceTimeRange(const char* name, float start, float end) = 0;
    };
    using EditorLayerTrackViewRequestBus = AZ::EBus<EditorLayerTrackViewRequests>;
}
