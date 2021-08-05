/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzQtComponents/Components/TagSelector.h>
#include <QWidget>
#endif

namespace EMotionFX
{
    class TagSelector
        : public QWidget
    {
        Q_OBJECT // AUTOMOC
    public:
        AZ_CLASS_ALLOCATOR_DECL

        TagSelector(QWidget* parent);

        void Reinit();

        void SetTags(const AZStd::vector<AZStd::string>& tags);
        const AZStd::vector<AZStd::string>& GetTags() const;

    protected:
        virtual void GetAvailableTags(QVector<QString>& outTags) const = 0;

    signals:
        void TagsChanged();

    private slots:
        void OnSelectedTagsChanged();

    private:
        void GetSelectedTags(QVector<QString>& outTags) const;

        AZStd::vector<AZStd::string> m_tags;
        AzQtComponents::TagSelector* m_tagSelector = nullptr;
    };
} // namespace EMotionFX
