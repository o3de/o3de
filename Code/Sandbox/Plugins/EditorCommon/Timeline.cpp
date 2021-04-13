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

#include "EditorCommon_precompiled.h"
#include "Timeline.h"
#include "QtUtil.h"
#include "DrawingPrimitives/TimeSlider.h"
#include "DrawingPrimitives/Ruler.h"

#include <vector>
#include <algorithm>
#include <VectorSet.h>
#include <StringUtils.h>

#include <QSet>
#include <QMenu>
#include <QPainter>
#include <QPaintEvent>
#include <QStyleOption>
#include <QLineEdit>
#include <QScrollBar>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

int STimelineViewState::ScrollOffset(float origin) const
{
    return int((origin / visibleDistance + 0.5f) * widthPixels);
}

int STimelineViewState::TimeToLayout(float time) const
{
    return int((time / visibleDistance) * widthPixels + 0.5f);
}

int STimelineViewState::TimeToLocal(float time) const
{
    return TimeToLayout(time) + treeWidth + scrollPixels.x();
}

float STimelineViewState::LayoutToTime(int x) const
{
    return (float(x) - 0.5f) / widthPixels * visibleDistance;
}

float STimelineViewState::LocalToTime(int x) const
{
    return LayoutToTime(x - treeWidth - scrollPixels.x());
}

QPoint STimelineViewState::LocalToLayout(const QPoint& p) const
{
    return p - scrollPixels - QPoint(treeWidth, 0);
}

QPoint STimelineViewState::LayoutToLocal(const QPoint& p) const
{
    return p + scrollPixels + QPoint(treeWidth, 0);
}

enum
{
    THUMB_WIDTH = 12,
    THUMB_HEIGHT = 24,

    RULER_HEIGHT = 16,
    RULER_SHADOW_HEIGHT = 6,
    RULER_MARK_HEIGHT = 8,

    TRACK_MARK_HEIGHT = 6,

    DEFAULT_KEY_WIDTH = 8,
    VERTICAL_PADDING = 4,
    TRACK_DESCRIPTION_INDENT = 8,

    SELECTION_WIDTH = 4,
    SCROLL_SHADOW_WIDTH = 8,

    MAX_PUSH_OUT = VERTICAL_PADDING * 2,
    PUSH_OUT_DISTANCE = 3,

    SPLITTER_WIDTH = 10,

    DEFAULT_TREE_WIDTH = 200,
    TREE_LEFT_MARGIN = 6,
    TREE_INDENT_MULTIPLIER = 12,
    TREE_BRANCH_INDICATOR_SIZE = 8
};

namespace
{
    const float DEFAULT_KEY_RADIUS = 0.1f;
    const int TIMELINE_PADDING = 20;
}

struct STimelineContentElementRef
{
    STimelineContentElementRef()
        : pTrack(nullptr)
        , index(0)
    {}

    STimelineContentElementRef(STimelineTrack* pTrack, size_t index)
        : pTrack(pTrack)
        , index(index)
    {}

    STimelineElement& GetElement() const
    {
        return pTrack->elements[index];
    }

    bool IsValid() const { return pTrack != 0 && index < pTrack->elements.size(); }

    bool operator<(const STimelineContentElementRef& rhs) const
    {
        if (pTrack == rhs.pTrack)
        {
            return index < rhs.index;
        }
        else
        {
            if (!pTrack && rhs.pTrack)
            {
                return true;
            }
            else if (pTrack && !rhs.pTrack)
            {
                return false;
            }
            else if (pTrack && rhs.pTrack)
            {
                return pTrack->name < rhs.pTrack->name;
            }
            return false;
        }
    }

    STimelineTrack* pTrack;
    size_t index;
};

struct SElementLayout
{
    STimelineElement::EType type;
    int caps;
    float pushOutDistance;
    QRect rect;
    ColorB color;
    string description;

    STimelineContentElementRef elementRef;
    std::vector<STimelineContentElementRef> subElements;

    bool IsSelected() const
    {
        if ((elementRef.pTrack->caps & STimelineTrack::CAP_COMPOUND_TRACK) == 0)
        {
            return elementRef.GetElement().selected;
        }
        else
        {
            bool bSelected = false;
            const size_t numSubElements = subElements.size();

            for (size_t i = 0; i < numSubElements; ++i)
            {
                bSelected = bSelected || subElements[i].GetElement().selected;
            }

            return bSelected;
        }
    }

    void SetSelected(const bool bSelected) const
    {
        if ((elementRef.pTrack->caps & STimelineTrack::CAP_COMPOUND_TRACK) == 0)
        {
            elementRef.GetElement().selected = bSelected;
        }
        else
        {
            const size_t numSubElements = subElements.size();
            for (size_t i = 0; i < numSubElements; ++i)
            {
                subElements[i].GetElement().selected = bSelected;
            }
        }
    }

    SElementLayout()
        : pushOutDistance(0.0f)
        , caps(0)
        , type(STimelineElement::KEY)
    {}
};

struct STrackLayout;
typedef std::vector<STrackLayout> STrackLayouts;

struct STrackLayout
{
    QRect rect;
    int indent;

    STimelineTrack* pTimelineTrack;

    std::vector<SElementLayout> elements;
    STrackLayouts tracks;

    STrackLayout()
        : indent(0)
        , pTimelineTrack(0)
    {
    }
};

struct STimelineLayout
{
    int thumbPositionX;
    STrackLayouts tracks;
    SAnimTime minStartTime;
    SAnimTime maxEndTime;
    QSize size;

    STimelineLayout()
        : thumbPositionX(0)
        , minStartTime(0.0f)
        , maxEndTime(1.0f)
        , size(1, 1)
    {
    }
};

namespace
{
    QColor InterpolateColor(const QColor& a, const QColor& b, float k)
    {
        float mk = 1.0f - k;
        return QColor(aznumeric_cast<int>(a.red() * mk  + b.red() * k),
            aznumeric_cast<int>(a.green() * mk + b.green() * k),
            aznumeric_cast<int>(a.blue() * mk + b.blue() * k),
            aznumeric_cast<int>(a.alpha() * mk + b.alpha() * k));
    }

    void ClampViewOrigin(STimelineViewState* viewState, const STimelineLayout& layout)
    {
        float zoomOffset = viewState->visibleDistance * 0.5f;

        const float padding = viewState->LayoutToTime(TIMELINE_PADDING);
        float maxViewOrigin = layout.minStartTime.ToFloat() - zoomOffset + padding;
        float minViewOrigin = std::min(viewState->visibleDistance - layout.maxEndTime.ToFloat() - zoomOffset - padding, maxViewOrigin);

        viewState->viewOrigin = clamp_tpl(viewState->viewOrigin, minViewOrigin, maxViewOrigin);
    }

    SElementLayout& AddElementToTrackLayout(STimelineTrack& track, STrackLayout& trackLayout, const STimelineElement& element,
        const STimelineViewState& viewState, uint keyWidth, [[maybe_unused]] int treeWidth, int& currentTop, size_t elementIndex)
    {
        trackLayout.elements.push_back(SElementLayout());
        SElementLayout& elementl = trackLayout.elements.back();
        elementl.color = element.color;
        elementl.type = element.type;
        elementl.caps = element.caps;
        elementl.description = element.description;
        elementl.elementRef.pTrack = &track;
        elementl.elementRef.index = elementIndex;

        if (element.type == STimelineElement::KEY)
        {
            int left = (viewState.TimeToLayout(element.start.ToFloat()) - keyWidth / 2);
            int right = left + keyWidth;
            elementl.rect = QRect(left, currentTop + VERTICAL_PADDING, right - left, track.height - VERTICAL_PADDING * 2);
        }
        else
        {
            int left = viewState.TimeToLayout(element.start.ToFloat());
            int right = viewState.TimeToLayout(element.end.ToFloat());
            elementl.rect = QRect(left, currentTop + VERTICAL_PADDING, right - left, track.height - VERTICAL_PADDING * 2);
        }

        return elementl;
    }

    void AddCompoundElementsToTrackLayout(STimelineTrack& track, STimelineLayout* layout, const STimelineViewState& viewState, int trackId, uint keyWidth, int treeWidth, int& currentTop)
    {
        const size_t numSubTracks = track.tracks.size();
        SAnimTime currentElementTime = SAnimTime::Min();
        size_t* pCurrentSubTrackIndices = static_cast<size_t*>(alloca(sizeof(size_t) * numSubTracks));
        memset(pCurrentSubTrackIndices, 0, sizeof(size_t) * numSubTracks);

        while (true)
        {
            bool bElementFound = false;
            SAnimTime minElementTime = SAnimTime::Max();

            // First search for minimum element time for current track positions
            for (size_t i = 0; i < numSubTracks; ++i)
            {
                const STimelineTrack& subTrack = *track.tracks[i];
                const STimelineElements& elements = subTrack.elements;
                const size_t numTrackElements = elements.size();
                const size_t index = pCurrentSubTrackIndices[i];

                if (index < numTrackElements)
                {
                    const SAnimTime elementTime = elements[index].start;
                    minElementTime = min(elementTime, minElementTime);
                    bElementFound = true;
                }
            }

            if (!bElementFound)
            {
                break;
            }

            STimelineElement compoundElement;
            compoundElement.start = minElementTime;
            compoundElement.end = minElementTime;

            // If elements were found create a compound element
            STrackLayout& trackLayout = layout->tracks[trackId];
            SElementLayout& compoundElementLayout = AddElementToTrackLayout(track, trackLayout, compoundElement, viewState, keyWidth, treeWidth, currentTop, 0);
            currentElementTime = minElementTime;

            compoundElementLayout.description = "(";

            // Advance track positions and add elements IDs to compound element if times match
            for (size_t i = 0; i < numSubTracks; ++i)
            {
                STimelineTrack* pSubTrack = track.tracks[i];
                const STimelineElements& elements = pSubTrack->elements;
                const size_t numTrackElements = elements.size();
                size_t& index = pCurrentSubTrackIndices[i];

                if (index < numTrackElements)
                {
                    const SAnimTime elementTime = elements[index].start;

                    if (elementTime == minElementTime)
                    {
                        STimelineContentElementRef ref;
                        ref.pTrack = pSubTrack;
                        ref.index = index;
                        compoundElementLayout.subElements.push_back(ref);
                        compoundElementLayout.description += elements[index].description;
                        ++index;
                    }
                    else
                    {
                        compoundElementLayout.description += "-";
                    }

                    if ((i + 1) < numSubTracks)
                    {
                        compoundElementLayout.description += ", ";
                    }
                }
            }

            compoundElementLayout.description += ")";
        }
    }

    bool FilterTracks(const STimelineTrack& track, std::unordered_set<const STimelineTrack*>& invisibleTracks, const char* filterString)
    {
        bool bAnyChildVisible = false;
        const bool bNameMatchesFilter = CryStringUtils::stristr(track.name, filterString) != nullptr;

        if (!bNameMatchesFilter)
        {
            const size_t numChildTracks = track.tracks.size();
            for (size_t i = 0; i < numChildTracks; ++i)
            {
                bAnyChildVisible = FilterTracks(*track.tracks[i], invisibleTracks, filterString) || bAnyChildVisible;
            }
        }

        if (!bNameMatchesFilter && !bAnyChildVisible)
        {
            invisibleTracks.insert(&track);
        }

        return bNameMatchesFilter || bAnyChildVisible;
    }

    void CalculateMinMaxTime(STimelineLayout* layout, STimelineTrack& parentTrack)
    {
        layout->minStartTime = min(layout->minStartTime, parentTrack.startTime);
        layout->maxEndTime = max(layout->maxEndTime, parentTrack.endTime);

        const size_t numTracks = parentTrack.tracks.size();
        for (size_t i = 0; i < numTracks; ++i)
        {
            STimelineTrack& track = *parentTrack.tracks[i];
            CalculateMinMaxTime(layout, track);
        }
    }

    void CalculateTrackLayout(STimelineLayout* layout, int& currentTop, int currentIndent, STimelineTrack& parentTrack, const STimelineViewState& viewState,
        float thumbTime, uint keyWidth, int treeWidth, const std::unordered_set<const STimelineTrack*>& invisibleTracks)
    {
        const size_t numTracks = parentTrack.tracks.size();
        for (size_t i = 0; i < numTracks; ++i)
        {
            STimelineTrack& track = *parentTrack.tracks[i];

            if (stl::find(invisibleTracks, &track))
            {
                continue;
            }

            layout->tracks.push_back(STrackLayout());
            STrackLayout& trackLayout = layout->tracks.back();
            trackLayout.elements.reserve(track.elements.size());
            trackLayout.indent = currentIndent;
            trackLayout.pTimelineTrack = &track;

            const bool bIsCompositeTrack = (track.caps & STimelineTrack::CAP_COMPOUND_TRACK) != 0;

            if (bIsCompositeTrack)
            {
                const int trackLayoutId = layout->tracks.size() - 1;
                AddCompoundElementsToTrackLayout(track, layout, viewState, trackLayoutId, keyWidth, treeWidth, currentTop);
            }
            else
            {
                for (size_t i2 = 0; i2 < track.elements.size(); ++i2)
                {
                    const STimelineElement& element = track.elements[i2];
                    AddElementToTrackLayout(track, trackLayout, element, viewState, keyWidth, treeWidth, currentTop, i2);
                }
            }

            int left = viewState.TimeToLayout(track.startTime.ToFloat());
            int right = viewState.TimeToLayout(track.endTime.ToFloat());
            trackLayout.rect = QRect(left, currentTop, right - left, track.height);
            currentTop += track.height;

            if (track.expanded)
            {
                CalculateTrackLayout(layout, currentTop, currentIndent + 1, track, viewState, thumbTime, keyWidth, treeWidth, invisibleTracks);
            }
        }
    }

    void ApplyPushOut(STimelineLayout* layout, uint keyWidth)
    {
        float maxPushOut = 0.0f;

        const size_t numLayoutTracks = layout->tracks.size();
        for (size_t i = 0; i < numLayoutTracks; ++i)
        {
            STrackLayout& track = layout->tracks[i];

            const size_t numElements = track.elements.size();
            for (size_t i2 = 0; i2 < numElements; ++i2)
            {
                if (track.elements[i2].type != STimelineElement::KEY)
                {
                    continue;
                }

                for (size_t j = 0; j < numElements; ++j)
                {
                    if ((track.elements[j].type != STimelineElement::KEY) || (j == i2))
                    {
                        continue;
                    }

                    float distance = aznumeric_cast<float>(track.elements[j].rect.left() - track.elements[i2].rect.left());
                    float delta = clamp_tpl(1.0f - fabsf(distance) / keyWidth, 0.0f, 1.0f);

                    if (delta == 0.0f)
                    {
                        continue;
                    }

                    float& pushOutDistance = track.elements[i2].pushOutDistance;
                    pushOutDistance += (i2 < j) ? -delta : delta;

                    if (fabsf(pushOutDistance) > maxPushOut)
                    {
                        maxPushOut = fabsf(pushOutDistance);
                    }
                }
            }
        }

        float maxPushOutNormalized = float(MAX_PUSH_OUT) / PUSH_OUT_DISTANCE;
        float pushOutScale = 1.0f;

        if (maxPushOut > maxPushOutNormalized && maxPushOut > 0.0f)
        {
            pushOutScale = maxPushOutNormalized / maxPushOut;
        }

        for (size_t i = 0; i < numLayoutTracks; ++i)
        {
            STrackLayout& track = layout->tracks[i];

            for (size_t j = 0; j < track.elements.size(); ++j)
            {
                track.elements[j].rect.translate(QPoint(0, int(pushOutScale * track.elements[j].pushOutDistance * PUSH_OUT_DISTANCE)));
            }
        }
    }

    void CalculateLayout(STimelineLayout* layout, STimelineContent& content, const STimelineViewState& viewState, const QLineEdit* pFilterLineEdit, float thumbTime, uint keyWidth, bool treeVisible)
    {
        layout->thumbPositionX = viewState.TimeToLayout(thumbTime);

        if (!content.track.tracks.empty())
        {
            layout->minStartTime = SAnimTime::Max();
            layout->maxEndTime = SAnimTime::Min();
        }
        else
        {
            layout->minStartTime = SAnimTime(0.0f);
            layout->maxEndTime = SAnimTime(1.0f);
        }

        int currentTop = RULER_HEIGHT + VERTICAL_PADDING;
        const int treeWidth = treeVisible ? viewState.treeWidth : 0;

        std::unordered_set<const STimelineTrack*> invisibleTracks;
        if (pFilterLineEdit && !pFilterLineEdit->text().isEmpty())
        {
            FilterTracks(content.track, invisibleTracks, QtUtil::ToString(pFilterLineEdit->text()));
        }

        CalculateMinMaxTime(layout, content.track);
        CalculateTrackLayout(layout, currentTop, 0, content.track, viewState, thumbTime, keyWidth, treeWidth, invisibleTracks);

        layout->size = QSize(viewState.TimeToLayout(layout->maxEndTime.ToFloat()), currentTop + VERTICAL_PADDING);
    }

    STrackLayout* HitTestTrack(STrackLayouts& tracks, const QPoint& point)
    {
        auto findIter = std::upper_bound(tracks.begin(), tracks.end(), point.y(), [&](int y, const STrackLayout& track)
                {
                    return y < track.rect.bottom();
                });

        if (findIter != tracks.end() && findIter->rect.contains(point))
        {
            return &(*findIter);
        }

        return nullptr;
    }

    void ForEachTrack(STimelineTrack& track, AZStd::function<void (STimelineTrack& track)> fun)
    {
        fun(track);

        for (size_t i = 0; i < track.tracks.size(); ++i)
        {
            STimelineTrack& subTrack = *track.tracks[i];
            ForEachTrack(subTrack, fun);
        }
    }

    void ForEachElement(STimelineTrack& track, AZStd::function<void (STimelineTrack& track, STimelineElement& element)> fun)
    {
        ForEachTrack(track, [&](STimelineTrack& subTrack)
            {
                for (size_t i = 0; i < subTrack.elements.size(); ++i)
                {
                    fun(subTrack, subTrack.elements[i]);
                }
            });
    }

    void ForEachElementWithIndex(STimelineTrack& track, AZStd::function<void (STimelineTrack& track, STimelineElement& element, size_t elementIndex)> fun)
    {
        ForEachTrack(track, [&](STimelineTrack& subTrack)
            {
                for (size_t i = 0; i < subTrack.elements.size(); ++i)
                {
                    fun(subTrack, subTrack.elements[i], i);
                }
            });
    }

    void ClearTrackSelection(STimelineTrack& track)
    {
        ForEachTrack(track, [](STimelineTrack& track)
            {
                track.selected = false;
            });
    }

    void GetSelectedTracks(STimelineTrack& track, std::vector<STimelineTrack*>& tracks)
    {
        ForEachTrack(track, [&](STimelineTrack& track)
            {
                if (track.selected)
                {
                    tracks.push_back(&track);
                }
            });
    }

    void ClearElementSelection(STimelineTrack& track)
    {
        ForEachElement(track, [](STimelineTrack& track, STimelineElement& element)
            {
                track.keySelectionChanged = track.keySelectionChanged || element.selected;
                element.selected = false;
            });
    }

    void SetSelectedElementTimes(STimelineTrack& track, const std::vector<SAnimTime>& times)
    {
        auto iter = times.begin();
        ForEachElement(track, [&](STimelineTrack& track, STimelineElement& element)
            {
                if (element.selected)
                {
                    track.modified = true;
                    element.start = *(iter++);
                }
            });
    }

    std::vector<SAnimTime> GetSelectedElementTimes(STimelineTrack& track)
    {
        std::vector<SAnimTime> times;
        ForEachElement(track, [&]([[maybe_unused]] STimelineTrack& track, STimelineElement& element)
            {
                if (element.selected)
                {
                    times.push_back(element.start);
                }
            });
        return times;
    }

    VectorSet<SAnimTime> GetSelectedElementsTimeSet(STimelineTrack& track)
    {
        VectorSet<SAnimTime> timeSet;
        ForEachElement(track, [&]([[maybe_unused]] STimelineTrack& track, STimelineElement& element)
            {
                if (element.selected)
                {
                    timeSet.insert(element.start);
                }
            });
        return timeSet;
    }

    typedef std::vector<std::pair<STimelineTrack*, STimelineElement*> > TSelectedElements;
    TSelectedElements GetSelectedElements(STimelineTrack& track)
    {
        TSelectedElements elements;

        ForEachElement(track, [&](STimelineTrack& track, STimelineElement& element)
            {
                if (element.selected)
                {
                    elements.push_back(std::make_pair(&track, &element));
                }
            });

        return elements;
    }

    void MoveSelectedElements(STimelineTrack& track, SAnimTime delta)
    {
        ForEachElement(track, [&](STimelineTrack& track, STimelineElement& element)
            {
                if (element.selected)
                {
                    track.modified = true;
                    element.start += delta;
                }
            });
    }

    void DeletedMarkedElements(STimelineTrack& track)
    {
        ForEachTrack(track, [&](STimelineTrack& track)
            {
                for (auto iter = track.elements.begin(); iter != track.elements.end(); )
                {
                    if (iter->deleted)
                    {
                        iter = track.elements.erase(iter);
                    }
                    else
                    {
                        ++iter;
                    }
                }
            });
    }

    void SelectElementsInRect(const STrackLayouts& tracks, const QRect& rect)
    {
        for (size_t j = 0; j < tracks.size(); ++j)
        {
            const STrackLayout& track = tracks[j];

            for (size_t i = 0; i < track.elements.size(); ++i)
            {
                const SElementLayout& element = track.elements[i];

                if ((element.caps & STimelineElement::CAP_SELECT) == 0)
                {
                    continue;
                }

                STimelineTrack* pTimelineTrack = element.elementRef.pTrack;
                const bool bIsCompoundTrack = (pTimelineTrack->caps & STimelineTrack::CAP_COMPOUND_TRACK) != 0;

                if (element.rect.intersects(rect))
                {
                    if (!bIsCompoundTrack)
                    {
                        if (!element.elementRef.GetElement().selected)
                        {
                            element.elementRef.GetElement().selected = true;
                            element.elementRef.pTrack->keySelectionChanged = true;
                        }
                    }
                    else
                    {
                        const size_t numSubElements = element.subElements.size();

                        for (size_t k = 0; k < numSubElements; ++k)
                        {
                            if (!element.subElements[k].GetElement().selected)
                            {
                                element.subElements[k].GetElement().selected = true;
                                element.subElements[k].pTrack->keySelectionChanged = true;
                            }
                        }
                    }
                }
            }

            SelectElementsInRect(track.tracks, rect);
        }
    }

    typedef std::vector<SElementLayout*> SElementLayoutPtrs;
    bool HitTestElements(STrackLayouts& tracks, const QRect& rect, SElementLayoutPtrs& out)
    {
        bool bHit = false;

        for (size_t j = 0; j < tracks.size(); ++j)
        {
            STrackLayout& track = tracks[j];

            for (size_t i = 0; i < track.elements.size(); ++i)
            {
                SElementLayout& element = track.elements[i];

                if (element.rect.intersects(rect))
                {
                    out.push_back(&element);
                    bHit = true;
                }
            }

            bHit = bHit || HitTestElements(track.tracks, rect, out);
        }

        return bHit;
    }

    enum EPass
    {
        PASS_BACKGROUND,
        PASS_SELECTION,
        PASS_SHADOW,
        PASS_MAIN,
        NUM_PASSES
    };

    QBrush PickTrackBrush(const QPalette& palette, const STrackLayout& track)
    {
        const QColor trackColor = InterpolateColor(palette.color(QPalette::Mid), palette.color(QPalette::Window), 0.96f);
        const QColor descriptionTrackColor = InterpolateColor(palette.color(QPalette::Mid), palette.color(QPalette::Window), 0.9f);
        const QColor compositeTrackColor = InterpolateColor(palette.color(QPalette::Mid), palette.color(QPalette::Window), 0.85f);
        const QColor selectionColor = InterpolateColor(palette.color(QPalette::Highlight), palette.color(QPalette::Window), 0.5f);

        const bool bIsDescriptionTrack = (track.pTimelineTrack->caps & STimelineTrack::CAP_DESCRIPTION_TRACK) != 0;
        const bool bIsCompositeTrack = (track.pTimelineTrack->caps & STimelineTrack::CAP_COMPOUND_TRACK) != 0;

        const QColor color = bIsDescriptionTrack ? descriptionTrackColor : (bIsCompositeTrack ? compositeTrackColor : trackColor);
        return QBrush(track.pTimelineTrack->selected ? InterpolateColor(color, selectionColor, 0.3f) : color);
    }

    struct SElementLayoutIntCompareLeft
    {
        const bool operator()(const SElementLayout& a, int b)   { return a.rect.left() < b; }
        const bool operator()(int a, const SElementLayout& b)   { return a < b.rect.left(); }
    };

    struct SElementLayoutIntCompareRight
    {
        const bool operator()(const SElementLayout& a, int b)   { return a.rect.right() < b; }
        const bool operator()(int a, const SElementLayout& b)   { return a < b.rect.right(); }
    };

    void DrawTracks(QPainter& painter, const uint startPass, const uint endPass, const STimelineLayout& layout, const STimelineViewState& viewState,
        const QPalette& palette, [[maybe_unused]] const QPoint& mousePos, bool hasFocus, int width, float keyRadius, float timeUnitScale, bool drawMarkers)
    {
        const STrackLayouts& tracks = layout.tracks;

        const int trackAreaLeft = viewState.LocalToLayout(QPoint(viewState.treeWidth, 0)).x();
        const int trackAreaRight = trackAreaLeft + width;

        const QColor textColor = palette.buttonText().color();
        QPen descriptionTextPen = QPen(InterpolateColor(textColor, palette.color(QPalette::Window), 0.5f));

        DrawingPrimitives::STickOptions markOptions;
        markOptions.m_rect = QRect(-viewState.scrollPixels.x(), 0, width - viewState.treeWidth, 0);
        markOptions.m_visibleRange = Range(viewState.LocalToTime(viewState.treeWidth) * timeUnitScale, viewState.LocalToTime(width) * timeUnitScale);
        markOptions.m_rulerRange = Range(layout.minStartTime.ToFloat() * timeUnitScale, layout.maxEndTime.ToFloat() * timeUnitScale);
        markOptions.m_markHeight = TRACK_MARK_HEIGHT;

        // Precalculate ticks because they are the same for all tracks
        std::vector<DrawingPrimitives::STick> ticks = DrawingPrimitives::CalculateTicks(markOptions.m_rect.width(), markOptions.m_visibleRange, markOptions.m_rulerRange, nullptr, nullptr);

        for (int i = startPass; i <= endPass; ++i)
        {
            EPass pass = (EPass)i;

            for (size_t j = 0; j < tracks.size(); ++j)
            {
                const STrackLayout& track = tracks[j];

                const bool bIsDescriptionTrack = (track.pTimelineTrack->caps & STimelineTrack::CAP_DESCRIPTION_TRACK) != 0;
                const bool bIsCompositeTrack = (track.pTimelineTrack->caps & STimelineTrack::CAP_COMPOUND_TRACK) != 0;
                const bool bIsToggleTrack = (track.pTimelineTrack->caps & STimelineTrack::CAP_TOGGLE_TRACK) != 0;

                std::vector<SElementLayout> sortedElements = track.elements;
                std::sort(sortedElements.begin(), sortedElements.end(), [](const SElementLayout& a, const SElementLayout& b) { return a.rect.left() < b.rect.left(); });
                const uint numElements = sortedElements.size();

                if (pass == PASS_BACKGROUND)
                {
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(PickTrackBrush(palette, track));
                    QRect backgroundRect = track.rect;
                    backgroundRect.setLeft(-viewState.scrollPixels.x());
                    backgroundRect.setWidth(width);
                    painter.drawRect(backgroundRect);

                    if (bIsDescriptionTrack)
                    {
                        painter.setPen(descriptionTextPen);
                        QRect textRect = track.rect;
                        textRect.moveLeft(textRect.left() - viewState.scrollPixels.x() + TRACK_DESCRIPTION_INDENT);
                        textRect.setWidth(width);
                        textRect.moveTop(textRect.top() + 1);
                        painter.drawText(textRect, QString(track.pTimelineTrack->name));
                    }

                    const int lineY = track.rect.bottom() + 1;
                    painter.setPen(QPen(InterpolateColor(palette.color(QPalette::Mid), palette.color(QPalette::Window), 0.75f)));
                    painter.drawLine(QPoint(trackAreaLeft, lineY), QPoint(trackAreaRight, lineY));

                    if (drawMarkers && !bIsDescriptionTrack)
                    {
                        markOptions.m_rect.setTop(track.rect.top());
                        markOptions.m_rect.setBottom(track.rect.bottom());
                        DrawingPrimitives::DrawTicks(ticks, painter, palette, markOptions);
                    }

                    if (bIsToggleTrack)
                    {
                        const QColor toggleColor = InterpolateColor(QColor(255, 255, 255), palette.color(QPalette::Mid), 0.5f);

                        const uint drawStart = track.pTimelineTrack->toggleDefaultState ? 0 : 1;

                        painter.setBrush(QBrush(toggleColor));
                        QRect toggleRect = track.rect;
                        toggleRect.setTop(toggleRect.top() + 2);
                        toggleRect.setBottom(toggleRect.bottom() - 2);

                        for (uint i2 = drawStart; i2 <= numElements; i2 += 2)
                        {
                            const int left = (i2 == 0) ? (-viewState.scrollPixels.x()) : sortedElements[i2 - 1].rect.right();
                            const int right = (i2 == numElements) ? (-viewState.scrollPixels.x() + width) : sortedElements[i2].rect.left();

                            toggleRect.setLeft(left);
                            toggleRect.setRight(right);
                            painter.drawRect(toggleRect);
                        }
                    }

                    continue;
                }

                auto begin = std::lower_bound(sortedElements.begin(), sortedElements.end(), -viewState.scrollPixels.x(), SElementLayoutIntCompareRight());
                auto end = std::upper_bound(sortedElements.begin(), sortedElements.end(), width - viewState.scrollPixels.x(), SElementLayoutIntCompareLeft());

                if (begin != sortedElements.begin())
                {
                    --begin;
                }
                if (end != sortedElements.end())
                {
                    ++end;
                }

                for (auto iter = begin; iter != end; ++iter)
                {
                    const SElementLayout& element = *iter;
                    QRectF rect = QRectF(element.rect);
                    float ratio = 1.0f;

                    if (rect.width() != 0)
                    {
                        ratio = rect.height() != 0 ? aznumeric_cast<float>(rect.width() / float(rect.height())) : 1.0f;
                    }

                    const bool bSelected = element.IsSelected();

                    if (element.type == STimelineElement::KEY)
                    {
                        float rx = keyRadius * 200.0f / ratio;
                        float ry = keyRadius * 200.0f;

                        if (pass == PASS_SELECTION)
                        {
                            if (bSelected)
                            {
                                QRectF selectionRect = rect.adjusted(-SELECTION_WIDTH * 0.5f + 0.5f, -SELECTION_WIDTH * 0.5f + 0.5f, SELECTION_WIDTH * 0.5f - 0.5f, SELECTION_WIDTH * 0.5f - 0.5f);
                                painter.setPen(QPen(palette.color(hasFocus ? QPalette::Highlight : QPalette::Shadow), SELECTION_WIDTH));
                                painter.setBrush(QBrush(Qt::NoBrush));
                                painter.drawRoundedRect(selectionRect, rx, ry, Qt::RelativeSize);
                            }
                        }
                        else if (pass != PASS_SHADOW)
                        {
                            QRectF shadowRect = rect.adjusted(-1.0f, -0.5f, 1.0f, 1.5f);
                            painter.setPen(QPen(QColor(0, 0, 0, 128), 2.0f));
                            painter.setBrush(QBrush(Qt::NoBrush));
                            painter.drawRoundedRect(shadowRect, rx, ry, Qt::RelativeSize);

                            painter.setPen(QPen(QColor(element.color.r, element.color.g, element.color.b, 255)));
                            painter.setBrush(QBrush(QColor(element.color.r, element.color.g, element.color.b, 255)));
                            painter.drawRoundedRect(rect, rx, ry, Qt::RelativeSize);

                            QRect textRect = track.rect;
                            textRect.moveLeft(aznumeric_cast<int>(rect.right() + TRACK_DESCRIPTION_INDENT));
                            textRect.setTop(textRect.top() + 1);

                            if ((iter + 1) != sortedElements.end())
                            {
                                textRect.setRight((iter + 1)->rect.left() - 6);
                            }

                            painter.setPen(descriptionTextPen);
                            const QString elidedText = painter.fontMetrics().elidedText(element.description.c_str(), Qt::ElideRight, textRect.width());
                            painter.drawText(textRect, Qt::TextSingleLine, elidedText);
                        }
                    }
                    else
                    {
                        float radius = 0.2f;
                        float rx = radius * 200.0f / ratio;
                        float ry = radius * 200.0f;

                        if (pass == PASS_SELECTION)
                        {
                            if (bSelected)
                            {
                                QRectF selectionRect = rect.adjusted(-SELECTION_WIDTH * 0.5f + 0.5f, -SELECTION_WIDTH * 0.5f + 0.5f, SELECTION_WIDTH * 0.5f - 0.5f, SELECTION_WIDTH * 0.5f - 0.5f);
                                painter.setPen(QPen(palette.color(hasFocus ? QPalette::Highlight : QPalette::Shadow), SELECTION_WIDTH));
                                painter.setBrush(QBrush(Qt::NoBrush));
                                painter.drawRoundedRect(selectionRect, rx, ry, Qt::RelativeSize);
                            }
                        }
                        else if (pass == PASS_SHADOW)
                        {
                            QRectF shadowRect = rect.adjusted(0.0f, 0.0f, 0.0f, 1.0f);
                            painter.setPen(QPen(QColor(0, 0, 0, 128), 2.0f));
                            painter.setBrush(QBrush(Qt::NoBrush));
                            painter.drawRoundedRect(shadowRect, radius * 200.0f / ratio, radius * 200.0f, Qt::RelativeSize);
                        }
                        else
                        {
                            painter.setPen(QPen(QColor(element.color.r, element.color.g, element.color.b, 255)));
                            painter.setBrush(QBrush(QColor(element.color.r, element.color.g, element.color.b, 128)));
                            painter.drawRoundedRect(rect, radius * 200.0f / ratio, radius * 200.0f, Qt::RelativeSize);
                        }
                    }
                }
            }
        }
    }

    void DrawSelectionLines(QPainter& painter, const QPalette& palette, const STimelineViewState& viewState, STimelineContent& content, [[maybe_unused]] int rulerPrecision, [[maybe_unused]] int width, int height, [[maybe_unused]] float time, [[maybe_unused]] float timeUnitScale, bool hasFocus)
    {
        const VectorSet<SAnimTime> times = GetSelectedElementsTimeSet(content.track);

        QColor indicatorColor = palette.color(hasFocus ? QPalette::Highlight : QPalette::Shadow);
        indicatorColor.setAlpha(70);

        for (auto iter = times.begin(); iter != times.end(); ++iter)
        {
            const float indicatorX = viewState.TimeToLocal(iter->ToFloat()) + 0.5f;
            painter.setPen(indicatorColor);
            painter.drawLine(QPointF(indicatorX, 0), QPointF(indicatorX, height));
        }
    }

    void DrawTree(QPainter& painter, const QRect& treeRect, const QPalette& palette, QWidget* timeline, [[maybe_unused]] const STimelineContent& content, const STrackLayouts& tracks, const STimelineViewState& viewState, int scroll)
    {
        painter.save();

        painter.setClipRect(treeRect);
        painter.setClipping(true);

        painter.translate(0, -scroll);

        QTextOption textOption;
        textOption.setWrapMode(QTextOption::NoWrap);

        const QColor textColor = palette.buttonText().color();

        QStyleOptionFrame opt;
        opt.palette = palette;
        opt.state = QStyle::State_Enabled;
        opt.rect = QRect(treeRect.left(), treeRect.top() - 1, treeRect.width(), treeRect.height() + 2);

        // Draw frame around tree
        timeline->style()->drawPrimitive(QStyle::PE_Frame, &opt, &painter, timeline);

        for (size_t i = 0; i < tracks.size(); ++i)
        {
            const STrackLayout& track = tracks[i];

            const QRect backgroundRect(1, track.rect.top() + 1, viewState.treeWidth - SPLITTER_WIDTH - 1, track.rect.height() - 1);

            const bool bIsDescriptionTrack = (track.pTimelineTrack->caps & STimelineTrack::CAP_DESCRIPTION_TRACK) != 0;
            const bool bIsCompositeTrack = (track.pTimelineTrack->caps & STimelineTrack::CAP_COMPOUND_TRACK) != 0;

            painter.setPen(Qt::NoPen);
            painter.setBrush(PickTrackBrush(palette, track));
            painter.drawRect(backgroundRect);

            const int branchLeft = TREE_LEFT_MARGIN + track.indent * TREE_INDENT_MULTIPLIER;

            if (track.pTimelineTrack->tracks.size() > 0)
            {
                QStyleOptionViewItem opt2;
                opt2.rect = QRect(branchLeft, track.rect.top() + 1, TREE_BRANCH_INDICATOR_SIZE, track.rect.height() - 2);
                opt2.state = QStyle::State_Enabled | QStyle::State_Children;
                opt2.state |= track.pTimelineTrack->expanded ? QStyle::State_Open : QStyle::State_None;

                timeline->style()->drawPrimitive(QStyle::PE_IndicatorBranch, &opt2, &painter, timeline);
            }

            const int textLeft = branchLeft + TREE_BRANCH_INDICATOR_SIZE + 4;
            const int textWidth = std::max(treeRect.width() - textLeft - 4, 0);
            const QRect textRect(textLeft, track.rect.top() + 1, textWidth, track.rect.height() - 2);
            painter.setPen(QPen(textColor));
            painter.drawText(textRect, QString(track.pTimelineTrack->name), textOption);
        }

        for (size_t i = 0; i < tracks.size(); ++i)
        {
            const STrackLayout& track = tracks[i];

            const int lineY = track.rect.bottom() + 1;
            painter.setPen(QPen(InterpolateColor(palette.color(QPalette::Mid), palette.color(QPalette::Window), 0.75f)));
            painter.drawLine(QPoint(0, lineY), QPoint(treeRect.width(), lineY));
        }

        painter.restore();
    }

    void DrawSplitter(QPainter& painter, const QRect& splitterRect, const QPalette& palette, QWidget* timeline)
    {
        painter.fillRect(splitterRect, palette.color(QPalette::Window));

        // Draw frame around splitter
        QStyleOptionFrame frameOpt;
        frameOpt.palette = palette;
        frameOpt.state = QStyle::State_Enabled;
        frameOpt.rect = QRect(splitterRect.left(), splitterRect.top(), splitterRect.width(), splitterRect.height() + 2);
        timeline->style()->drawPrimitive(QStyle::PE_Frame, &frameOpt, &painter, timeline);

        // Draw resize handle dots
        QStyleOption option;
        option.palette = palette;
        option.rect = QRect(splitterRect.left(), splitterRect.top() - 1, splitterRect.width() - 2, splitterRect.height() + 2);
        timeline->style()->drawPrimitive(QStyle::PE_IndicatorDockWidgetResizeHandle, &option, &painter, timeline);
    }
}

struct CTimeline::SMouseHandler
{
    virtual ~SMouseHandler() = default;
    virtual void mousePressEvent([[maybe_unused]] QMouseEvent* ev) {}
    virtual void mouseDoubleClickEvent([[maybe_unused]] QMouseEvent* ev) {}
    virtual void mouseMoveEvent([[maybe_unused]] QMouseEvent* ev) {}
    virtual void mouseReleaseEvent([[maybe_unused]] QMouseEvent* ev) {}
    virtual void focusOutEvent([[maybe_unused]] QFocusEvent* ev) {}
    virtual void paintOver([[maybe_unused]] QPainter& painter) {}
};

struct CTimeline::SSelectionHandler
    : SMouseHandler
{
    CTimeline* m_timeline;
    QPoint m_startPoint;
    QRect m_rect;
    bool m_add;
    TSelectedElements m_oldSelectedElements;

    SSelectionHandler(CTimeline* timeline, bool add)
        : m_timeline(timeline)
        , m_add(add)
    {
        if (m_timeline->m_pContent)
        {
            m_oldSelectedElements = GetSelectedElements(m_timeline->m_pContent->track);
        }
    }

    void mousePressEvent(QMouseEvent* ev) override
    {
        const int scroll = m_timeline->m_scrollBar ? m_timeline->m_scrollBar->value() : 0;
        const QPoint pos(ev->pos().x(), ev->pos().y() + scroll);
        m_startPoint = m_timeline->m_viewState.LocalToLayout(pos);
        m_rect = QRect(m_startPoint, m_startPoint + QPoint(1, 1));
    }

    void mouseMoveEvent(QMouseEvent* ev) override
    {
        const int scroll = m_timeline->m_scrollBar ? m_timeline->m_scrollBar->value() : 0;
        const QPoint pos(ev->pos().x(), ev->pos().y() + scroll);
        m_rect = QRect(m_startPoint, m_timeline->m_viewState.LocalToLayout(pos) + QPoint(1, 1));
        Apply(true);
    }

    void Apply(bool continuous)
    {
        if (m_timeline->m_pContent)
        {
            const TSelectedElements selectedElements = GetSelectedElements(m_timeline->m_pContent->track);

            ClearElementSelection(m_timeline->m_pContent->track);
            SelectElementsInRect(m_timeline->m_layout->tracks, m_rect);

            const TSelectedElements newSelectedElements = GetSelectedElements(m_timeline->m_pContent->track);
            if ((continuous && selectedElements != newSelectedElements)
                || (!continuous && m_oldSelectedElements != newSelectedElements))
            {
                m_timeline->SignalSelectionChanged(continuous);
            }
        }
    }

    void mouseReleaseEvent([[maybe_unused]] QMouseEvent* ev) override
    {
        Apply(false);
    }

    void paintOver(QPainter& painter) override
    {
        painter.save();
        QColor highlightColor = m_timeline->palette().color(QPalette::Highlight);
        QColor highlightColorA = QColor(highlightColor.red(), highlightColor.green(), highlightColor.blue(), 128);
        painter.setPen(QPen(highlightColor));
        painter.setBrush(QBrush(highlightColorA));
        painter.drawRect(QRectF(m_rect));
        painter.restore();
    }
};

static STimelineElement* NextSelectedElement(const SElementLayoutPtrs& array, STimelineElement* nextToValue, STimelineElement* defaultValue)
{
    for (size_t i = 0; i < array.size(); ++i)
    {
        if (&array[i]->elementRef.GetElement() == nextToValue)
        {
            return &array[(i + 1) % array.size()]->elementRef.GetElement();
        }
    }
    return defaultValue;
}

struct CTimeline::SMoveHandler
    : SMouseHandler
{
    CTimeline* m_timeline;
    QPoint m_startPoint;
    bool m_cycleSelection;
    SAnimTime m_startTime;
    SAnimTime m_newTime;
    std::vector<SAnimTime> m_elementTimes;

    SMoveHandler(CTimeline* timeline, bool cycleSelection)
        : m_timeline(timeline)
        , m_cycleSelection(cycleSelection) {}

    void mousePressEvent(QMouseEvent* ev) override
    {
        m_startTime = m_timeline->m_time;
        m_newTime = m_startTime;

        const int scroll = m_timeline->m_scrollBar ? m_timeline->m_scrollBar->value() : 0;
        const QPoint currentPos(ev->pos().x(), ev->pos().y() + scroll);

        m_startPoint = m_timeline->m_viewState.LocalToLayout(QPoint(currentPos));
        m_elementTimes = GetSelectedElementTimes(m_timeline->m_pContent->track);
    }

    void mouseMoveEvent(QMouseEvent* ev) override
    {
        if (m_timeline->m_viewState.widthPixels == 0)
        {
            return;
        }

        const int scroll = m_timeline->m_scrollBar ? m_timeline->m_scrollBar->value() : 0;
        const QPoint currentPos(ev->pos().x(), ev->pos().y() + scroll);

        int delta = m_timeline->m_viewState.LocalToLayout(currentPos).x() - m_startPoint.x();

        const TSelectedElements selectedElements = GetSelectedElements(m_timeline->m_pContent->track);

        SetSelectedElementTimes(m_timeline->m_pContent->track, m_elementTimes);

        SAnimTime minDeltaTime = SAnimTime::Min();
        SAnimTime maxDeltaTime = SAnimTime::Max();
        SAnimTime minKeyTime = SAnimTime::Min();

        for (size_t i = 0; i < selectedElements.size(); ++i)
        {
            const STimelineTrack& track = *selectedElements[i].first;
            const STimelineElement& element = *selectedElements[i].second;

            SAnimTime minStartDelta = track.startTime - element.start;
            minDeltaTime = max(minStartDelta, minDeltaTime);
            SAnimTime maxEndDelta = track.endTime - (element.type == element.CLIP ? element.end : element.start);
            maxDeltaTime = min(maxEndDelta, maxDeltaTime);

            minKeyTime = min(element.start, minKeyTime);
        }

        SAnimTime deltaTime = SAnimTime(float(delta) / m_timeline->m_viewState.widthPixels * m_timeline->m_viewState.visibleDistance);
        if (m_timeline->m_snapKeys)
        {
            SAnimTime newMinKeyTime = minKeyTime + deltaTime;
            newMinKeyTime = newMinKeyTime.SnapToNearest(m_timeline->m_frameRate);
            deltaTime = newMinKeyTime - minKeyTime;
        }

        deltaTime = clamp_tpl(deltaTime, minDeltaTime, maxDeltaTime);

        m_newTime = m_startTime + deltaTime;

        MoveSelectedElements(m_timeline->m_pContent->track, deltaTime);

        m_timeline->ContentChanged(true);

        m_timeline->setCursor(Qt::SizeHorCursor);
        m_cycleSelection = false;
    }

    void focusOutEvent([[maybe_unused]] QFocusEvent* ev)
    {
        SetSelectedElementTimes(m_timeline->m_pContent->track, m_elementTimes);
        m_timeline->UpdateLayout();
    }

    void mouseReleaseEvent(QMouseEvent* ev)
    {
        if (m_cycleSelection)
        {
            SElementLayoutPtrs hitElements;

            const int scroll = m_timeline->m_scrollBar ? m_timeline->m_scrollBar->value() : 0;
            const QPoint currentPos(ev->pos().x(), ev->pos().y() + scroll);

            QPoint posInLayoutSpace = m_timeline->m_viewState.LocalToLayout(currentPos);
            HitTestElements(m_timeline->m_layout->tracks, QRect(posInLayoutSpace - QPoint(2, 2), posInLayoutSpace + QPoint(2, 2)), hitElements);

            if (!hitElements.empty())
            {
                TSelectedElements selectedElements = GetSelectedElements(m_timeline->m_pContent->track);
                if (selectedElements.size() == 1)
                {
                    STimelineElement* lastSelection = selectedElements[0].second;

                    ClearElementSelection(m_timeline->m_pContent->track);
                    NextSelectedElement(hitElements, lastSelection, &hitElements.back()->elementRef.GetElement())->selected = true;
                    m_timeline->SignalSelectionChanged(false);
                }
                else
                {
                    ClearElementSelection(m_timeline->m_pContent->track);
                    hitElements.back()->elementRef.GetElement().selected = true;
                    m_timeline->SignalSelectionChanged(false);
                }
            }
        }

        m_timeline->ContentChanged(false);
    }
};

struct CTimeline::SPanHandler
    : SMouseHandler
{
    CTimeline* m_timeline;
    QPoint m_startPoint;
    float m_startOrigin;

    SPanHandler(CTimeline* timeline)
        : m_timeline(timeline)
    {
    }

    void mousePressEvent(QMouseEvent* ev) override
    {
        m_startPoint = QPoint(int(ev->x()), int(ev->y()));
        m_startOrigin = m_timeline->m_viewState.viewOrigin;
    }

    void mouseMoveEvent(QMouseEvent* ev) override
    {
        QPoint pos(int(ev->x()), int(ev->y()));
        float delta = 0.0f;

        if (m_timeline->m_viewState.widthPixels != 0)
        {
            delta = (pos - m_startPoint).x() * m_timeline->m_viewState.visibleDistance / m_timeline->m_viewState.widthPixels;
        }

        m_timeline->m_viewState.viewOrigin = m_startOrigin + delta;
        ClampViewOrigin(&m_timeline->m_viewState, *m_timeline->m_layout);
    }

    void mouseReleaseEvent([[maybe_unused]] QMouseEvent* ev) override
    {
    }
};

struct CTimeline::SScrubHandler
    : SMouseHandler
{
    CTimeline* m_timeline;
    SScrubHandler(CTimeline* timeline)
        : m_timeline(timeline) {}
    SAnimTime m_startThumbPosition;
    QPoint m_startPoint;

    void SetThumbPositionX(int positionX)
    {
        SAnimTime time = SAnimTime(m_timeline->m_viewState.LayoutToTime(positionX));

        m_timeline->ClampAndSetTime(time, false);
    }

    void mousePressEvent(QMouseEvent* ev) override
    {
        QPoint point = QPoint(ev->pos().x(), ev->pos().y());

        QPoint posInLayout = m_timeline->m_viewState.LocalToLayout(point);

        int thumbPositionX = m_timeline->m_viewState.TimeToLayout(m_timeline->m_time.ToFloat());
        QRect thumbRect(thumbPositionX - THUMB_WIDTH / 2, 0, THUMB_WIDTH, THUMB_HEIGHT);

        if (!thumbRect.contains(posInLayout))
        {
            SetThumbPositionX(m_timeline->m_viewState.LocalToLayout(point).x());
        }

        m_startThumbPosition = m_timeline->m_time;
        m_startPoint = point;
    }

    void Apply(QMouseEvent* ev, [[maybe_unused]] bool continuous)
    {
        QPoint point = QPoint(ev->pos().x(), ev->pos().y());

        bool shift = ev->modifiers().testFlag(Qt::ShiftModifier);
        bool control = ev->modifiers().testFlag(Qt::ControlModifier);

        float delta = 0.0f;

        if (m_timeline->m_viewState.widthPixels != 0)
        {
            delta = (point.x() - m_startPoint.x()) * m_timeline->m_viewState.visibleDistance / m_timeline->m_viewState.widthPixels;
        }

        if (shift)
        {
            delta *= 0.01f;
        }

        if (control)
        {
            delta *= 0.1f;
        }

        m_timeline->ClampAndSetTime(m_startThumbPosition + SAnimTime(delta), true);
    }

    void mouseMoveEvent(QMouseEvent* ev) override
    {
        Apply(ev, true);
    }

    void mouseReleaseEvent(QMouseEvent* ev) override
    {
        Apply(ev, false);
    }
};

struct CTimeline::SSplitterHandler
    : SMouseHandler
{
    CTimeline* m_timeline;
    int m_offset;
    bool m_movedSlider;

    SSplitterHandler(CTimeline* timeline)
        : m_timeline(timeline)
        , m_offset(0)
        , m_movedSlider(false) {}

    void mousePressEvent(QMouseEvent* ev) override
    {
        m_offset = m_timeline->m_viewState.treeWidth - ev->pos().x();
    }

    void mouseReleaseEvent([[maybe_unused]] QMouseEvent* ev) override
    {
        if (!m_movedSlider)
        {
            STimelineViewState& viewState = m_timeline->m_viewState;

            if (viewState.treeWidth == SPLITTER_WIDTH)
            {
                viewState.treeWidth = viewState.treeLastOpenedWidth;
            }
            else
            {
                viewState.treeLastOpenedWidth = viewState.treeWidth;
                viewState.treeWidth = SPLITTER_WIDTH;
            }

            m_timeline->UpdateLayout();
            m_timeline->update();
        }
    }

    void mouseMoveEvent(QMouseEvent* ev) override
    {
        m_timeline->setCursor(QCursor(Qt::SplitHCursor));
        uint treeWidth = clamp_tpl<int>(ev->pos().x(), SPLITTER_WIDTH, m_timeline->width()) + m_offset;
        m_timeline->m_viewState.treeWidth = treeWidth;
        m_timeline->m_viewState.treeLastOpenedWidth = treeWidth;
        m_timeline->UpdateLayout();
        m_timeline->update();
        m_movedSlider = true;
    }
};

struct CTimeline::STreeMouseHandler
    : SMouseHandler
{
    CTimeline* m_timeline;
    STreeMouseHandler(CTimeline* timeline)
        : m_timeline(timeline) {}

    void mousePressEvent(QMouseEvent* ev) override
    {
        const bool bCtrlPressed = (ev->modifiers() & Qt::CTRL) != 0;
        const bool bShiftPressed = (ev->modifiers() & Qt::SHIFT) != 0;

        const int scroll = m_timeline->m_scrollBar ? m_timeline->m_scrollBar->value() : 0;
        const QPoint pos(ev->pos().x(), ev->pos().y() + scroll);

        STrackLayout* pTrackLayout = m_timeline->GetTrackLayoutFromPos(pos);

        if (!pTrackLayout)
        {
            if (!bShiftPressed && !bCtrlPressed)
            {
                ClearTrackSelection(m_timeline->m_pContent->track);
            }
        }
        else
        {
            const int left = TREE_LEFT_MARGIN + pTrackLayout->indent * TREE_INDENT_MULTIPLIER;
            const int right = left + TREE_BRANCH_INDICATOR_SIZE;

            const int x = pos.x();

            if (x >= left && x <= right)
            {
                ToggleTrackExpansion(pTrackLayout);
            }
            else
            {
                const bool bPreviousState = pTrackLayout->pTimelineTrack->selected;

                if (!bCtrlPressed)
                {
                    ClearTrackSelection(m_timeline->m_pContent->track);
                }

                if (bCtrlPressed)
                {
                    pTrackLayout->pTimelineTrack->selected = !bPreviousState;
                }
                else if (bShiftPressed)
                {
                    STrackLayouts& tracks = m_timeline->m_layout->tracks;

                    auto startFindIter = std::find_if(tracks.begin(), tracks.end(), [=](const STrackLayout& track) { return (pTrackLayout == &track); });
                    auto endFindIter = std::find_if(tracks.begin(), tracks.end(), [=](const STrackLayout& track) { return (m_timeline->m_pLastSelectedTrack == &track); });

                    if (startFindIter != tracks.end() && endFindIter != tracks.end())
                    {
                        if (startFindIter > endFindIter)
                        {
                            std::swap(startFindIter, endFindIter);
                        }

                        for (auto iter = startFindIter; iter <= endFindIter; ++iter)
                        {
                            iter->pTimelineTrack->selected = true;
                        }
                    }
                }
                else
                {
                    pTrackLayout->pTimelineTrack->selected = true;
                }

                if (!bShiftPressed && pTrackLayout->pTimelineTrack->selected)
                {
                    m_timeline->m_pLastSelectedTrack = pTrackLayout;
                }

                m_timeline->SignalTrackSelectionChanged();
            }
        }
    }

    void mouseDoubleClickEvent(QMouseEvent* ev) override
    {
        if (ev->modifiers() == 0)
        {
            STrackLayout* pTrackLayout = m_timeline->GetTrackLayoutFromPos(ev->pos());
            ToggleTrackExpansion(pTrackLayout);
        }
    }

private:
    void ToggleTrackExpansion(STrackLayout* pTrackLayout)
    {
        if (pTrackLayout && pTrackLayout->pTimelineTrack)
        {
            pTrackLayout->pTimelineTrack->expanded = !pTrackLayout->pTimelineTrack->expanded;
            m_timeline->UpdateLayout();
            m_timeline->update();
        }
    }
};

// ---------------------------------------------------------------------------

CTimeline::CTimeline(QWidget* parent)
    : QWidget(parent)
    , m_cycled(true)
    , m_sizeToContent(false)
    , m_snapTime(false)
    , m_snapKeys(false)
    , m_treeVisible(false)
    , m_selIndicators(false)
    , m_verticalScrollbarVisible(false)
    , m_drawMarkers(false)
    , m_layout(new STimelineLayout)
    , m_keyWidth(DEFAULT_KEY_WIDTH)
    , m_keyRadius(DEFAULT_KEY_RADIUS)
    , m_cornerWidget(nullptr)
    , m_scrollBar(nullptr)
    , m_cornerWidgetWidth(0)
    , m_pContent(nullptr)
    , m_timeUnitScale(1.0f)
    , m_timeStepNum(1)
    , m_timeStepIndex(0)
    , m_frameRate(SAnimTime::eFrameRate_30fps)
    , m_time(0.0f)
    , m_pFilterLineEdit(nullptr)
    , m_pLastSelectedTrack(nullptr)
{
    setMinimumWidth(THUMB_WIDTH * 3);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
    setFocusPolicy(Qt::WheelFocus);
    setMouseTracking(true);

    m_viewState.visibleDistance = 1.0f;
}

CTimeline::~CTimeline()
{
}

void CTimeline::paintEvent([[maybe_unused]] QPaintEvent* ev)
{
    const QPoint mousePos = mapFromGlobal(QCursor::pos());

    QPainter painter(this);
    painter.save();
    painter.translate(0.5f, 0.5f);

    if (m_viewState.visibleDistance != 0)
    {
        SAnimTime totalDuration = m_layout->maxEndTime - m_layout->minStartTime;
        m_viewState.scrollPixels = QPoint(m_viewState.ScrollOffset(m_viewState.viewOrigin), 0);
        m_viewState.maxScrollX = int(m_viewState.widthPixels * totalDuration.ToFloat() / m_viewState.visibleDistance) - m_viewState.widthPixels;
    }
    else
    {
        m_viewState.scrollPixels = QPoint(0, 0);
        m_viewState.maxScrollX = 0;
    }

    const int scroll = m_scrollBar ? m_scrollBar->value() : 0;
    const QPoint localToLayoutTranslate = m_viewState.LayoutToLocal(QPoint(0, -scroll));

    int rulerPrecision = 0;

    painter.translate(localToLayoutTranslate);
    painter.setRenderHint(QPainter::Antialiasing);

    DrawTracks(painter, PASS_BACKGROUND, PASS_BACKGROUND, *m_layout, m_viewState,
        palette(), mousePos, hasFocus(), width(), m_keyRadius, m_timeUnitScale, m_drawMarkers);

    DrawTracks(painter, PASS_SELECTION, PASS_MAIN, *m_layout, m_viewState,
        palette(), mousePos, hasFocus(), width(), m_keyRadius, m_timeUnitScale, m_drawMarkers);

    painter.translate(-localToLayoutTranslate);

    DrawingPrimitives::SRulerOptions rulerOptions;
    rulerOptions.m_rect = QRect(m_viewState.treeWidth, -1, size().width() - m_viewState.treeWidth, RULER_HEIGHT + 2);
    rulerOptions.m_visibleRange = Range(m_viewState.LocalToTime(m_viewState.treeWidth) * m_timeUnitScale, m_viewState.LocalToTime(size().width()) * m_timeUnitScale);
    rulerOptions.m_rulerRange = Range(m_layout->minStartTime.ToFloat() * m_timeUnitScale, m_layout->maxEndTime.ToFloat() * m_timeUnitScale);
    rulerOptions.m_markHeight = RULER_MARK_HEIGHT;
    rulerOptions.m_shadowSize = RULER_SHADOW_HEIGHT;
    DrawingPrimitives::DrawRuler(painter, palette(), rulerOptions, &rulerPrecision);

    if (m_pContent && isEnabled())
    {
        DrawingPrimitives::STimeSliderOptions timeSliderOptions;
        timeSliderOptions.m_rect = rect();
        timeSliderOptions.m_precision = rulerPrecision;
        timeSliderOptions.m_position = m_viewState.TimeToLocal(m_time.ToFloat());
        timeSliderOptions.m_time = m_time.ToFloat() * m_timeUnitScale;
        timeSliderOptions.m_bHasFocus = hasFocus();
        DrawingPrimitives::DrawTimeSlider(painter, palette(), timeSliderOptions);

        DrawSelectionLines(painter, palette(), m_viewState, *m_pContent, rulerPrecision, width(), height(), m_time.ToFloat(), m_timeUnitScale, hasFocus());
    }

    painter.translate(localToLayoutTranslate);

    if (m_mouseHandler)
    {
        m_mouseHandler->paintOver(painter);
    }

    painter.translate(-localToLayoutTranslate);

    if (m_viewState.scrollPixels.x() < 0)
    {
        QRect rect(m_viewState.treeWidth, 0, SCROLL_SHADOW_WIDTH, height());
        QLinearGradient grad(rect.left(), rect.top(), rect.right(), rect.top());
        grad.setColorAt(0.0f, QColor(0, 0, 0, 96));
        grad.setColorAt(1.0f, QColor(0, 0, 0, 0));
        painter.fillRect(rect, QBrush(grad));
    }

    SAnimTime totalDuration = m_layout->maxEndTime - m_layout->minStartTime;

    if (m_viewState.scrollPixels.x() > -m_viewState.maxScrollX)
    {
        QRect rect(width() - SCROLL_SHADOW_WIDTH, 0, SCROLL_SHADOW_WIDTH, height());
        QLinearGradient grad(rect.left(), rect.top(), rect.right(), rect.top());
        grad.setColorAt(0.0f, QColor(0, 0, 0, 0));
        grad.setColorAt(1.0f, QColor(0, 0, 0, 96));
        painter.fillRect(rect, QBrush(grad));
    }

    {
        QColor color = palette().color(QPalette::Dark);
        color.setAlpha(128);
        painter.setPen(QPen(color));
        painter.drawLine(QPoint(0, 0), QPoint(0, height()));
        painter.drawLine(QPoint(width() - 1, 0), QPoint(width() - 1, height()));
        painter.drawLine(QPoint(1, 0), QPoint(width() - 1, 0));
        painter.drawLine(QPoint(0, height()), QPoint(width() - 1, height()));
    }

    painter.restore();

    if (m_treeVisible)
    {
        if (m_pContent)
        {
            QRect treeRect(0, 0, m_viewState.treeWidth - SPLITTER_WIDTH + 1, height());
            DrawTree(painter, treeRect, palette(), this, *m_pContent, m_layout->tracks, m_viewState, scroll);
        }

        QRect splitterRect(m_viewState.treeWidth - SPLITTER_WIDTH, 0, SPLITTER_WIDTH, height());
        DrawSplitter(painter, splitterRect, palette(), this);
    }

    if (!isEnabled())
    {
        QColor disabledOverlayColor = palette().color(QPalette::Disabled, QPalette::Button);
        disabledOverlayColor.setAlpha(128);
        painter.fillRect(0, 0, width(), height(), QBrush(disabledOverlayColor));
    }
}

void CTimeline::keyPressEvent(QKeyEvent* ev)
{
    const QPoint mousePos = mapFromGlobal(QCursor::pos());
    QMouseEvent mouseEvent(QEvent::MouseMove, mousePos, Qt::NoButton, Qt::NoButton, ev->modifiers());
    mouseMoveEvent(&mouseEvent);
    int rawKey = ev->key() | ev->modifiers();
    QKeySequence key(rawKey);

    if (key == QKeySequence(Qt::Key_Z | Qt::CTRL))
    {
        SignalUndo();
    }
    else if (key == QKeySequence(Qt::Key_Y | Qt::CTRL) || (key == QKeySequence(Qt::Key_Z | Qt::CTRL | Qt::SHIFT)))
    {
        SignalRedo();
    }
    else
    {
        HandleKeyEvent(rawKey);
    }
}

void CTimeline::keyReleaseEvent(QKeyEvent* ev)
{
    const QPoint mousePos = mapFromGlobal(QCursor::pos());
    QMouseEvent mouseEvent(QEvent::MouseMove, mousePos, Qt::NoButton, Qt::NoButton, ev->modifiers());
    mouseMoveEvent(&mouseEvent);
}

bool CTimeline::HandleKeyEvent(int k)
{
    QKeySequence key(k);

    if (key == QKeySequence(Qt::Key_Delete))
    {
        OnMenuDelete();
        return true;
    }

    if (key == QKeySequence(Qt::Key_D))
    {
        OnMenuDuplicate();
        return true;
    }

    if (key == QKeySequence(Qt::Key_Home))
    {
        m_time = SAnimTime(0);
        update();
        SignalScrub(false);
        return true;
    }
    if (key == QKeySequence(Qt::Key_End))
    {
        SAnimTime endTime = SAnimTime(0);
        for (size_t i = 0; i < m_pContent->track.tracks.size(); ++i)
        {
            endTime = std::max(endTime, m_pContent->track.tracks[i]->endTime);
        }
        m_time = endTime;
        update();
        SignalScrub(false);
        return true;
    }
    if (key == QKeySequence(Qt::Key_X) || key == QKeySequence(Qt::Key_PageUp))
    {
        OnMenuPreviousKey();
        return true;
    }
    if (key == QKeySequence(Qt::Key_C) || key == QKeySequence(Qt::Key_PageDown))
    {
        OnMenuNextKey();
        return true;
    }

    if (k == Qt::Key_Comma || k == Qt::Key_Left)
    {
        OnMenuPreviousFrame();
        return true;
    }

    if (k == Qt::Key_Period || k == Qt::Key_Right)
    {
        OnMenuNextFrame();
        return true;
    }

    if (key == QKeySequence(Qt::Key_Space))
    {
        OnMenuPlay();
        return true;
    }

    // shortcut is Ctrl+#
    int maskedKey = ((~Qt::KeyboardModifierMask) & k);
    if (((k & Qt::CTRL) != 0) && (maskedKey >= Qt::Key_0 && maskedKey <= Qt::Key_9))
    {
        int number = maskedKey - int(Qt::Key_0);
        SignalNumberHotkey(number);
        return true;
    }

    return false;
}

bool CTimeline::ProcessesKey(const QKeySequence& key)
{
    static QSet<QKeySequence> customShortcuts = {
        QKeySequence(Qt::Key_Delete),
        QKeySequence(Qt::Key_D),
        QKeySequence(Qt::Key_Home),
        QKeySequence(Qt::Key_End),
        QKeySequence(Qt::Key_PageUp),
        QKeySequence(Qt::Key_X),
        QKeySequence(Qt::Key_PageDown),
        QKeySequence(Qt::Key_C),
        QKeySequence(Qt::Key_Comma),
        QKeySequence(Qt::Key_Left),
        QKeySequence(Qt::Key_Period),
        QKeySequence(Qt::Key_Right),
        QKeySequence(Qt::Key_Space),
        QKeySequence(Qt::CTRL | Qt::Key_0),
        QKeySequence(Qt::CTRL | Qt::Key_1),
        QKeySequence(Qt::CTRL | Qt::Key_2),
        QKeySequence(Qt::CTRL | Qt::Key_3),
        QKeySequence(Qt::CTRL | Qt::Key_4),
        QKeySequence(Qt::CTRL | Qt::Key_5),
        QKeySequence(Qt::CTRL | Qt::Key_6),
        QKeySequence(Qt::CTRL | Qt::Key_7),
        QKeySequence(Qt::CTRL | Qt::Key_8),
        QKeySequence(Qt::CTRL | Qt::Key_9)
    };

    return customShortcuts.contains(key);
}

void CTimeline::mousePressEvent(QMouseEvent* ev)
{
    setFocus();

    const bool bInTreeArea = m_treeVisible && (ev->x() <= m_viewState.treeWidth);

    if (ev->button() == Qt::LeftButton)
    {
        QPoint posInLayout = m_viewState.LocalToLayout(ev->pos());

        if (bInTreeArea)
        {
            const bool bOverSplitter = ev->x() >= (m_viewState.treeWidth - SPLITTER_WIDTH);

            if (bOverSplitter)
            {
                m_mouseHandler.reset(new SSplitterHandler(this));
                m_mouseHandler->mousePressEvent(ev);
                update();
            }
            else
            {
                m_mouseHandler.reset(new STreeMouseHandler(this));
                m_mouseHandler->mousePressEvent(ev);
                update();
            }
        }
        else if (ev->y() < RULER_HEIGHT)
        {
            m_mouseHandler.reset(new SScrubHandler(this));
            m_mouseHandler->mousePressEvent(ev);
            update();
        }
        else
        {
            QPoint posInLayoutSpace = m_viewState.LocalToLayout(ev->pos());

            SElementLayoutPtrs hitElements;
            bool bHit = HitTestElements(m_layout->tracks, QRect(posInLayoutSpace - QPoint(2, 2), posInLayoutSpace + QPoint(2, 2)), hitElements);

            if (ev->modifiers() & Qt::SHIFT || ev->modifiers() & Qt::CTRL)
            {
                if (bHit)
                {
                    hitElements.back()->SetSelected(hitElements.back()->IsSelected());
                    mouseMoveEvent(ev);
                    update();
                }
                else
                {
                    m_mouseHandler.reset(new SSelectionHandler(this, true));
                    m_mouseHandler->mousePressEvent(ev);
                }
            }
            else
            {
                if (bHit)
                {
                    bool useExistingSelection = std::any_of(hitElements.begin(), hitElements.end(), [](const SElementLayout* element) { return element->IsSelected(); });

                    if (!useExistingSelection)
                    {
                        const TSelectedElements selectedElements = GetSelectedElements(m_pContent->track);

                        ClearElementSelection(m_pContent->track);
                        hitElements.back()->SetSelected(true);

                        if (selectedElements != GetSelectedElements(m_pContent->track))
                        {
                            SignalSelectionChanged(false);
                        }
                    }

                    bool cycleSelection = useExistingSelection;
                    m_mouseHandler.reset(new SMoveHandler(this, cycleSelection));
                    m_mouseHandler->mousePressEvent(ev);
                    update();
                }
                else
                {
                    m_mouseHandler.reset(new SSelectionHandler(this, false));
                    m_mouseHandler->mousePressEvent(ev);
                    update();
                }
            }
        }
    }
    else if (ev->button() == Qt::MiddleButton)
    {
        if (!bInTreeArea)
        {
            m_mouseHandler.reset(new SPanHandler(this));
            m_mouseHandler->mousePressEvent(ev);
            update();
        }
    }
    else if (ev->button() == Qt::RightButton)
    {
        if (bInTreeArea)
        {
            std::vector<STimelineTrack*> selectedTracks;
            GetSelectedTracks(m_pContent->track, selectedTracks);

            STrackLayout*   pLayout = GetTrackLayoutFromPos(ev->pos());
            if (pLayout)
            {
                if (!stl::find(selectedTracks, pLayout->pTimelineTrack))
                {
                    ClearTrackSelection(m_pContent->track);
                    pLayout->pTimelineTrack->selected = true;
                }

                SignalTreeContextMenu(mapToGlobal(ev->pos()));
            }
        }
        else
        {
            QMenu menu;
            bool hasSelection = false;
            ForEachElement(m_pContent->track, [&]([[maybe_unused]] STimelineTrack& t, STimelineElement& e)
                {
                    if (e.selected)
                    {
                        hasSelection = true;
                    }
                });

            menu.addAction("Selection to Cursor", this, SLOT(OnMenuSelectionToCursor()))->setEnabled(hasSelection);
            QAction* duplicateAction = menu.addAction("Duplicate", this, SLOT(OnMenuDuplicate()), QKeySequence("D"));
            duplicateAction->setEnabled(hasSelection);
            menu.addSeparator();
            menu.addAction("Delete Event(s)", this, SLOT(OnMenuDelete()), QKeySequence("Delete"))->setEnabled(hasSelection);
            menu.addSeparator();
            menu.addAction("Play / Pause", this, SLOT(OnMenuPlay()), QKeySequence("Space"));
            menu.addAction("Previous Frame", this, SLOT(OnMenuPreviousFrame()), QKeySequence(","));
            menu.addAction("Next Frame", this, SLOT(OnMenuNextFrame()), QKeySequence("."));
            menu.addAction("Jump to Previous Event", this, SLOT(OnMenuPreviousKey()), QKeySequence("X"));
            menu.addAction("Jump to Next Event", this, SLOT(OnMenuNextKey()), QKeySequence("C"));
            menu.exec(QCursor::pos(), duplicateAction);
        }
    }
}

void CTimeline::AddKeyToTrack(STimelineTrack& track, SAnimTime time)
{
    if (m_snapKeys)
    {
        time = time.SnapToNearest(m_frameRate);
    }

    track.modified = true;
    track.elements.push_back(track.defaultElement);
    track.elements.back().added = true;
    SAnimTime length = track.defaultElement.end - track.defaultElement.start;
    track.elements.back().start = time;
    track.elements.back().end = length;
    track.elements.back().selected = true;
}

void CTimeline::mouseDoubleClickEvent(QMouseEvent* ev)
{
    if (ev->button() == Qt::LeftButton)
    {
        QPoint posInLayout = m_viewState.LocalToLayout(ev->pos());

        const bool bInTreeArea = m_treeVisible && (ev->x() <= m_viewState.treeWidth);

        if (bInTreeArea)
        {
            const bool bOverSplitter = ev->x() >= (m_viewState.treeWidth - SPLITTER_WIDTH);

            if (!bOverSplitter)
            {
                m_mouseHandler.reset(new STreeMouseHandler(this));
                m_mouseHandler->mouseDoubleClickEvent(ev);
                update();
            }
        }

        QPoint layoutPoint = m_viewState.LocalToLayout(ev->pos());
        STrackLayout* track = HitTestTrack(m_layout->tracks, layoutPoint);

        if (track)
        {
            SElementLayoutPtrs hitElements;
            const bool bHit = HitTestElements(m_layout->tracks, QRect(layoutPoint - QPoint(2, 2), layoutPoint + QPoint(2, 2)), hitElements);

            if (!bHit)
            {
                float time = m_viewState.LayoutToTime(layoutPoint.x());
                STimelineTrack& timelineTrack = *track->pTimelineTrack;

                if ((timelineTrack.caps & STimelineTrack::CAP_COMPOUND_TRACK) == 0)
                {
                    AddKeyToTrack(timelineTrack, SAnimTime(time));
                }
                else
                {
                    const size_t numSubTracks = timelineTrack.tracks.size();
                    for (size_t i = 0; i < numSubTracks; ++i)
                    {
                        STimelineTrack& subTrack = *timelineTrack.tracks[i];
                        AddKeyToTrack(subTrack, SAnimTime(time));
                    }
                }

                ContentChanged(false);
                m_mouseHandler.reset();
                mouseMoveEvent(ev);
            }
        }
    }
}

void CTimeline::UpdateCursor(QMouseEvent* ev)
{
    const int scroll = m_scrollBar ? m_scrollBar->value() : 0;
    const QPoint pos(ev->pos().x(), ev->pos().y() + scroll);

    QPoint posInLayoutSpace = m_viewState.LocalToLayout(pos);

    SElementLayoutPtrs hitElements;
    HitTestElements(m_layout->tracks, QRect(posInLayoutSpace - QPoint(2, 2), posInLayoutSpace + QPoint(2, 2)), hitElements);
    const bool bOverSelected = !hitElements.empty() && hitElements.back()->IsSelected();
    const bool bInTreeArea = m_treeVisible && (ev->x() <= m_viewState.treeWidth);

    bool shift = ev->modifiers().testFlag(Qt::ShiftModifier);
    bool control = ev->modifiers().testFlag(Qt::ControlModifier);

    if (m_mouseHandler)
    {
        m_mouseHandler->mouseMoveEvent(ev);
        update();
    }
    else if (m_treeVisible && (ev->x() <= m_viewState.treeWidth) && (ev->x() >= (m_viewState.treeWidth - SPLITTER_WIDTH)))
    {
        setCursor(QCursor(Qt::SplitHCursor));
    }
    else if (!bInTreeArea && bOverSelected && !(shift || control))
    {
        setCursor(QCursor(Qt::SizeHorCursor));
    }
    else
    {
        setCursor(QCursor());
    }
}

void CTimeline::mouseMoveEvent(QMouseEvent* ev)
{
    UpdateCursor(ev);
}

void CTimeline::mouseReleaseEvent(QMouseEvent* ev)
{
    if (ev->button() == Qt::LeftButton || ev->button() == Qt::MiddleButton)
    {
        if (m_mouseHandler.get())
        {
            m_mouseHandler->mouseReleaseEvent(ev);
            m_mouseHandler.reset();
            update();
        }
    }
    UpdateCursor(ev);
}

void CTimeline::focusOutEvent(QFocusEvent* ev)
{
    if (m_mouseHandler.get())
    {
        m_mouseHandler->focusOutEvent(ev);
        m_mouseHandler.reset();
    }

    update();
}

void CTimeline::wheelEvent(QWheelEvent* ev)
{
    int pixelDelta = ev->pixelDelta().manhattanLength();

    if (pixelDelta == 0)
    {
        pixelDelta = ev->angleDelta().y();
    }

    const float fractionOfView = std::min(m_viewState.widthPixels != 0 ? float(pixelDelta) / m_viewState.widthPixels : 0.0f, 0.5f);

    SetVisibleDistance(m_viewState.visibleDistance - m_viewState.visibleDistance * fractionOfView);
}

QSize CTimeline::sizeHint() const
{
    return QSize(m_layout->size);
}

void CTimeline::resizeEvent([[maybe_unused]] QResizeEvent* ev)
{
    UpdateLayout();
}

SAnimTime CTimeline::ClampAndSnapTime(SAnimTime time, bool snapToFrames) const
{
    SAnimTime minTime = m_layout->minStartTime;
    SAnimTime maxTime = m_layout->maxEndTime;
    SAnimTime unclampedTime = time;
    SAnimTime deltaTime = maxTime - minTime;

    if (m_cycled)
    {
        while (unclampedTime < minTime)
        {
            unclampedTime += deltaTime;
        }

        unclampedTime = ((unclampedTime - minTime) % deltaTime) + minTime;
    }

    SAnimTime clampedTime = clamp_tpl(unclampedTime, minTime, maxTime);

    if (!snapToFrames)
    {
        return clampedTime;
    }
    else
    {
        int timeStepIndex = static_cast<int>(floor(clampedTime.ToFloat() * static_cast<float>(m_timeStepNum) + 0.05f));
        float normalizedTime = static_cast<float>(timeStepIndex) / static_cast<float>(m_timeStepNum);
        return SAnimTime(normalizedTime);
    }
}

void CTimeline::ClampAndSetTime(SAnimTime time, bool scrubThrough)
{
    SAnimTime newTime = ClampAndSnapTime(time, m_snapTime);

    if (newTime != m_time)
    {
        m_time = newTime;
        UpdateLayout();
        update();
        SignalScrub(scrubThrough);
    }
}

void CTimeline::SetTimeUnitScale(float scale, float step)
{
    m_timeUnitScale = scale;
    m_timeStepNum = static_cast<int>(scale / step);
    update();
}

void CTimeline::SetTime(SAnimTime time)
{
    m_time = time;
    update();
}

void CTimeline::SetCycled(bool cycled)
{
    m_cycled = cycled;
}

void CTimeline::SetContent(STimelineContent* pContent)
{
    m_pContent = pContent;

    UpdateLayout();
    update();
}

void CTimeline::UpdateLayout()
{
    m_layout->tracks.clear();

    m_viewState.widthPixels = width();

    if (m_treeVisible)
    {
        m_viewState.widthPixels -= m_viewState.treeWidth;
        m_viewState.widthPixels = std::max(m_viewState.widthPixels, 0);
    }

    if (m_verticalScrollbarVisible)
    {
        if (!m_scrollBar)
        {
            m_scrollBar = new QScrollBar(Qt::Vertical, this);
            connect(m_scrollBar, SIGNAL(valueChanged(int)), this, SLOT(OnVerticalScroll(int)));
        }

        const uint scrollbarWidth = style()->pixelMetric(QStyle::PM_ScrollBarExtent, 0, this);
        m_scrollBar->setGeometry(width() - scrollbarWidth, 0, scrollbarWidth, height());

        m_viewState.widthPixels -= scrollbarWidth;
        m_viewState.widthPixels = std::max(m_viewState.widthPixels, 0);
    }
    else if (!m_verticalScrollbarVisible && m_scrollBar)
    {
        SAFE_DELETE(m_scrollBar);
    }

    ClampViewOrigin(&m_viewState, *m_layout);

    if (m_pContent)
    {
        CalculateLayout(m_layout.get(), *m_pContent, m_viewState, m_pFilterLineEdit, m_time.ToFloat(), m_keyWidth, m_treeVisible);
        ApplyPushOut(m_layout.get(), m_keyWidth);
    }

    if (m_scrollBar)
    {
        const int timelineHeight = rect().height();
        const int scrollBarRange = m_layout->size.height() - timelineHeight;

        if (scrollBarRange > 0)
        {
            m_scrollBar->setRange(0, scrollBarRange);
            m_scrollBar->show();
        }
        else
        {
            m_scrollBar->setValue(0);
            m_scrollBar->hide();
        }
    }

    if (m_sizeToContent)
    {
        setMaximumHeight(m_layout->size.height());
        setMinimumHeight(m_layout->size.height());
    }
    else
    {
        setMinimumHeight(RULER_HEIGHT + 1);
        setMaximumHeight(QWIDGETSIZE_MAX);
    }

    if (m_treeVisible)
    {
        if (!m_pFilterLineEdit)
        {
            m_pFilterLineEdit = new QLineEdit(this);
            connect(m_pFilterLineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(OnFilterChanged()));
        }

        const uint cornerWidgetWidth = m_cornerWidget ? m_cornerWidgetWidth : 0;
        m_pFilterLineEdit->resize(m_viewState.treeWidth - SPLITTER_WIDTH - cornerWidgetWidth, RULER_HEIGHT + VERTICAL_PADDING);

        if (m_cornerWidget)
        {
            m_cornerWidget->setGeometry(m_viewState.treeWidth - SPLITTER_WIDTH - cornerWidgetWidth, 0, cornerWidgetWidth, RULER_HEIGHT + VERTICAL_PADDING);
        }
    }
    else if (!m_treeVisible && m_pFilterLineEdit)
    {
        SAFE_DELETE(m_pFilterLineEdit);
    }
}

void CTimeline::SetSizeToContent(bool sizeToContent)
{
    m_sizeToContent = sizeToContent;

    UpdateLayout();
}

void CTimeline::ContentChanged(bool continuous)
{
    SignalContentChanged(continuous);

    DeletedMarkedElements(m_pContent->track);

    if (!continuous)
    {
        ForEachElement(m_pContent->track, [](STimelineTrack& track, STimelineElement& element)
            {
                track.modified = false;
                element.added = false;
            });
    }

    UpdateLayout();
    update();
}

void CTimeline::OnMenuSelectionToCursor()
{
    TSelectedElements elements = GetSelectedElements(m_pContent->track);

    for (size_t i = 0; i < elements.size(); ++i)
    {
        STimelineTrack& track = *elements[i].first;
        STimelineElement& element = *elements[i].second;
        SAnimTime length = element.end - element.start;
        element.start = m_time;
        element.end = element.start + length;
        if (element.type == element.CLIP)
        {
            if (length > track.endTime)
            {
                element.start = track.endTime - length;
            }
        }
        if (element.start < track.startTime)
        {
            element.start = track.startTime;
        }
    }

    ContentChanged(false);
}

void CTimeline::OnMenuDuplicate()
{
    TSelectedElements selectedElements = GetSelectedElements(m_pContent->track);
    if (selectedElements.empty())
    {
        return;
    }

    typedef std::vector<std::pair<STimelineTrack*, STimelineElement> > TTrackElements;

    TTrackElements elements;

    ForEachElement(m_pContent->track, [&](STimelineTrack& track, STimelineElement& element)
        {
            if (element.selected)
            {
                elements.push_back(std::make_pair(&track, element));
                element.selected = false;
            }
        });

    for (size_t i = 0; i < elements.size(); ++i)
    {
        STimelineTrack* track = elements[i].first;
        const STimelineElement& element = elements[i].second;
        track->elements.push_back(element);
        STimelineElement& e = track->elements.back();
        e.userId = 0;
        e.added = true;
        e.sideLoadChanged = true;
        e.selected = true;
    }

    ContentChanged(false);
    SignalSelectionChanged(false);
}

void CTimeline::OnMenuCopy()
{
}

void CTimeline::OnMenuPaste()
{
}

void CTimeline::OnMenuDelete()
{
    ForEachElement(m_pContent->track, [](STimelineTrack& track, STimelineElement& element)
        {
            if (element.selected)
            {
                track.modified = true;
                element.deleted = true;
            }
        });

    ContentChanged(false);
}

void CTimeline::OnMenuPlay()
{
    SignalPlay();
}

typedef std::vector<std::pair<SAnimTime, STimelineContentElementRef> > TimeToId;
static void GetAllTimes(TimeToId* times, const STimelineTrack& track)
{
    for (size_t i = 0; i < track.tracks.size(); ++i)
    {
        GetAllTimes(times, *track.tracks[i]);
    }
}

static void GetAllTimes(TimeToId* times, STimelineContent& content)
{
    ForEachTrack(content.track, [&](STimelineTrack& track)
        {
            times->push_back(std::make_pair(track.startTime, STimelineContentElementRef()));
            times->push_back(std::make_pair(track.endTime, STimelineContentElementRef()));
        });

    ForEachElementWithIndex(content.track, [=](STimelineTrack& track, STimelineElement& element, size_t i)
        {
            STimelineContentElementRef ref(&track, i);
            times->push_back(std::make_pair(element.start, ref));
            if (element.type == STimelineElement::CLIP)
            {
                times->push_back(std::make_pair(element.end, ref));
            }
        });

    std::sort(times->begin(), times->end());
}

static STimelineContentElementRef SelectedIdAtTime(const std::vector<STimelineContentElementRef>& selection, [[maybe_unused]] const STimelineContent& content, SAnimTime time)
{
    for (size_t i = 0; i < selection.size(); ++i)
    {
        const STimelineContentElementRef& id = selection[i];
        const STimelineElement& element = id.GetElement();
        if (element.start == time || element.end == time)
        {
            return id;
        }
    }
    return STimelineContentElementRef();
}

void CTimeline::OnMenuPreviousKey()
{
    if (!m_pContent)
    {
        return;
    }
    TimeToId times;
    GetAllTimes(&times, *m_pContent);

    std::vector<STimelineContentElementRef> selection;
    ForEachElementWithIndex(m_pContent->track, [&](STimelineTrack& t, STimelineElement& e, size_t i)
        {
            if (e.selected)
            {
                selection.push_back(STimelineContentElementRef(&t, i));
            }
        });

    STimelineContentElementRef selectedId = SelectedIdAtTime(selection, *m_pContent, m_time);

    TimeToId::iterator it = std::lower_bound(times.begin(), times.end(), std::make_pair(m_time, selectedId));
    if (it != times.end())
    {
        if (it != times.begin())
        {
            --it;
        }

        ClearElementSelection(m_pContent->track);
        if (it->second.IsValid())
        {
            it->second.GetElement().selected = true;
        }

        m_time = it->first;

        SignalSelectionChanged(false);
        SignalScrub(false);
        update();
    }
}

void CTimeline::OnMenuNextKey()
{
    if (!m_pContent)
    {
        return;
    }
    TimeToId times;
    GetAllTimes(&times, *m_pContent);

    std::vector<STimelineContentElementRef> selection;
    ForEachElementWithIndex(m_pContent->track, [&](STimelineTrack& t, STimelineElement& e, size_t i)
        {
            if (e.selected)
            {
                selection.push_back(STimelineContentElementRef(&t, i));
            }
        });

    STimelineContentElementRef selectedId = SelectedIdAtTime(selection, *m_pContent, m_time);

    TimeToId::iterator it = std::upper_bound(times.begin(), times.end(), std::make_pair(m_time, selectedId));
    if (it != times.end())
    {
        ClearElementSelection(m_pContent->track);
        if (it->second.IsValid())
        {
            it->second.GetElement().selected = true;
        }

        m_time = it->first;

        SignalSelectionChanged(false);
        SignalScrub(false);
        update();
    }
}

void CTimeline::OnMenuPreviousFrame()
{
    m_timeStepIndex = static_cast<int>(floor(m_time.ToFloat() * static_cast<float>(m_timeStepNum) + 0.05f)) - 1;
    if (m_timeStepIndex < 0)
    {
        m_timeStepIndex = m_timeStepNum;
    }
    float normalizedTime = static_cast<float>(m_timeStepIndex) / static_cast<float>(m_timeStepNum);
    m_time = SAnimTime(normalizedTime);
    SignalScrub(false);
    update();
}

void CTimeline::OnMenuNextFrame()
{
    m_timeStepIndex = static_cast<int>(floor(m_time.ToFloat() * static_cast<float>(m_timeStepNum) + 0.05f)) + 1;
    if (m_timeStepIndex > m_timeStepNum)
    {
        m_timeStepIndex = 0;
    }
    float normalizedTime = static_cast<float>(m_timeStepIndex) / static_cast<float>(m_timeStepNum);
    m_time = SAnimTime(normalizedTime);
    SignalScrub(false);
    update();
}

void CTimeline::OnFilterChanged()
{
    UpdateLayout();
    update();
}

void CTimeline::OnVerticalScroll([[maybe_unused]] int value)
{
    update();
}

bool CTimeline::event(QEvent* e)
{
    switch (e->type())
    {
        case QEvent::ShortcutOverride:
        {
            // When a shortcut is matched, Qt's event processing sends out a shortcut override event
            // to allow other systems to override it. If it's not overridden, then the key events
            // get processed as a shortcut, even if the widget that's the target has a keyPress event
            // handler. So, we need to communicate that we've processed the shortcut override
            // which will tell Qt not to process it as a shortcut and instead pass along the
            // keyPressEvent.

            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
            QKeySequence keySequence = keyEvent->key() | keyEvent->modifiers();

            // special case undo/redo, because they're only handled in CTimeline::keyPressEvent()
            // and not in HandleKeyEvent:
            static QSet<QKeySequence> customShortcuts = {
                QKeySequence(Qt::CTRL | Qt::Key_Z),
                QKeySequence(Qt::CTRL | Qt::Key_Y),
                QKeySequence(Qt::Key_Z | Qt::CTRL | Qt::SHIFT)
            };

            if (ProcessesKey(keySequence) || customShortcuts.contains(keySequence))
            {
                e->accept();
                return true;
            }
        }
        break;
    }

    return QWidget::event(e);
}

void CTimeline::SetTreeVisible(bool visible)
{
    m_treeVisible = visible;
    m_viewState.treeWidth = visible ? DEFAULT_TREE_WIDTH : 0;

    UpdateLayout();
    update();
}

STrackLayout* CTimeline::GetTrackLayoutFromPos(const QPoint& pos) const
{
    if (pos.y() < RULER_HEIGHT)
    {
        return nullptr;
    }

    STrackLayouts& tracks = m_layout->tracks;

    auto findIter = std::upper_bound(tracks.begin(), tracks.end(), pos.y(), [&](int y, const STrackLayout& track)
            {
                return y < track.rect.bottom();
            });

    if (findIter != tracks.end() && pos.y() <= findIter->rect.bottom())
    {
        return &(*findIter);
    }

    return nullptr;
}

void CTimeline::SetCustomTreeCornerWidget(QWidget* pWidget, uint width)
{
    SAFE_DELETE(m_cornerWidget);

    m_cornerWidget = pWidget;
    m_cornerWidgetWidth = width;

    if (m_cornerWidget)
    {
        m_cornerWidget->setCursor(QCursor());
    }

    UpdateLayout();
    update();
}

void CTimeline::SetVerticalScrollbarVisible(bool bVisible)
{
    m_verticalScrollbarVisible = bVisible;
    UpdateLayout();
    update();
}

void CTimeline::SetDrawTrackTimeMarkers(bool bDrawMarkers)
{
    m_drawMarkers = bDrawMarkers;
    update();
}

void CTimeline::SetVisibleDistance(float distance)
{
    const float totalDuration = (m_layout->maxEndTime - m_layout->minStartTime).ToFloat();
    const float padding = (float(TIMELINE_PADDING) - 0.5f) / m_viewState.widthPixels * totalDuration;
    m_viewState.visibleDistance = clamp_tpl(distance, 0.01f, totalDuration + 2.0f * padding);

    UpdateLayout();
    update();
}
