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
package com.amazon.lumberyard.io;

import android.content.res.AssetManager;
import android.util.Log;
import java.io.IOException;
import android.app.Activity;


////////////////////////////////////////////////////////////////
public class APKHandler
{
    ////////////////////////////////////////////////////////////////
    public static void SetAssetManager(AssetManager assetManager)
    {
        s_assetManager = assetManager;
    }

    ////////////////////////////////////////////////////////////////
    public static String[] GetFilesAndDirectoriesInPath(String path)
    {
        String[] filelist = {};

        try
        {
            filelist = s_assetManager.list(path);
        }
        catch (IOException e)
        {
            Log.e(s_tag, String.format("File I/O error: %s", e.getMessage()));
            e.printStackTrace();
        }
        finally
        {
            if (s_debug)
            {
                Log.d(s_tag, String.format("Files in path: %s", path));
                for(String name : filelist)
                {
                    Log.d(s_tag, String.format(" -- %s", name));
                }
            }

            return filelist;
        }
    }

    ////////////////////////////////////////////////////////////////
    public static boolean IsDirectory(String path)
    {
        String[] filelist = {};
        boolean retVal = false;

        try
        {
            filelist = s_assetManager.list(path);

            if(filelist.length > 0)
            {
                retVal = true;
            }
        }
        catch (IOException e)
        {
            Log.e(s_tag, String.format("File I/O error: %s", e.getMessage()));
            e.printStackTrace();
        }
        finally
        {
            return retVal;
        }
    }


    // ----

    private static final String s_tag = "LMBR";

    private static AssetManager s_assetManager = null;
    private static boolean s_debug = false;
}