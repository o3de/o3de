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

package com.amazon.lumberyard.NativeUI;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.util.Log;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicReference;

public class LumberyardNativeUI
{
    public static void DisplayDialog(final Activity activity, final String title, final String message, final String[] options)
    {
        Log.d("LMBR", "DisplayDialog called");

        userSelection = new AtomicReference<String>("");
        userSelection.set("");

        Runnable uiDialog = new Runnable()
        {
            public void run()
            {
                AlertDialog.Builder builder = new AlertDialog.Builder(activity);
                TextView textView = new TextView(activity);
                textView.setText(title + "\n" + message);
                builder.setCustomTitle(textView);
                builder.setItems(options, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int index) {
                        userSelection.set(options[index]);
                        Log.d("LMBR", "Selected option: " + userSelection.get());
                    }
                });
                AlertDialog dialog = builder.create();
                dialog.show();
            }
        };
    
        activity.runOnUiThread(uiDialog);
    }

    public static String GetUserSelection()
    {
        return userSelection.get();
    }

    public static AtomicReference<String> userSelection;
}
