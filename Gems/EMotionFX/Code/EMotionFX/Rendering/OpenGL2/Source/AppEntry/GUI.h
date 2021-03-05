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

#include <AzCore/Math/Vector2>

class Widget
{
public:
    Widget(const char* text, int32 x, int32 y, int32 width, int32 height)
    {
        mText               = text;
        mX                  = x;
        mY                  = y;
        mWidth              = width;
        mHeight             = height;
        mVisible            = true;
    }

    virtual ~Widget()                                   { };

    MCORE_INLINE int32 GetX() const                     { return mX; }
    MCORE_INLINE int32 GetY() const                     { return mY; }
    MCORE_INLINE int32 GetWidth() const                 { return mWidth; }
    MCORE_INLINE int32 GetHeight() const                { return mHeight; }

    MCORE_INLINE void SetText(const char* text)         { mText = text; }
    MCORE_INLINE const char* GetText() const            { return mText.AsChar(); }

    MCORE_INLINE void SetVisible(bool visible)          { mVisible = visible; }
    MCORE_INLINE bool IsVisible() const                 { return mVisible; }

    virtual bool Contains(int32 x, int32 y) const
    {
        if (x > GetX() && y > GetY() && x < GetX() + GetWidth() && y < GetY() + GetHeight())
        {
            return true;
        }
        return false;
    }

protected:
    AZStd::string   mText;
    float           mFontSize;
    int32           mX;
    int32           mY;
    int32           mWidth;
    int32           mHeight;
    bool            mVisible;
};


class Label
    : public Widget
{
public:
    Label(const char* text, int32 x, int32 y, float fontSize = 9.0f)
        : Widget(text, x, y, MCORE_INVALIDINDEX32, MCORE_INVALIDINDEX32)
    {
        mFontSize = fontSize;
    }

    MCORE_INLINE float GetFontSize()            { return mFontSize; }

private:
    float mFontSize;
};


class Button
    : public Widget
{
public:
    Button(const char* text, int32 x, int32 y, int32 width, int32 height)
        : Widget(text, x, y, width, height)
    {
        mToggled            = false;
        mToggleMode         = false;
        mOnClickedCallback  = NULL;
    }

    MCORE_INLINE void SetToggled(bool toggled)                      { mToggled = toggled; }
    MCORE_INLINE bool IsToggled() const
    {
        if (mToggleMode == false)
        {
            return false;
        }
        return mToggled;
    }
    MCORE_INLINE void Toggle()                                      { mToggled = !mToggled; }
    MCORE_INLINE void SetToggleMode(bool enable)                    { mToggleMode = enable; }
    MCORE_INLINE bool GetToggleMode() const                         { return mToggleMode; }

    void SetOnClickedHandler(void (* callbackFunction)(Button*))     { mOnClickedCallback = callbackFunction; }
    void OnClicked()
    {
        if (mOnClickedCallback != NULL)
        {
            (*mOnClickedCallback)(this);
        }
    }

private:
    // on clicked handling callback
    void (* mOnClickedCallback)(Button*);

    bool            mToggled;
    bool            mToggleMode;
};


class Checkbox
    : public Widget
{
public:
    Checkbox(const char* text, int32 x, int32 y, int32 width, int32 height)
        : Widget(text, x, y, width, height)
    {
        mText               = text;
        mWidth              = width;
        mHeight             = height;
        mOnClickedCallback  = NULL;
        mChecked            = false;
    }

    MCORE_INLINE void SetChecked(bool checked)                      { mChecked = checked; OnClicked(); }
    MCORE_INLINE bool IsChecked() const                             { return mChecked; }

    void SetOnClickedHandler(void (* callbackFunction)(Checkbox*))   { mOnClickedCallback = callbackFunction; }
    void OnClicked()
    {
        if (mOnClickedCallback != NULL)
        {
            (*mOnClickedCallback)(this);
        }
    }

private:
    // on clicked handling callback
    void (* mOnClickedCallback)(Checkbox*);

    bool            mChecked;
};


class Slider
    : public Widget
{
public:
    enum SliderType
    {
        SLIDERTYPE_INT      = 0,
        SLIDERTYPE_FLOAT    = 1
    };

    Slider(SliderType type, float minValue, float maxValue, float defaultValue, int32 x, int32 y)
        : Widget("Unknown Slider Name", x, y - 20, MCORE_INVALIDINDEX32, MCORE_INVALIDINDEX32)
    {
        mMinValue           = minValue;
        mMaxValue           = maxValue;
        mDefaultValue       = defaultValue;
        mSliding            = false;
        mOnChangedCallback  = NULL;
        mType               = type;
        SetValue(defaultValue);
    }

    MCORE_INLINE int32 Round(float d) const                         { return d < 0 ? d - .5 : d + .5; }
    MCORE_INLINE float GetValue() const
    {
        if (mType == SLIDERTYPE_INT)
        {
            return Round(mValue);
        }
        return mValue;
    }
    MCORE_INLINE float GetFloatValue() const                        { return mValue; }
    MCORE_INLINE float GetMinValue() const                          { return mMinValue; }
    MCORE_INLINE float GetMaxValue() const                          { return mMaxValue; }
    MCORE_INLINE float GetDefaultValue() const                      { return mDefaultValue; }
    MCORE_INLINE const char* GetValueText() const                   { return mValueText.AsChar(); }
    MCORE_INLINE void SetSliding(bool isSliding)                    { mSliding = isSliding; }
    MCORE_INLINE bool IsSliding() const                             { return mSliding; }
    MCORE_INLINE void SetRange(float minValue, float maxValue)      { mMinValue = minValue; mMaxValue = maxValue; }

    void SetValue(float value)
    {
        mValue = value;
        if (mValue < mMinValue)
        {
            mValue = mMinValue;
        }
        if (mValue > mMaxValue)
        {
            mValue = mMaxValue;
        }

        if (mType == SLIDERTYPE_INT)
        {
            mValueText = AZStd::string::format("%.0f", mValue);
        }
        else
        {
            mValueText = AZStd::string::format("%.1f", mValue);
        }

        OnChanged();
    }

    void SetOnChangedHandler(void (* callbackFunction)(Slider*, float))      { mOnChangedCallback = callbackFunction; }
    void OnChanged()
    {
        if (mOnChangedCallback != NULL)
        {
            (*mOnChangedCallback)(this, GetValue());
        }
    }

private:
    // on slider changed handling callback
    void (* mOnChangedCallback)(Slider*, float);

    AZStd::string   mValueText;
    SliderType      mType;
    float           mValue;
    float           mMinValue;
    float           mMaxValue;
    float           mDefaultValue;
    bool            mSliding;
};


class GUIManager
{
public:
    GUIManager()
    {
        mButtons.Reserve(10);
        mCheckboxes.Reserve(10);
        mSliders.Reserve(10);
        mLabels.Reserve(20);
    }

    ~GUIManager()
    {
        uint32 i;

        // get the number of buttons, iterate through and destroy them
        const uint32 numButtons = mButtons.GetLength();
        for (i = 0; i < numButtons; ++i)
        {
            delete mButtons[i];
        }
        mButtons.Clear();

        // get the number of checkboxes, iterate through and destroy them
        const uint32 numCheckboxes = mCheckboxes.GetLength();
        for (i = 0; i < numCheckboxes; ++i)
        {
            delete mCheckboxes[i];
        }
        mCheckboxes.Clear();

        // get the number of sliders, iterate through and destroy them
        const uint32 numSliders = mSliders.GetLength();
        for (i = 0; i < numSliders; ++i)
        {
            delete mSliders[i];
        }
        mSliders.Clear();

        // get the number of labels, iterate through and destroy them
        const uint32 numLabels = mLabels.GetLength();
        for (i = 0; i < numLabels; ++i)
        {
            delete mLabels[i];
        }
        mLabels.Clear();
    }

    void Init()
    {
        mButtonTexture              = gEngine->LoadTexture("../../../Shared/Textures/GUI/Button.png", false);
        mButtonToggledTexture       = gEngine->LoadTexture("../../../Shared/Textures/GUI/ButtonToggled.png", false);
        mCheckboxCheckedTexture     = gEngine->LoadTexture("../../../Shared/Textures/GUI/CheckboxChecked.png", false);
        mCheckboxUncheckedTexture   = gEngine->LoadTexture("../../../Shared/Textures/GUI/CheckboxUnchecked.png", false);
        mSliderButtonTexture        = gEngine->LoadTexture("../../../Shared/Textures/GUI/SliderButton.png", false);
        mSliderTexture              = gEngine->LoadTexture("../../../Shared/Textures/GUI/Slider.png", false);
    }

    void Render()
    {
        uint32 i;

        const MCore::RGBAColor textColor    = MCore::RGBAColor(1.0f, 1.0f, 1.0f, 1.0f);
        const float fontSize                = 9.0f;

        // get the number of buttons, iterate through and render them
        const uint32 numButtons = mButtons.GetLength();
        for (i = 0; i < numButtons; ++i)
        {
            Button* button = mButtons[i];
            if (button->IsVisible() == false)
            {
                continue;
            }

            if (button->IsToggled() == false)
            {
                gEngine->GetRenderUtil()->RenderTexture(mButtonTexture, AZ::Vector2(button->GetX(), button->GetY()));
            }
            else
            {
                gEngine->GetRenderUtil()->RenderTexture(mButtonToggledTexture, AZ::Vector2(button->GetX(), button->GetY()));
            }
        }

        // get the number of checkboxes, iterate through and render them
        const uint32 numCheckboxes = mCheckboxes.GetLength();
        for (i = 0; i < numCheckboxes; ++i)
        {
            Checkbox* checkbox = mCheckboxes[i];
            if (checkbox->IsVisible() == false)
            {
                continue;
            }

            if (checkbox->IsChecked() == true)
            {
                gEngine->GetRenderUtil()->RenderTexture(mCheckboxCheckedTexture, AZ::Vector2(checkbox->GetX(), checkbox->GetY()));
            }
            else
            {
                gEngine->GetRenderUtil()->RenderTexture(mCheckboxUncheckedTexture, AZ::Vector2(checkbox->GetX(), checkbox->GetY()));
            }
        }

        // get the number of sliders, iterate through and render them
        const uint32 numSliders = mSliders.GetLength();
        for (i = 0; i < numSliders; ++i)
        {
            Slider* slider = mSliders[i];
            if (slider->IsVisible() == false)
            {
                continue;
            }

            const float sliderRange     = slider->GetMaxValue() - slider->GetMinValue();
            float sliderButtonX         = slider->GetX();
            const float sliderY         = slider->GetY() + GetDefaultSliderButtonHeight() * 0.5f - GetDefaultSliderHeight() * 0.5f;

            if (sliderRange > MCore::Math::epsilon)
            {
                const float normalizedValue = (slider->GetFloatValue() - slider->GetMinValue()) / sliderRange;
                sliderButtonX   = slider->GetX() + (normalizedValue * (GetDefaultSliderWidth() - GetDefaultSliderButtonWidth()));
            }

            gEngine->GetRenderUtil()->RenderTexture(mSliderTexture, AZ::Vector2(slider->GetX(), sliderY));
            gEngine->GetRenderUtil()->RenderTexture(mSliderButtonTexture, MCore::Vector2(sliderButtonX, slider->GetY()));
        }

        // draw all textures
        gEngine->GetRenderUtil()->RenderTextures();

        // render the text labels on top of the background textures
        for (i = 0; i < numButtons; ++i)
        {
            Button* button          = mButtons[i];
            if (button->IsVisible() == false)
            {
                continue;
            }

            const int32 x           = button->GetX() + button->GetWidth() * 0.5f;
            const int32 y           = button->GetY() + button->GetHeight() * 0.5f - fontSize * 0.5f + 1;

            gRenderUtil->RenderText(x, y, button->GetText(), textColor, fontSize, true);
        }

        for (i = 0; i < numCheckboxes; ++i)
        {
            Checkbox*   checkbox    = mCheckboxes[i];
            if (checkbox->IsVisible() == false)
            {
                continue;
            }

            const int32 x           = checkbox->GetX() + checkbox->GetWidth() + 5;
            const int32 y           = checkbox->GetY() + checkbox->GetHeight() * 0.5f - fontSize * 0.5f + 1;

            gRenderUtil->RenderText(x, y, checkbox->GetText(), textColor, fontSize);
        }

        for (i = 0; i < numSliders; ++i)
        {
            Slider* slider = mSliders[i];
            if (slider->IsVisible() == false)
            {
                continue;
            }

            // slider value moving along with the slider button
            //const float textSize      = gRenderUtil->CalculateTextWidth( slider->GetValueText(), fontSize );
            //const float sliderRange       = slider->GetMaxValue() - slider->GetMinValue();
            //float textX                   = slider->GetX() + GetDefaultSliderButtonWidth()*0.5f - textSize * 0.5;
            //const float textY         = slider->GetY() + GetDefaultSliderButtonHeight() * 0.5f - fontSize + 1;
            //if (sliderRange > MCore::Math::epsilon)
            //{
            //  const float normalizedValue = (slider->GetFloatValue() - slider->GetMinValue()) / sliderRange;
            //  textX                       = slider->GetX() + GetDefaultSliderButtonWidth()*0.5f - textSize * 0.5 + ( normalizedValue * (GetDefaultSliderWidth()-GetDefaultSliderButtonWidth()) );
            //}

            // text right of the slider
            const float textX = slider->GetX() + GetDefaultSliderWidth() + 5;
            const float textY = slider->GetY() + GetDefaultSliderButtonHeight() * 0.5f - fontSize * 0.5f;

            gRenderUtil->RenderText(textX, textY, slider->GetValueText(), textColor, fontSize);
        }

        const uint32 numLabels = mLabels.GetLength();
        for (i = 0; i < numLabels; ++i)
        {
            Label* label = mLabels[i];
            if (label->IsVisible() == false)
            {
                continue;
            }
            gRenderUtil->RenderText(label->GetX(), label->GetY(), label->GetText(), textColor, label->GetFontSize());
        }
    }

    // events
    void OnMouseButtonDown(bool left, bool middle, bool right, int32 x, int32 y)
    {
        uint32 i;

        // get the number of buttons and iterate through
        const uint32 numButtons = mButtons.GetLength();
        for (i = 0; i < numButtons; ++i)
        {
            Button* button = mButtons[i];
            if (button->IsVisible() == false)
            {
                continue;
            }

            if (button->Contains(x, y) == true)
            {
                if (button->GetToggleMode() == true)
                {
                    button->Toggle();
                }

                button->OnClicked();
            }
        }

        // get the number of checkboxes and iterate through them
        const uint32 numCheckboxes = mCheckboxes.GetLength();
        for (i = 0; i < numCheckboxes; ++i)
        {
            Checkbox* checkbox = mCheckboxes[i];
            if (checkbox->IsVisible() == false)
            {
                continue;
            }

            if (checkbox->Contains(x, y) == true)
            {
                if (checkbox->IsChecked() == true)
                {
                    checkbox->SetChecked(false);
                }
                else
                {
                    checkbox->SetChecked(true);
                }

                checkbox->OnClicked();
            }
        }


        // get the number of sliders, iterate through and render them
        const uint32 numSliders = mSliders.GetLength();
        for (i = 0; i < numSliders; ++i)
        {
            Slider* slider = mSliders[i];
            if (slider->IsVisible() == false)
            {
                continue;
            }

            const float sliderRange     = slider->GetMaxValue() - slider->GetMinValue();
            const float normalizedValue = (slider->GetFloatValue() - slider->GetMinValue()) / sliderRange;
            const float sliderButtonX   = slider->GetX() + (normalizedValue * (GetDefaultSliderWidth() - GetDefaultSliderButtonWidth()));

            const int32 sliderMinX = sliderButtonX;
            const int32 sliderMaxX = sliderMinX + GetDefaultSliderButtonWidth();
            const int32 sliderMinY = slider->GetY();
            const int32 sliderMaxY = sliderMinY + GetDefaultSliderButtonHeight();

            if (x > sliderMinX && y > sliderMinY && x < sliderMaxX && y < sliderMaxY)
            {
                slider->SetSliding(true);
            }
        }
    }

    void OnMouseButtonUp(bool left, bool middle, bool right, int32 x, int32 y)
    {
        uint32 i;

        // get the number of sliders, iterate through and render them
        const uint32 numSliders = mSliders.GetLength();
        for (i = 0; i < numSliders; ++i)
        {
            Slider* slider = mSliders[i];
            slider->SetSliding(false);
        }
    }

    void OnMouseMove(int32 x, int32 y, int32 deltaX, int32 deltaY)
    {
        uint32 i;

        // get the number of sliders, iterate through and render them
        const uint32 numSliders = mSliders.GetLength();
        for (i = 0; i < numSliders; ++i)
        {
            Slider* slider = mSliders[i];
            if (slider->IsSliding() == false || slider->IsVisible() == false)
            {
                continue;
            }

            if (x < slider->GetX())
            {
                slider->SetValue(slider->GetMinValue());
                continue;
            }

            if (x > slider->GetX() + GetDefaultSliderWidth())
            {
                slider->SetValue(slider->GetMaxValue());
                continue;
            }

            const float sliderRange     = slider->GetMaxValue() - slider->GetMinValue();
            const float deltaPerPixel   = sliderRange / (GetDefaultSliderWidth() - GetDefaultSliderButtonWidth());

            slider->SetValue(slider->GetMinValue() + (x - slider->GetX() - GetDefaultSliderButtonWidth() / 2) * deltaPerPixel);
        }
    }

    MCORE_INLINE void AddButton(Button* button)                 { mButtons.Add(button); }
    MCORE_INLINE void AddLabel(Label* label)                    { mLabels.Add(label); }
    MCORE_INLINE void AddSlider(Slider* slider)                 { mSliders.Add(slider); }
    MCORE_INLINE void AddCheckbox(Checkbox* checkbox)           { mCheckboxes.Add(checkbox); }

    MCORE_INLINE int32 GetDefaultButtonWidth()                  { return mButtonTexture->GetWidth(); }
    MCORE_INLINE int32 GetDefaultButtonHeight()                 { return mButtonTexture->GetHeight(); }
    MCORE_INLINE int32 GetDefaultCheckboxWidth()                { return mCheckboxCheckedTexture->GetWidth(); }
    MCORE_INLINE int32 GetDefaultCheckboxHeight()               { return mCheckboxCheckedTexture->GetHeight(); }
    MCORE_INLINE int32 GetDefaultSliderButtonWidth()            { return mSliderButtonTexture->GetWidth(); }
    MCORE_INLINE int32 GetDefaultSliderButtonHeight()           { return mSliderButtonTexture->GetHeight(); }
    MCORE_INLINE int32 GetDefaultSliderWidth()                  { return mSliderTexture->GetWidth(); }
    MCORE_INLINE int32 GetDefaultSliderHeight()                 { return mSliderTexture->GetHeight(); }

private:
    MCore::Array<Button*>   mButtons;
    MCore::Array<Checkbox*> mCheckboxes;
    MCore::Array<Slider*>   mSliders;
    MCore::Array<Label*>    mLabels;

    RenderGL::Texture*      mButtonTexture;
    RenderGL::Texture*      mButtonToggledTexture;

    RenderGL::Texture*      mCheckboxCheckedTexture;
    RenderGL::Texture*      mCheckboxUncheckedTexture;

    RenderGL::Texture*      mSliderButtonTexture;
    RenderGL::Texture*      mSliderTexture;
};

GUIManager* gGUIManager = NULL;


Button* AddButton(int32 x, int32 y, const char* text)
{
    Button* button = new Button(text, x, y, gGUIManager->GetDefaultButtonWidth(), gGUIManager->GetDefaultButtonHeight());
    gGUIManager->AddButton(button);
    return button;
}



Label* AddLabel(int32 x, int32 y, const char* text, float fontSize = 9.0f)
{
    Label* label = new Label(text, x, y, fontSize);
    gGUIManager->AddLabel(label);
    return label;
}


void AddTitleLabel(const char* text)
{
    AddLabel(4, 4, text, 11.0f);
}


Checkbox* AddCheckBox(int32 x, int32 y, const char* text)
{
    Checkbox* checkbox = new Checkbox(text, x, y, gGUIManager->GetDefaultCheckboxWidth(), gGUIManager->GetDefaultCheckboxHeight());
    gGUIManager->AddCheckbox(checkbox);
    return checkbox;
}


Label* AddSliderLabel(Slider* slider, const char* text)
{
    int x = slider->GetX();
    int y = slider->GetY() - gGUIManager->GetDefaultButtonHeight() * 0.225f;
    return AddLabel(x, y, text);
}


Slider* AddSlider(int32 x, int32 y, const char* labelText, float minVal, float maxVal, float defaultVal, bool addLabel = true)
{
    Slider* slider = new Slider(Slider::SLIDERTYPE_FLOAT, minVal, maxVal, defaultVal, x, y);
    gGUIManager->AddSlider(slider);

    if (addLabel == true)
    {
        AddSliderLabel(slider, labelText);
    }

    return slider;
}


Slider* AddSlider(int x, int y, const char* labelText, const float minVal, const float maxVal, const float defaultVal, Label** outLabel)
{
    Slider* slider = AddSlider(x, y, labelText, minVal, maxVal, defaultVal, false);
    *outLabel = AddSliderLabel(slider, labelText);
    return slider;
}


Slider* AddIntSlider(int32 x, int32 y, const char* labelText, int32 minVal, int32 maxVal, int32 defaultVal)
{
    Slider* slider = new Slider(Slider::SLIDERTYPE_INT, minVal, maxVal, defaultVal, x, y);
    gGUIManager->AddSlider(slider);
    AddSliderLabel(slider, labelText);
    return slider;
}
