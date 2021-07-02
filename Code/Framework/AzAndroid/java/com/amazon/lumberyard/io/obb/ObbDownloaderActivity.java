/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
package com.amazon.lumberyard.io.obb;


import android.app.Activity;
import android.app.PendingIntent;
import android.content.Intent;
import android.content.res.Resources;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Messenger;
import android.provider.Settings;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.google.android.vending.expansion.downloader.Constants;
import com.google.android.vending.expansion.downloader.DownloadProgressInfo;
import com.google.android.vending.expansion.downloader.DownloaderClientMarshaller;
import com.google.android.vending.expansion.downloader.DownloaderServiceMarshaller;
import com.google.android.vending.expansion.downloader.Helpers;
import com.google.android.vending.expansion.downloader.IDownloaderClient;
import com.google.android.vending.expansion.downloader.IDownloaderService;
import com.google.android.vending.expansion.downloader.IStub;

import java.lang.Exception;


////////////////////////////////////////////////////////////////
// Activity that handles the download of the APK expansion package (Obb)
public class ObbDownloaderActivity extends Activity implements IDownloaderClient
{
    ////////////////////////////////////////////////////////////////
    @Override
    public void onServiceConnected(Messenger messenger)
    {
        m_remoteService = DownloaderServiceMarshaller.CreateProxy(messenger);
        m_remoteService.onClientUpdated(m_downloaderClientStub.getMessenger());
    }

    ////////////////////////////////////////////////////////////////
    @Override
    public void onDownloadStateChanged(int newState)
    {
        setState(newState);
        boolean showDashboard = true;
        boolean showCellMessage = false;
        boolean paused;
        boolean indeterminate;
        switch (newState)
        {
            case IDownloaderClient.STATE_IDLE:
                // STATE_IDLE means the service is listening, so it's
                // safe to start making calls via m_remoteService.
                paused = false;
                indeterminate = true;
                break;
            case IDownloaderClient.STATE_CONNECTING:
            case IDownloaderClient.STATE_FETCHING_URL:
                showDashboard = true;
                paused = false;
                indeterminate = true;
                break;
            case IDownloaderClient.STATE_DOWNLOADING:
                paused = false;
                showDashboard = true;
                indeterminate = false;
                break;

            case IDownloaderClient.STATE_FAILED_CANCELED:
            case IDownloaderClient.STATE_FAILED:
            case IDownloaderClient.STATE_FAILED_FETCHING_URL:
            case IDownloaderClient.STATE_FAILED_UNLICENSED:
                paused = true;
                showDashboard = false;
                indeterminate = false;
                break;
            case IDownloaderClient.STATE_PAUSED_NEED_CELLULAR_PERMISSION:
            case IDownloaderClient.STATE_PAUSED_WIFI_DISABLED_NEED_CELLULAR_PERMISSION:
                showDashboard = false;
                paused = true;
                indeterminate = false;
                showCellMessage = true;
                break;

            case IDownloaderClient.STATE_PAUSED_BY_REQUEST:
                paused = true;
                indeterminate = false;
                break;
            case IDownloaderClient.STATE_PAUSED_ROAMING:
            case IDownloaderClient.STATE_PAUSED_SDCARD_UNAVAILABLE:
                paused = true;
                indeterminate = false;
                break;
            case IDownloaderClient.STATE_COMPLETED:
                showDashboard = false;
                paused = false;
                indeterminate = false;
                endActivity(Activity.RESULT_OK);
                return;
            default:
                paused = true;
                indeterminate = true;
                showDashboard = true;
        }

        int newDashboardVisibility = showDashboard ? View.VISIBLE : View.GONE;
        if (m_dashboard.getVisibility() != newDashboardVisibility)
        {
            m_dashboard.setVisibility(newDashboardVisibility);
        }
        int cellMessageVisibility = showCellMessage ? View.VISIBLE : View.GONE;
        if (m_cellMessage.getVisibility() != cellMessageVisibility)
        {
            m_cellMessage.setVisibility(cellMessageVisibility);
        }

        m_progressBar.setIndeterminate(indeterminate);
        setButtonPausedState(paused);
    }

    ////////////////////////////////////////////////////////////////
    @Override
    public void onDownloadProgress(DownloadProgressInfo progress)
    {
        m_averageSpeed.setText(getString(m_kbPerSecondTextId, Helpers.getSpeedString(progress.mCurrentSpeed)));
        m_timeRemaining.setText(getString(m_timeRemainingTextId, Helpers.getTimeRemaining(progress.mTimeRemaining)));

        progress.mOverallTotal = progress.mOverallTotal;
        m_progressBar.setMax((int) (progress.mOverallTotal >> 8));
        m_progressBar.setProgress((int) (progress.mOverallProgress >> 8));
        m_progressPercent.setText(Long.toString(progress.mOverallProgress * 100 / progress.mOverallTotal) + "%");
        m_progressFraction.setText(Helpers.getDownloadProgressString(progress.mOverallProgress, progress.mOverallTotal));
    }

    ////////////////////////////////////////////////////////////////
    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        // Build an Intent to start this activity from the Notification
        Intent notifierIntent = new Intent(ObbDownloaderActivity.this, ObbDownloaderActivity.this.getClass());
        notifierIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, notifierIntent, PendingIntent.FLAG_UPDATE_CURRENT);

        try
        {
            // Start the download service (if required)
            int startResult = DownloaderClientMarshaller.startDownloadServiceIfRequired(this, pendingIntent, ObbDownloaderService.class);
            // If download has started, initialize this activity to show download progress
            if (startResult != DownloaderClientMarshaller.NO_DOWNLOAD_REQUIRED)
            {
                initializeUI();
                return;
            }
        }
        catch (Exception e)
        {
            endActivity(Activity.RESULT_CANCELED);
            return;
        }

        endActivity(Activity.RESULT_OK);
    }

    ////////////////////////////////////////////////////////////////
    @Override
    protected void onResume()
    {
        if (m_downloaderClientStub != null)
        {
            m_downloaderClientStub.connect(this);
        }
        super.onResume();
    }

    ////////////////////////////////////////////////////////////////
    @Override
    protected void onStop()
    {
        if (m_downloaderClientStub != null)
        {
            m_downloaderClientStub.disconnect(this);
        }
        super.onStop();
    }

    ////////////////////////////////////////////////////////////////
    protected void endActivity(int result)
    {
        if (isFinishing())
        {
            return;
        }

        Intent returnIntent = new Intent();
        setResult(result, returnIntent);
        finish();
    }

    ////////////////////////////////////////////////////////////////
    private void setState(int newState)
    {
        if (m_state != newState)
        {
            m_state = newState;
            m_statusText.setText(Helpers.getDownloaderStringResourceIDFromState(newState));
        }
    }

    ////////////////////////////////////////////////////////////////
    private void setButtonPausedState(boolean paused)
    {
        m_statePaused = paused;
        int stringResourceID = paused ? m_buttonResumeTextId : m_buttonPauseTextId;
        m_pauseButton.setText(stringResourceID);
    }

    ////////////////////////////////////////////////////////////////
    private void initializeUI()
    {
        Resources resources = this.getResources();
        String packageName = getApplicationContext().getPackageName();

        m_downloaderClientStub = DownloaderClientMarshaller.CreateStub(this, ObbDownloaderService.class);
        setContentView(resources.getIdentifier("obb_downloader", "layout", packageName));

        m_progressBar = (ProgressBar) findViewById(resources.getIdentifier("progressBar", "id", packageName));
        m_statusText = (TextView) findViewById(resources.getIdentifier("statusText", "id", packageName));
        m_progressFraction = (TextView) findViewById(resources.getIdentifier("progressAsFraction", "id", packageName));
        m_progressPercent = (TextView) findViewById(resources.getIdentifier("progressAsPercentage", "id", packageName));
        m_averageSpeed = (TextView) findViewById(resources.getIdentifier("progressAverageSpeed", "id", packageName));
        m_timeRemaining = (TextView) findViewById(resources.getIdentifier("progressTimeRemaining", "id", packageName));
        m_dashboard = findViewById(resources.getIdentifier("downloaderDashboard", "id", packageName));
        m_cellMessage = findViewById(resources.getIdentifier("approveCellular", "id", packageName));
        m_pauseButton = (Button) findViewById(resources.getIdentifier("pauseButton", "id", packageName));
        m_wiFiSettingsButton = (Button) findViewById(resources.getIdentifier("wifiSettingsButton", "id", packageName));

        m_buttonResumeTextId = resources.getIdentifier("text_button_resume", "string", packageName);
        m_buttonPauseTextId = resources.getIdentifier("text_button_pause", "string", packageName);
        m_timeRemainingTextId = resources.getIdentifier("time_remaining", "string", packageName);
        m_kbPerSecondTextId = resources.getIdentifier("kilobytes_per_second", "string", packageName);

        m_pauseButton.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View view)
            {
                if (m_statePaused)
                {
                    m_remoteService.requestContinueDownload();
                }
                else
                {
                    m_remoteService.requestPauseDownload();
                }
                setButtonPausedState(!m_statePaused);
            }
        });

        m_wiFiSettingsButton.setOnClickListener(new View.OnClickListener()
        {

            @Override
            public void onClick(View v)
            {
                startActivity(new Intent(Settings.ACTION_WIFI_SETTINGS));
            }
        });

        Button resumeOnCell = (Button) findViewById(resources.getIdentifier("resumeOverCellular", "id", packageName));
        resumeOnCell.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View view)
            {
                m_remoteService.setDownloadFlags(IDownloaderService.FLAGS_DOWNLOAD_OVER_CELLULAR);
                m_remoteService.requestContinueDownload();
                m_cellMessage.setVisibility(View.GONE);
            }
        });
    }

    private static final String TAG = "ObbDownloaderActivity";

    private ProgressBar m_progressBar;

    private TextView m_statusText;
    private TextView m_progressFraction;
    private TextView m_progressPercent;
    private TextView m_averageSpeed;
    private TextView m_timeRemaining;

    private View m_dashboard;
    private View m_cellMessage;

    private Button m_pauseButton;
    private Button m_wiFiSettingsButton;

    private boolean m_statePaused;
    private int m_state;

    private IDownloaderService m_remoteService;

    private IStub m_downloaderClientStub;

    private int m_buttonResumeTextId;
    private int m_buttonPauseTextId;
    private int m_kbPerSecondTextId;
    private int m_timeRemainingTextId;
}
