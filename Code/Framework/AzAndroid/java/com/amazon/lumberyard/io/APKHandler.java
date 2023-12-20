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
import android.content.Context;
import java.io.File;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.file.*;

////////////////////////////////////////////////////////////////
public class APKHandler
{
    ////////////////////////////////////////////////////////////////
    private static void copyFile(InputStream in, OutputStream out) throws IOException
    {
        byte[] buffer = new byte[64*1024];
        int read;
        while ((read = in.read(buffer)) != -1)
        {
            out.write(buffer, 0, read);
        }
    }

    ////////////////////////////////////////////////////////////////
    private static boolean isDir(String fileName)
    {
        Path path = new File(fileName).toPath();
        if (!Files.exists(path))
        {
            return false;
        }
        return Files.isDirectory(path);
    }

    ////////////////////////////////////////////////////////////////
    private static void copyPakFiles(AssetManager mgr, String apkAssetsRelativePath, Context context)
    {
        String outputDir = context.getFilesDir().getAbsolutePath();
        copyAssetFiles(mgr, apkAssetsRelativePath, outputDir, ".pak");
    }

    ////////////////////////////////////////////////////////////////
    private static void copyAssetFiles(AssetManager mgr, String apkAssetsRelativePath, String outputDir, String fileMask)
    {
        try 
        {
            String[] list = mgr.list(apkAssetsRelativePath);
            if (list == null)
            {
                Log.i(s_tag, String.format("No asset files found in '%s'", apkAssetsRelativePath));
                return;
            }

            for (String filename : list)
            {
                if (!isDir(filename) && filename.endsWith(fileMask))
                {
                    InputStream in = null;
                    OutputStream out = null;
                    try
                    {
                        in = mgr.open(filename);
                        String outFileName = String.format("%s/%s", outputDir, filename);
                        out = Files.newOutputStream(Paths.get(outFileName));
                        Log.i(s_tag, String.format("Copying asset %s to %s", filename, outFileName));
                        copyFile(in, out);
                    } catch(IOException e)
                    {
                        Log.e(s_tag, "Failed to copy asset file: " + filename, e);
                    } finally
                    {
                        try
                        {
                            in.close();
                            out.flush();
                            out.close();
                        } catch(IOException e1)
                        {
                            Log.e(s_tag, "Failed to close: " + filename, e1);
                        }
                    }
                }
            }
        } catch (IOException e)
        {
            Log.e(s_tag, String.format("AssetManager cannot list in '%s'", apkAssetsRelativePath), e);
        }
    }

    ////////////////////////////////////////////////////////////////
    public static void SetAssetManager(AssetManager assetManager, Context context, boolean copyPaks)
    {
        s_assetManager = assetManager;

        if (copyPaks)
        {
            copyPakFiles(s_assetManager, "", context);
        }
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
