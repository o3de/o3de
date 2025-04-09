/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            // Asset manager doesn't handle '.' as a directory expression, so replace it with '' if it is encountered
            path = (path.equals( ".")) ? "" : path;
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
