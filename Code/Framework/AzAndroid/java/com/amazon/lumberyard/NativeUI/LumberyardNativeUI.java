/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
