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
package com.amazon.lumberyard.input;


import android.app.Activity;
import android.content.Context;
import android.util.Log;
import android.view.KeyEvent;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.view.View;


////////////////////////////////////////////////////////////////
public class KeyboardHandler
{
    // ----
    // KeyboardHandler (public)
    // ----

    public static native void SendUnicodeText(String unicodeText);


    ////////////////////////////////////////////////////////////////
    public KeyboardHandler(Activity activity)
    {
        m_activity = activity;
        m_inputManager = (InputMethodManager)m_activity.getSystemService(Context.INPUT_METHOD_SERVICE);
    }

    ////////////////////////////////////////////////////////////////
    public void ShowTextInput()
    {
        m_activity.runOnUiThread(new Runnable() {
            @Override
            public void run()
            {
                if (m_textView == null)
                {
                    m_textView = new DummyTextView(m_activity);

                    ViewGroup viewGroup = (ViewGroup)GetView();
                    viewGroup.addView(m_textView);
                }

                m_textView.Show();
                m_inputManager.showSoftInput(m_textView, 0);
            }
        });
    }

    ////////////////////////////////////////////////////////////////
    public void HideTextInput()
    {
        if (m_textView != null)
        {
            m_activity.runOnUiThread(new Runnable() {
                @Override
                public void run()
                {
                    m_inputManager.hideSoftInputFromWindow(m_textView.getWindowToken(), 0);
                    m_textView.Hide();
                }
            });
        }
    }

    ////////////////////////////////////////////////////////////////
    public boolean IsShowing()
    {
        if (m_textView != null)
        {
            return m_textView.IsShowing();
        }
        return false;
    }


    // ----

    private class DummyTextView extends View
    {
        ////////////////////////////////////////////////////////////////
        public DummyTextView(Context context)
        {
            super(context);

            setFocusableInTouchMode(true);
            setFocusable(true);

            m_isShowing = false;
        }

        ////////////////////////////////////////////////////////////////
        @Override
        public boolean onKeyDown(int keyCode, KeyEvent event)
        {
            if (event.isPrintingKey())
            {
                int unicode = event.getUnicodeChar();
                String character = String.valueOf((char)unicode);

                Log.d(s_tag, String.format("OnKeyDown - Unicode: %s - Printed character: %s", unicode, character));
                SendUnicodeText(character);
            }
            return super.onKeyDown(keyCode, event);
        }

        ////////////////////////////////////////////////////////////////
        @Override
        public boolean onKeyMultiple(int keyCode, int count, KeyEvent event)
        {
            if(event.getAction() == KeyEvent.ACTION_MULTIPLE && keyCode == KeyEvent.KEYCODE_UNKNOWN)
            {
                String text = event.getCharacters();
                Log.d(s_tag, String.format("onKeyMultiple - Text: %s", text));
                if (text != null)
                {
                    SendUnicodeText(text);
                }
            }
            return super.onKeyMultiple(keyCode, count, event);
        }

        ////////////////////////////////////////////////////////////////
        @Override
        public boolean onKeyPreIme(int keyCode, KeyEvent event)
        {
            if (keyCode == KeyEvent.KEYCODE_BACK)
            {
                Hide();
            }

            return super.onKeyPreIme(keyCode, event);
        }

        ////////////////////////////////////////////////////////////////
        public boolean IsShowing()
        {
            return m_isShowing;
        }

        ////////////////////////////////////////////////////////////////
        public void Show()
        {
            m_windowFlags = GetView().getSystemUiVisibility();

            setVisibility(View.VISIBLE);
            requestFocus();
            m_isShowing = true;
        }

        ////////////////////////////////////////////////////////////////
        public void Hide()
        {
            setVisibility(View.GONE);
            m_isShowing = false;

            GetView().setSystemUiVisibility(m_windowFlags);
        }


        // ----

        private boolean m_isShowing;
        private int m_windowFlags;
    }


    // ----

    ////////////////////////////////////////////////////////////////
    private View GetView()
    {
        return m_activity.getWindow().getDecorView();
    }


    // ----

    private static final String s_tag = "KeyboardHandler";

    private Activity m_activity;
    private InputMethodManager m_inputManager;
    private DummyTextView m_textView;
}