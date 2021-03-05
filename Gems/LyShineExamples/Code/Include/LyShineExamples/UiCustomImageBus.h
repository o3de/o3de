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

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Color.h>

class ISprite;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiCustomImageInterface
    : public AZ::ComponentBus
{
public: // types

    // Coordinates of the points that define the rectangle to draw
    struct UVRect
    {
        AZ_TYPE_INFO(UVRect, "{E134EAE8-52A1-4E43-847B-09E546CC5B95}")

            UVRect()
            : m_left(0.f)
            , m_top(1.f)
            , m_right(1.f)
            , m_bottom(0.f) {}

        UVRect(float left, float top, float right, float bottom)
            : m_left(left)
            , m_top(top)
            , m_right(right)
            , m_bottom(bottom) {}

        float   m_left;
        float   m_top;
        float   m_right;
        float   m_bottom;

        void UnitClamp()
        {
            m_left = FClamp(m_left, 0.0f, 1.0f);
            m_top = FClamp(m_top, 0.0f, 1.0f);
            m_right = FClamp(m_right, 0.0f, 1.0f);
            m_bottom = FClamp(m_bottom, 0.0f, 1.0f);
        }

        bool operator==(const UVRect& rhs) const
        {
            return (m_left == rhs.m_left &&
                    m_right == rhs.m_right &&
                    m_top == rhs.m_top &&
                    m_bottom == rhs.m_bottom);
        }

        bool operator!=(const UVRect& rhs) const
        {
            return !(*this == rhs);
        }
    };

public: // member functions

    virtual ~UiCustomImageInterface() {}

    virtual AZ::Color GetColor() = 0;
    virtual void SetColor(const AZ::Color& color) = 0;

    virtual ISprite* GetSprite() = 0;
    virtual void SetSprite(ISprite* sprite) = 0;

    virtual AZStd::string GetSpritePathname() = 0;
    virtual void SetSpritePathname(AZStd::string spritePath) = 0;

    virtual UVRect GetUVs() = 0;
    virtual void SetUVs(UVRect uvs) = 0;

    virtual bool GetClamp() = 0;
    virtual void SetClamp(bool clamp) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiCustomImageInterface> UiCustomImageBus;
