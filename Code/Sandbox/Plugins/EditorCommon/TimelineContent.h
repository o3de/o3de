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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITORCOMMON_TIMELINECONTENT_H
#define CRYINCLUDE_EDITORCOMMON_TIMELINECONTENT_H
#pragma once

#include "QPropertyTree/Color.h"
#include "Serialization/Strings.h"
#include "Serialization/SmartPtr.h"
#include "Serialization.h"

#include <smartptr.h>
#include <AnimTime.h>

using Serialization::string;

struct STimelineElement
{
    enum EType
    {
        KEY,
        CLIP
    };

    enum ECaps
    {
        CAP_SELECT = BIT(0),
        CAP_DELETE = BIT(1),

        // not implemented:
        CAP_MOVE = BIT(2),
        CAP_CHANGE_DURATION = BIT(3)
    };

    EType type;
    int caps;
    SAnimTime start;
    SAnimTime end;
    ColorB color;
    float baseWeight;
    uint64 userId;
    string description;
    DynArray<char> userSideLoad;
    // state flags
    bool selected : 1;
    bool added : 1;
    bool deleted : 1;
    bool sideLoadChanged : 1;

    STimelineElement()
        : type(KEY)
        , start(0.0f)
        , end(0.1f)
        , color(212, 212, 212, 255)
        , caps(CAP_SELECT | CAP_MOVE | CAP_CHANGE_DURATION)
        , selected(false)
        , added(false)
        , deleted(false)
        , sideLoadChanged(false)
        , userId(0)
    {
    }

    void Serialize(IArchive& ar)
    {
        ar(type, "type", "^>80>");
        ar(start, "start", "^");
        if (type == CLIP)
        {
            ar(end, "end", "^");
        }
        ar(BitFlags<ECaps>(caps), "caps", "Capabilities");
        ar(color, "color", "Color");
    }
};
typedef std::vector<STimelineElement> STimelineElements;

struct STimelineTrack;
typedef std::vector<_smart_ptr<STimelineTrack> > STimelineTracks;

struct STimelineTrack
    : public _i_reference_target_t
{
    enum ECaps
    {
        CAP_ADD_ELEMENTS = BIT(0),
        CAP_DESCRIPTION_TRACK = BIT(1), // No keys
        CAP_COMPOUND_TRACK = BIT(2), // No own keys, but will show combined keys for child tracks
        CAP_TOGGLE_TRACK = BIT(3), // For boolean tracks that are either on or off between keys. Used to key visibility etc.
    };
    bool expanded : 1;
    bool modified : 1;
    bool selected : 1;
    bool deleted : 1;
    bool keySelectionChanged : 1;
    bool toggleDefaultState : 1; // Default state for toggle tracks (on or off)
    int height;
    int caps;
    SAnimTime startTime;
    SAnimTime endTime;
    string type;
    string name;
    DynArray<char> userSideLoad;
    STimelineElements elements;
    STimelineElement defaultElement;
    STimelineTracks tracks;

    STimelineTrack()
        : expanded(true)
        , modified(false)
        , selected(false)
        , deleted(false)
        , keySelectionChanged(false)
        , height(64)
        , startTime(0.0f)
        , endTime(1.0f)
        , caps(CAP_ADD_ELEMENTS)
    {}

    void Serialize(IArchive& ar)
    {
        ar(name, "name", "^");
        ar(type, "type", "^");
        ar(height, "height", "Height");
        ar(startTime, "startTime", "Start Time");
        ar(endTime, "endTime", "End Time");
        ar(elements, "elements", "Elements");
        ar(tracks, "tracks", "+Tracks");
    }
};

struct STimelineContent
{
    STimelineTrack track;
    DynArray<char> userSideLoad;

    void Serialize(IArchive& ar)
    {
        ar(track, "track", "Track");
    }
};

#endif // CRYINCLUDE_EDITORCOMMON_TIMELINECONTENT_H
