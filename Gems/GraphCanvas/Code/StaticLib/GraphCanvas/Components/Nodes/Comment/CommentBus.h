/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <qrect.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>

#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Types/EntitySaveData.h>
#include <GraphCanvas/Types/Types.h>
#include <GraphCanvas/Utils/StateControllers/StateController.h>

class QGraphicsLayoutItem;

namespace GraphCanvas
{
    class Scene;

    enum class CommentMode
    {
        Unknown,
        Comment,
        BlockComment
    };

    //! CommentRequests
    //! Requests that get or set the properties of a comment.
    class CommentRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Set the name of the comment. This often acts as a kind of visual title for the comment.
        virtual void SetComment(const AZStd::string&) = 0;
        //! Get the name of the comment.
        virtual const AZStd::string& GetComment() const = 0;

        //! Sets the type of comment that is being used(controls how the comment resizes, how excess text is handled).
        virtual void SetCommentMode(CommentMode commentMode) = 0;

        //! Sets the background color for the comment
        virtual void SetBackgroundColor(const AZ::Color& color) = 0;

        //! Returns the background color set for the comment
        virtual AZ::Color GetBackgroundColor() const = 0;
    };

    using CommentRequestBus = AZ::EBus<CommentRequests>;

    //! CommentNotifications
    //! Notifications about changes to the state of comments
    class CommentNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Signals when the comment begins being edited.
        virtual void OnEditBegin() {}

        //! Signals when the comment ends being edited
        virtual void OnEditEnd() {}

        //! When the comment is changed, this is emitted.
        virtual void OnCommentChanged(const AZStd::string&) {}

        //! Emitted when the size of a comment changes(in reaction to text updating)
        virtual void OnCommentSizeChanged(const QSizeF& /*oldSize*/, const QSizeF& /*newSize*/) {};

        virtual void OnCommentFontReloadBegin() {};
        virtual void OnCommentFontReloadEnd() {};

        virtual void OnBackgroundColorChanged(const AZ::Color& /*color*/) {};
    };

    using CommentNotificationBus = AZ::EBus<CommentNotifications>;

    class CommentUIRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void SetEditable(bool editable) = 0;
    };

    using CommentUIRequestBus = AZ::EBus<CommentUIRequests>;

    class CommentLayoutRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual QGraphicsLayoutItem* GetGraphicsLayoutItem() = 0;
    };

    using CommentLayoutRequestBus = AZ::EBus<CommentLayoutRequests>;

    class CommentNodeTextSaveDataInterface
    {
    public:

        virtual void OnCommentChanged() = 0;
        virtual void OnBackgroundColorChanged() = 0;
        virtual void UpdateStyleOverrides() = 0;

        virtual CommentMode GetCommentMode() const = 0;
    };

    class CommentNodeTextSaveData
        : public ComponentSaveData
    {
    public:
        AZ_RTTI(CommentNodeTextSaveData, "{524D8380-AC09-444E-870E-9CEF2535B4A2}", ComponentSaveData);
        AZ_CLASS_ALLOCATOR(CommentNodeTextSaveData, AZ::SystemAllocator);

        CommentNodeTextSaveData()
            : m_saveDataInterface(nullptr)
            , m_backgroundColor(1.0f, 1.0f, 1.0f, 1.0f)
        {
        }

        CommentNodeTextSaveData(CommentNodeTextSaveDataInterface* saveDataInterface)
            : m_saveDataInterface(saveDataInterface)
            , m_backgroundColor(1.0f, 1.0f, 1.0f, 1.0f)
        {
        }

        void operator=(const CommentNodeTextSaveData& other)
        {
            // Purposefully skipping over the callback.
            m_comment = other.m_comment;
            m_fontConfiguration = other.m_fontConfiguration;
            m_backgroundColor = other.m_backgroundColor;
        }

        void OnCommentChanged()
        {
            if (m_saveDataInterface)
            {
                m_saveDataInterface->OnCommentChanged();
                SignalDirty();
            }
        }

        void OnBackgroundColorChanged()
        {
            if (m_saveDataInterface)
            {
                m_saveDataInterface->OnBackgroundColorChanged();
                SignalDirty();
            }
        }

        void UpdateStyleOverrides()
        {
            if (m_saveDataInterface)
            {
                m_saveDataInterface->UpdateStyleOverrides();
                SignalDirty();
            }
        }

        AZStd::string GetCommentLabel() const
        {
            if (m_saveDataInterface)
            {
                switch (m_saveDataInterface->GetCommentMode())
                {
                case CommentMode::BlockComment:
                {
                    return "Group Name";
                }
                case CommentMode::Comment:
                {
                    return "Comment";
                }
                default:
                {
                    break;
                }
                }
            }

            return "Title";
        }

        AZStd::string GetBackgroundLabel() const
        {
            if (m_saveDataInterface)
            {
                switch (m_saveDataInterface->GetCommentMode())
                {
                case CommentMode::BlockComment:
                {
                    return "Group Color";
                }
                case CommentMode::Comment:
                {
                    return "Background Color";
                }
                default:
                {
                    break;
                }
                }
            }

            return "Background Color";
        }

        AZStd::string m_comment;
        AZ::Color     m_backgroundColor;
        FontConfiguration m_fontConfiguration;

    private:
        CommentNodeTextSaveDataInterface* m_saveDataInterface;
    };
}
