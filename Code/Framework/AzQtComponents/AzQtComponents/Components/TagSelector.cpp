/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/TagSelector.h>
#include <QComboBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QMouseEvent>


namespace AzQtComponents
{
    TagWidget::TagWidget(const QString& text, QWidget* parent)
        : QPushButton(parent)
    {
        setText(text);
        setMouseTracking(true);

        // Create the close button on the right side.
        setLayoutDirection(Qt::RightToLeft);
        setIcon(QIcon(":/stylesheet/img/titlebarmenu/close.png"));
        
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        connect(this, &QPushButton::clicked, this, &TagWidget::OnClicked);
    }


    void TagWidget::OnClicked()
    {
        if (IsOverCloseButton(m_lastMouseX, m_lastMouseY))
        {
            emit DeleteClicked();
        }
    }


    bool TagWidget::IsOverCloseButton(int localX, int localY)
    {
        Q_UNUSED(localY);
        return (localX > width() - iconSize().width() * 1.5) && (localX < width());
    }


    void TagWidget::mouseMoveEvent(QMouseEvent* event)
    {
        m_lastMouseX = event->x();
        m_lastMouseY = event->y();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    TagWidgetContainer::TagWidgetContainer(QWidget* parent)
        : QWidget(parent)
        , m_widget(nullptr)
    {
        m_layout = new QVBoxLayout();
        m_layout->setMargin(0);
        setLayout(m_layout);

        m_width = 250;
    }


    void TagWidgetContainer::SetWrapWidth(int width)
    {
        m_width = width;
        Reinit(m_tags);
    }

    
    void TagWidgetContainer::Reinit(const QVector<QString>& tags)
    {
        m_tags = tags;

        if (m_widget)
        {
            // Hide the old widget and request deletion.
            m_widget->hide();
            m_widget->deleteLater();
        }

        m_widget = new QWidget(this);

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setAlignment(Qt::AlignLeft);
        vLayout->setMargin(0);

        QHBoxLayout* hLayout = nullptr;
        int usedSpaceInRow = 0;
        const int numTags = m_tags.count();
        for (int i = 0; i < numTags; ++i)
        {
            // Create the new tag widget.
            TagWidget* tagWidget = new TagWidget(m_tags[i]);
            const int tagWidgetWidth = tagWidget->minimumSizeHint().width();

            // Calculate the width we're currently using in the current row. Does the new tag still fit in the current row?
            const bool isRowFull = m_width - usedSpaceInRow - tagWidgetWidth < 0;
            if (isRowFull || i == 0)
            {
                // Add a spacer widget after the last tag widget in a row to push the tag widgets to the left.
                if (i > 0)
                {
                    QWidget* spacerWidget = new QWidget();
                    spacerWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
                    hLayout->addWidget(spacerWidget);
                }

                // Add a new row for the current tag widget.
                hLayout = new QHBoxLayout();
                hLayout->setAlignment(Qt::AlignLeft);
                hLayout->setMargin(0);
                vLayout->addLayout(hLayout);

                // Reset the used space in the row.
                usedSpaceInRow = 0;
            }

            // Calculate the width of the tag widgets including the spacing between them of the current row.
            usedSpaceInRow += tagWidgetWidth + hLayout->spacing();

            // Add the tag widget to the current row.
            hLayout->addWidget(tagWidget);

            // Connect the clicked event of the close button of the tag widget to the remove tag function in the container.
            connect(tagWidget, &TagWidget::DeleteClicked, this, [this, tagWidget]{ RemoveTag(tagWidget->text()); });
        }

        m_widget->setLayout(vLayout);
        m_layout->addWidget(m_widget);
    }


    int TagWidgetContainer::GetNumTags() const
    {
        return m_tags.count();
    }


    const QString& TagWidgetContainer::GetTag(int index) const
    {
        return m_tags[index];
    }


    const QVector<QString>& TagWidgetContainer::GetTags() const
    {
        return m_tags;
    }


    bool TagWidgetContainer::Contains(const QString& tag) const
    {
        return m_tags.contains(tag);
    }

    
    void TagWidgetContainer::AddTag(const QString& tag)
    {
        // Is the tag already present in our container? If so, return directly to avoid duplicates.
        if (Contains(tag))
        {
            return;
        }

        m_tags.push_back(tag);
        Reinit(m_tags);
        emit TagsChanged();
    }


    void TagWidgetContainer::AddTags(const QVector<QString>& selectedTags)
    {
        m_tags.reserve(m_tags.size() + selectedTags.size());

        bool changed = false;
        for (const QString& tag : selectedTags)
        {
            // Is the tag already present in our container? Only add the tag if not to avoid duplicates.
            if (!Contains(tag))
            {
                m_tags.push_back(tag);
                changed = true;
            }
        }

        if (changed)
        {
            Reinit(m_tags);
            emit TagsChanged();
        }
    }

    
    void TagWidgetContainer::RemoveTag(const QString& tag)
    {
        m_tags.removeAll(tag);
        Reinit(m_tags);
        emit TagsChanged();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    TagSelector::TagSelector(QWidget* parent)
        : QWidget(parent)
    {
        Init();
    }


    TagSelector::TagSelector(const QVector<QString>& availableTags, QWidget* parent)
        : TagSelector(parent)
    {
        Reinit(availableTags);
    }


    void TagSelector::Init()
    {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setMargin(0);

        // Add the tag widget container representing the currently selected tags.
        m_tagWidgets = new TagWidgetContainer();
        layout->addWidget(m_tagWidgets);

        // Add the combo box for adding tags to the selection.
        m_combo = new QComboBox();
        m_combo->setEditable(true);
        m_combo->lineEdit()->setPlaceholderText(tr("Enter tag name..."));
        m_combo->lineEdit()->setClearButtonEnabled(true);
        layout->addWidget(m_combo);

        connect(m_combo, SIGNAL(activated(int)), this, SLOT(OnComboActivated(int)));
        connect(m_tagWidgets, &TagWidgetContainer::TagsChanged, this, [this]{ Reinit(); emit TagsChanged(); });
    }


    void TagSelector::Reinit(const QVector<QString>& availableTags)
    {
        m_availableTags = availableTags;
        Reinit();
    }


    void TagSelector::Reinit()
    {
        m_combo->blockSignals(true);
        m_combo->clear();

        // Fill the combo box with all available tags so that we can choose tags from them.
        for (const QString& availableTag : m_availableTags)
        {
            // Do not show tags in the combobox that have already been selected.
            if (!IsTagSelected(availableTag))
            {
                m_combo->addItem(availableTag);
            }
        }

        m_combo->setCurrentText("");
        m_combo->blockSignals(false);
    }


    bool TagSelector::IsTagSelected(const QString& tag) const
    {
        return m_tagWidgets->Contains(tag);
    }


    void TagSelector::SelectTag(const QString& tag)
    {
        // Is the tag available?
        if (m_availableTags.indexOf(tag) == -1)
        {
            return;
        }

        // Add a tag widget to the container. The tag widgets represent the currently selected tags.
        m_tagWidgets->AddTag(tag);
    }


    void TagSelector::SelectTags(const QVector<QString>& selectedTags)
    {
        QVector<QString> checkedTags;
        checkedTags.reserve(selectedTags.size());

        for (const QString& tag : selectedTags)
        {
            // Is the tag available?
            if (m_availableTags.indexOf(tag) == -1)
            {
                continue;
            }

            checkedTags.push_back(tag);
        }

        m_tagWidgets->Reinit(checkedTags);
        Reinit();

        emit TagsChanged();
    }


    // Called when pressing enter in the combo box.
    void TagSelector::OnComboActivated(int index)
    {
        if (index < 0 || index >= m_combo->count())
        {
            return;
        }

        QString tag = m_combo->itemText(index);
        if (tag.isEmpty())
        {
            return;
        }

        // If the tag is not available, remove it so that it doesn't appear in the combo box.
        if (m_availableTags.indexOf(tag) == -1)
        {
            m_combo->removeItem(index);

            // Clear the text as the tag was not available.
            m_combo->setCurrentText("");
            return;
        }

        // Add a tag widget to the container. The tag widgets represent the currently selected tags.
        m_tagWidgets->AddTag(tag);

        // Clear the text as the tag got added to selection.
        m_combo->setCurrentText("");
    }


    void TagSelector::GetSelectedTagStrings(QVector<QString>& outTags) const
    {
        outTags = m_tagWidgets->GetTags();
    }
} // namespace AzQtComponents

#include "Components/moc_TagSelector.cpp"
