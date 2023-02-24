/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <Editor/TagSelector.h>
#include <QHBoxLayout>
#include <QSignalBlocker>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(TagSelector, AZ::SystemAllocator)

    TagSelector::TagSelector(QWidget* parent)
        : QWidget(parent)
    {
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);

        m_tagSelector = new AzQtComponents::TagSelector(this);
        connect(m_tagSelector, &AzQtComponents::TagSelector::TagsChanged, this, &TagSelector::OnSelectedTagsChanged);
        hLayout->addWidget(m_tagSelector);

        setLayout(hLayout);
    }

    void TagSelector::Reinit()
    {
        SetTags(m_tags);
    }

    void TagSelector::SetTags(const AZStd::vector<AZStd::string>& tags)
    {
        m_tags = tags;

        // Get the list of available tags and update the tag selector.
        QVector<QString> availableTags;
        GetAvailableTags(availableTags);
        m_tagSelector->Reinit(availableTags);

        // Get the tag strings from the array of string attributes for selection.
        QVector<QString> tagStrings;
        GetSelectedTags(tagStrings);
        
        QSignalBlocker tagSelectorSignalBlocker(m_tagSelector);
        m_tagSelector->SelectTags(tagStrings);
    }

    const AZStd::vector<AZStd::string>& TagSelector::GetTags() const
    {
        return m_tags;
    }

    void TagSelector::OnSelectedTagsChanged()
    {
        // Get the currently selected tag strings from the widget.
        QVector<QString> tagStrings;
        m_tagSelector->GetSelectedTagStrings(tagStrings);
        const int numTags = tagStrings.count();

        AZStd::vector<AZStd::string> newTags;
        newTags.reserve(numTags);
        for (const QString& tagString : tagStrings)
        {
            newTags.emplace_back(tagString.toUtf8().data());
        }

        m_tags = newTags;
        emit TagsChanged();
    }

    void TagSelector::GetSelectedTags(QVector<QString>& outTags) const
    {
        outTags.clear();
        outTags.reserve(static_cast<int>(m_tags.size()));

        for (const AZStd::string& tag : m_tags)
        {
            outTags.push_back(tag.c_str());
        }
    }
} // namespace EMotionFX
