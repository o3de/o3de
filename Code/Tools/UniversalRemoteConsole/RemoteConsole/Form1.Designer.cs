﻿/*
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

namespace RemoteConsole
{
    partial class MainForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainForm));
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea1 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.Windows.Forms.DataVisualization.Charting.Legend legend1 = new System.Windows.Forms.DataVisualization.Charting.Legend();
            this.topToolStrip = new System.Windows.Forms.ToolStrip();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.cmsGraph = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.toolStripMenuItem3 = new System.Windows.Forms.ToolStripMenuItem();
            this.logPanel = new System.Windows.Forms.Panel();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.tabControlTop = new System.Windows.Forms.TabControl();
            this.tabPage2 = new System.Windows.Forms.TabPage();
            this.logChart = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this.contextMenuHistoryChart = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.historyChartViewMenu = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
            this.historyChartClear = new System.Windows.Forms.ToolStripMenuItem();
            this.tabFullLog = new System.Windows.Forms.TabPage();
            this.fullLogConsole = new System.Windows.Forms.RichTextBox();
            this.contextMenuTabs = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.actionSelectAll = new System.Windows.Forms.ToolStripMenuItem();
            this.actionCopy = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.actionClear = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator4 = new System.Windows.Forms.ToolStripSeparator();
            this.actionClearAll = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator5 = new System.Windows.Forms.ToolStripSeparator();
            this.actionEditFiltersFile = new System.Windows.Forms.ToolStripMenuItem();
            this.lalala = new System.Windows.Forms.ToolStripTextBox();
            this.lalala2 = new System.Windows.Forms.ToolStripTextBox();
            this.toolStripSeparator6 = new System.Windows.Forms.ToolStripSeparator();
            this.tabPage1 = new System.Windows.Forms.TabPage();
            this.panelHelp = new System.Windows.Forms.Panel();
            this.richTextBox1 = new System.Windows.Forms.RichTextBox();
            this.tabPageMidi = new System.Windows.Forms.TabPage();
            this.lvMidiLog = new System.Windows.Forms.ListView();
            this.cHNum = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.cHLog = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.cHChannel = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.chMode = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.chCode = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.chVelocity = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.tabControlFilters = new System.Windows.Forms.TabControl();
            this.commandPanel = new System.Windows.Forms.Panel();
            this.label1 = new System.Windows.Forms.Label();
            this.editInput = new System.Windows.Forms.TextBox();
            this.imgList = new System.Windows.Forms.ImageList(this.components);
            this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
            this.statusStrip1 = new System.Windows.Forms.StatusStrip();
            this.toolStripStatusLabel2 = new System.Windows.Forms.ToolStripStatusLabel();
            this.toolStripStatusLabel1 = new System.Windows.Forms.ToolStripStatusLabel();
            this.statusLabelTarget = new System.Windows.Forms.ToolStripStatusLabel();
            this.statusLabelIp = new System.Windows.Forms.ToolStripStatusLabel();
            this.labelSeparator = new System.Windows.Forms.ToolStripStatusLabel();
            this.toolStripStatusLabel4 = new System.Windows.Forms.ToolStripStatusLabel();
            this.statusLabelMode = new System.Windows.Forms.ToolStripStatusLabel();
            this.toolStripStatusLabel5 = new System.Windows.Forms.ToolStripStatusLabel();
            this.statusLabelMidi = new System.Windows.Forms.ToolStripStatusLabel();
            this.forceRightAlignment = new System.Windows.Forms.ToolStripStatusLabel();
            this.statusMenuLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this.menuStatusText = new System.Windows.Forms.ToolStripStatusLabel();
            this.statusLabelFilters = new System.Windows.Forms.ToolStripStatusLabel();
            this.filtersTextStatus = new System.Windows.Forms.ToolStripStatusLabel();
            this.toolStripStatusLabel3 = new System.Windows.Forms.ToolStripStatusLabel();
            this.statusConnectedImg = new System.Windows.Forms.ToolStripStatusLabel();
            this.statusLabelConnected = new System.Windows.Forms.ToolStripStatusLabel();
            this.bottomPanel = new System.Windows.Forms.Panel();
            this.bodyPanel = new System.Windows.Forms.Panel();
            this.imgList32 = new System.Windows.Forms.ImageList(this.components);
            this.topToolStrip.SuspendLayout();
            this.cmsGraph.SuspendLayout();
            this.logPanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).BeginInit();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.tabControlTop.SuspendLayout();
            this.tabPage2.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.logChart)).BeginInit();
            this.contextMenuHistoryChart.SuspendLayout();
            this.tabFullLog.SuspendLayout();
            this.contextMenuTabs.SuspendLayout();
            this.tabPage1.SuspendLayout();
            this.panelHelp.SuspendLayout();
            this.tabPageMidi.SuspendLayout();
            this.commandPanel.SuspendLayout();
            this.statusStrip1.SuspendLayout();
            this.bottomPanel.SuspendLayout();
            this.bodyPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // topToolStrip
            // 
            this.topToolStrip.Dock = System.Windows.Forms.DockStyle.None;
            this.topToolStrip.ImageScalingSize = new System.Drawing.Size(32, 32);
            this.topToolStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripSeparator1});
            this.topToolStrip.Location = new System.Drawing.Point(3, 9);
            this.topToolStrip.Name = "topToolStrip";
            this.topToolStrip.Size = new System.Drawing.Size(18, 25);
            this.topToolStrip.TabIndex = 0;
            this.topToolStrip.ItemClicked += new System.Windows.Forms.ToolStripItemClickedEventHandler(this.topToolStrip_ItemClicked);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(6, 25);
            // 
            // cmsGraph
            // 
            this.cmsGraph.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripMenuItem3});
            this.cmsGraph.Name = "contextMenuStrip1";
            this.cmsGraph.Size = new System.Drawing.Size(134, 26);
            // 
            // toolStripMenuItem3
            // 
            this.toolStripMenuItem3.Image = ((System.Drawing.Image)(resources.GetObject("toolStripMenuItem3.Image")));
            this.toolStripMenuItem3.Name = "toolStripMenuItem3";
            this.toolStripMenuItem3.Size = new System.Drawing.Size(133, 22);
            this.toolStripMenuItem3.Text = "Next Graph";
            // 
            // logPanel
            // 
            this.logPanel.AutoSize = true;
            this.logPanel.Controls.Add(this.splitContainer1);
            this.logPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.logPanel.Location = new System.Drawing.Point(0, 0);
            this.logPanel.Name = "logPanel";
            this.logPanel.Padding = new System.Windows.Forms.Padding(3);
            this.logPanel.Size = new System.Drawing.Size(884, 503);
            this.logPanel.TabIndex = 1;
            // 
            // splitContainer1
            // 
            this.splitContainer1.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.Location = new System.Drawing.Point(3, 3);
            this.splitContainer1.Name = "splitContainer1";
            this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.tabControlTop);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.tabControlFilters);
            this.splitContainer1.Size = new System.Drawing.Size(878, 497);
            this.splitContainer1.SplitterDistance = 204;
            this.splitContainer1.TabIndex = 2;
            // 
            // tabControlTop
            // 
            this.tabControlTop.Controls.Add(this.tabPage2);
            this.tabControlTop.Controls.Add(this.tabFullLog);
            this.tabControlTop.Controls.Add(this.tabPage1);
            this.tabControlTop.Controls.Add(this.tabPageMidi);
            this.tabControlTop.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tabControlTop.Location = new System.Drawing.Point(0, 0);
            this.tabControlTop.Name = "tabControlTop";
            this.tabControlTop.SelectedIndex = 0;
            this.tabControlTop.Size = new System.Drawing.Size(874, 200);
            this.tabControlTop.TabIndex = 2;
            // 
            // tabPage2
            // 
            this.tabPage2.Controls.Add(this.logChart);
            this.tabPage2.Location = new System.Drawing.Point(4, 22);
            this.tabPage2.Name = "tabPage2";
            this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage2.Size = new System.Drawing.Size(866, 174);
            this.tabPage2.TabIndex = 1;
            this.tabPage2.Text = "Chart";
            this.tabPage2.UseVisualStyleBackColor = true;
            // 
            // logChart
            // 
            this.logChart.BorderlineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            chartArea1.Name = "ChartArea1";
            this.logChart.ChartAreas.Add(chartArea1);
            this.logChart.ContextMenuStrip = this.contextMenuHistoryChart;
            this.logChart.Dock = System.Windows.Forms.DockStyle.Fill;
            legend1.Name = "Legend1";
            this.logChart.Legends.Add(legend1);
            this.logChart.Location = new System.Drawing.Point(3, 3);
            this.logChart.Name = "logChart";
            this.logChart.Size = new System.Drawing.Size(860, 168);
            this.logChart.TabIndex = 16;
            // 
            // contextMenuHistoryChart
            // 
            this.contextMenuHistoryChart.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.historyChartViewMenu,
            this.toolStripSeparator3,
            this.historyChartClear});
            this.contextMenuHistoryChart.Name = "contextMenuStrip1";
            this.contextMenuHistoryChart.Size = new System.Drawing.Size(102, 54);
            this.contextMenuHistoryChart.Opening += new System.ComponentModel.CancelEventHandler(this.contextMenuHistoryChart_Opening);
            // 
            // historyChartViewMenu
            // 
            this.historyChartViewMenu.Image = ((System.Drawing.Image)(resources.GetObject("historyChartViewMenu.Image")));
            this.historyChartViewMenu.Name = "historyChartViewMenu";
            this.historyChartViewMenu.Size = new System.Drawing.Size(101, 22);
            this.historyChartViewMenu.Text = "View";
            // 
            // toolStripSeparator3
            // 
            this.toolStripSeparator3.Name = "toolStripSeparator3";
            this.toolStripSeparator3.Size = new System.Drawing.Size(98, 6);
            // 
            // historyChartClear
            // 
            this.historyChartClear.Image = ((System.Drawing.Image)(resources.GetObject("historyChartClear.Image")));
            this.historyChartClear.Name = "historyChartClear";
            this.historyChartClear.Size = new System.Drawing.Size(101, 22);
            this.historyChartClear.Text = "Clear";
            this.historyChartClear.Click += new System.EventHandler(this.historyChartClear_Click);
            // 
            // tabFullLog
            // 
            this.tabFullLog.Controls.Add(this.fullLogConsole);
            this.tabFullLog.Location = new System.Drawing.Point(4, 22);
            this.tabFullLog.Name = "tabFullLog";
            this.tabFullLog.Padding = new System.Windows.Forms.Padding(3);
            this.tabFullLog.Size = new System.Drawing.Size(866, 174);
            this.tabFullLog.TabIndex = 0;
            this.tabFullLog.Text = "Full Log";
            this.tabFullLog.UseVisualStyleBackColor = true;
            // 
            // fullLogConsole
            // 
            this.fullLogConsole.BackColor = System.Drawing.SystemColors.WindowText;
            this.fullLogConsole.ContextMenuStrip = this.contextMenuTabs;
            this.fullLogConsole.Dock = System.Windows.Forms.DockStyle.Fill;
            this.fullLogConsole.ForeColor = System.Drawing.Color.Silver;
            this.fullLogConsole.Location = new System.Drawing.Point(3, 3);
            this.fullLogConsole.Name = "fullLogConsole";
            this.fullLogConsole.ReadOnly = true;
            this.fullLogConsole.Size = new System.Drawing.Size(860, 168);
            this.fullLogConsole.TabIndex = 0;
            this.fullLogConsole.Text = "";
            // 
            // contextMenuTabs
            // 
            this.contextMenuTabs.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.actionSelectAll,
            this.actionCopy,
            this.toolStripSeparator2,
            this.actionClear,
            this.toolStripSeparator4,
            this.actionClearAll,
            this.toolStripSeparator5,
            this.actionEditFiltersFile,
            this.lalala,
            this.lalala2,
            this.toolStripSeparator6});
            this.contextMenuTabs.Name = "contextMenuStrip1";
            this.contextMenuTabs.Size = new System.Drawing.Size(176, 188);
            // 
            // actionSelectAll
            // 
            this.actionSelectAll.Name = "actionSelectAll";
            this.actionSelectAll.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.A)));
            this.actionSelectAll.Size = new System.Drawing.Size(175, 22);
            this.actionSelectAll.Text = "Select All";
            this.actionSelectAll.Click += new System.EventHandler(this.actionSelectAll_Click);
            // 
            // actionCopy
            // 
            this.actionCopy.Image = ((System.Drawing.Image)(resources.GetObject("actionCopy.Image")));
            this.actionCopy.Name = "actionCopy";
            this.actionCopy.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.C)));
            this.actionCopy.Size = new System.Drawing.Size(175, 22);
            this.actionCopy.Text = "Copy";
            this.actionCopy.Click += new System.EventHandler(this.actionCopy_Click);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(172, 6);
            // 
            // actionClear
            // 
            this.actionClear.Image = ((System.Drawing.Image)(resources.GetObject("actionClear.Image")));
            this.actionClear.Name = "actionClear";
            this.actionClear.ShortcutKeys = System.Windows.Forms.Keys.Delete;
            this.actionClear.Size = new System.Drawing.Size(175, 22);
            this.actionClear.Text = "Clear";
            this.actionClear.Click += new System.EventHandler(this.actionClear_Click);
            // 
            // toolStripSeparator4
            // 
            this.toolStripSeparator4.Name = "toolStripSeparator4";
            this.toolStripSeparator4.Size = new System.Drawing.Size(172, 6);
            // 
            // actionClearAll
            // 
            this.actionClearAll.Image = ((System.Drawing.Image)(resources.GetObject("actionClearAll.Image")));
            this.actionClearAll.Name = "actionClearAll";
            this.actionClearAll.Size = new System.Drawing.Size(175, 22);
            this.actionClearAll.Text = "Clear ALL Filters";
            this.actionClearAll.Click += new System.EventHandler(this.actionClearAll_Click);
            // 
            // toolStripSeparator5
            // 
            this.toolStripSeparator5.Name = "toolStripSeparator5";
            this.toolStripSeparator5.Size = new System.Drawing.Size(172, 6);
            // 
            // actionEditFiltersFile
            // 
            this.actionEditFiltersFile.Image = ((System.Drawing.Image)(resources.GetObject("actionEditFiltersFile.Image")));
            this.actionEditFiltersFile.Name = "actionEditFiltersFile";
            this.actionEditFiltersFile.Size = new System.Drawing.Size(175, 22);
            this.actionEditFiltersFile.Text = "-- Edit Filters File --";
            this.actionEditFiltersFile.Click += new System.EventHandler(this.actionEditFiltersFile_Click);
            // 
            // lalala
            // 
            this.lalala.Enabled = false;
            this.lalala.Name = "lalala";
            this.lalala.Size = new System.Drawing.Size(100, 23);
            this.lalala.Text = "PORT";
            // 
            // lalala2
            // 
            this.lalala2.Name = "lalala2";
            this.lalala2.Size = new System.Drawing.Size(100, 23);
            // 
            // toolStripSeparator6
            // 
            this.toolStripSeparator6.Name = "toolStripSeparator6";
            this.toolStripSeparator6.Size = new System.Drawing.Size(172, 6);
            // 
            // tabPage1
            // 
            this.tabPage1.Controls.Add(this.panelHelp);
            this.tabPage1.Location = new System.Drawing.Point(4, 22);
            this.tabPage1.Name = "tabPage1";
            this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage1.Size = new System.Drawing.Size(866, 174);
            this.tabPage1.TabIndex = 2;
            this.tabPage1.Text = "About/Help";
            this.tabPage1.UseVisualStyleBackColor = true;
            // 
            // panelHelp
            // 
            this.panelHelp.Controls.Add(this.richTextBox1);
            this.panelHelp.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panelHelp.Location = new System.Drawing.Point(3, 3);
            this.panelHelp.Name = "panelHelp";
            this.panelHelp.Size = new System.Drawing.Size(860, 168);
            this.panelHelp.TabIndex = 0;
            // 
            // richTextBox1
            // 
            this.richTextBox1.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.richTextBox1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.richTextBox1.Location = new System.Drawing.Point(0, 0);
            this.richTextBox1.Name = "richTextBox1";
            this.richTextBox1.ReadOnly = true;
            this.richTextBox1.Size = new System.Drawing.Size(860, 168);
            this.richTextBox1.TabIndex = 0;
            this.richTextBox1.Text = resources.GetString("richTextBox1.Text");
            // 
            // tabPageMidi
            // 
            this.tabPageMidi.Controls.Add(this.lvMidiLog);
            this.tabPageMidi.Location = new System.Drawing.Point(4, 22);
            this.tabPageMidi.Name = "tabPageMidi";
            this.tabPageMidi.Padding = new System.Windows.Forms.Padding(3);
            this.tabPageMidi.Size = new System.Drawing.Size(866, 174);
            this.tabPageMidi.TabIndex = 3;
            this.tabPageMidi.Text = "Midi Debug";
            this.tabPageMidi.UseVisualStyleBackColor = true;
            // 
            // lvMidiLog
            // 
            this.lvMidiLog.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.cHNum,
            this.cHLog,
            this.cHChannel,
            this.chMode,
            this.chCode,
            this.chVelocity});
            this.lvMidiLog.Dock = System.Windows.Forms.DockStyle.Fill;
            this.lvMidiLog.Location = new System.Drawing.Point(3, 3);
            this.lvMidiLog.Name = "lvMidiLog";
            this.lvMidiLog.Size = new System.Drawing.Size(860, 168);
            this.lvMidiLog.TabIndex = 0;
            this.lvMidiLog.UseCompatibleStateImageBehavior = false;
            this.lvMidiLog.View = System.Windows.Forms.View.Details;
            // 
            // cHNum
            // 
            this.cHNum.Text = "#";
            // 
            // cHLog
            // 
            this.cHLog.Text = "Log";
            this.cHLog.Width = 278;
            // 
            // cHChannel
            // 
            this.cHChannel.Text = "Channel";
            // 
            // chMode
            // 
            this.chMode.Text = "Mode";
            // 
            // chCode
            // 
            this.chCode.Text = "Code";
            // 
            // chVelocity
            // 
            this.chVelocity.Text = "Vel";
            // 
            // tabControlFilters
            // 
            this.tabControlFilters.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tabControlFilters.Location = new System.Drawing.Point(0, 0);
            this.tabControlFilters.Name = "tabControlFilters";
            this.tabControlFilters.SelectedIndex = 0;
            this.tabControlFilters.Size = new System.Drawing.Size(874, 285);
            this.tabControlFilters.TabIndex = 1;
            // 
            // commandPanel
            // 
            this.commandPanel.AutoSize = true;
            this.commandPanel.Controls.Add(this.label1);
            this.commandPanel.Controls.Add(this.editInput);
            this.commandPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this.commandPanel.Location = new System.Drawing.Point(0, 0);
            this.commandPanel.MaximumSize = new System.Drawing.Size(0, 26);
            this.commandPanel.MinimumSize = new System.Drawing.Size(0, 26);
            this.commandPanel.Name = "commandPanel";
            this.commandPanel.Padding = new System.Windows.Forms.Padding(3);
            this.commandPanel.Size = new System.Drawing.Size(884, 26);
            this.commandPanel.TabIndex = 2;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(5, 7);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(92, 13);
            this.label1.TabIndex = 1;
            this.label1.Text = "Type a command:";
            // 
            // editInput
            // 
            this.editInput.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.editInput.AutoCompleteMode = System.Windows.Forms.AutoCompleteMode.SuggestAppend;
            this.editInput.AutoCompleteSource = System.Windows.Forms.AutoCompleteSource.CustomSource;
            this.editInput.BackColor = System.Drawing.Color.Black;
            this.editInput.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.editInput.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(224)))), ((int)(((byte)(224)))), ((int)(((byte)(224)))));
            this.editInput.Location = new System.Drawing.Point(103, 0);
            this.editInput.Name = "editInput";
            this.editInput.Size = new System.Drawing.Size(6768, 23);
            this.editInput.TabIndex = 0;
            this.editInput.KeyUp += new System.Windows.Forms.KeyEventHandler(this.editInput_KeyUp);
            // 
            // imgList
            // 
            this.imgList.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imgList.ImageStream")));
            this.imgList.TransparentColor = System.Drawing.Color.Transparent;
            this.imgList.Images.SetKeyName(0, "red.png");
            this.imgList.Images.SetKeyName(1, "green.png");
            this.imgList.Images.SetKeyName(2, "clear.png");
            this.imgList.Images.SetKeyName(3, "copy.png");
            this.imgList.Images.SetKeyName(4, "pc");
            this.imgList.Images.SetKeyName(6, "UnknownTarget");
            this.imgList.Images.SetKeyName(7, "videoClipRecord");
            this.imgList.Images.SetKeyName(8, "screenShoot");
            this.imgList.Images.SetKeyName(9, "PcMouseImg");
            this.imgList.Images.SetKeyName(10, "EditIpImg");
            this.imgList.Images.SetKeyName(11, "IpImg");
            this.imgList.Images.SetKeyName(12, "ModeFullImg");
            this.imgList.Images.SetKeyName(13, "ModeCmdImg");
            this.imgList.Images.SetKeyName(14, "ModeFileImg");
            this.imgList.Images.SetKeyName(15, "PortImg");
            this.imgList.Images.SetKeyName(16, "WinLogoImg16");
            this.imgList.Images.SetKeyName(17, "provo");
            // 
            // statusStrip1
            // 
            this.statusStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripStatusLabel2,
            this.toolStripStatusLabel1,
            this.statusLabelTarget,
            this.statusLabelIp,
            this.labelSeparator,
            this.toolStripStatusLabel4,
            this.statusLabelMode,
            this.toolStripStatusLabel5,
            this.statusLabelMidi,
            this.forceRightAlignment,
            this.statusMenuLabel,
            this.menuStatusText,
            this.statusLabelFilters,
            this.filtersTextStatus,
            this.toolStripStatusLabel3,
            this.statusConnectedImg,
            this.statusLabelConnected});
            this.statusStrip1.Location = new System.Drawing.Point(0, 26);
            this.statusStrip1.Name = "statusStrip1";
            this.statusStrip1.Size = new System.Drawing.Size(884, 24);
            this.statusStrip1.TabIndex = 3;
            this.statusStrip1.Text = "statusStrip1";
            // 
            // toolStripStatusLabel2
            // 
            this.toolStripStatusLabel2.Name = "toolStripStatusLabel2";
            this.toolStripStatusLabel2.Size = new System.Drawing.Size(0, 19);
            // 
            // toolStripStatusLabel1
            // 
            this.toolStripStatusLabel1.Name = "toolStripStatusLabel1";
            this.toolStripStatusLabel1.Size = new System.Drawing.Size(43, 19);
            this.toolStripStatusLabel1.Text = "Target:";
            // 
            // statusLabelTarget
            // 
            this.statusLabelTarget.Name = "statusLabelTarget";
            this.statusLabelTarget.Size = new System.Drawing.Size(54, 19);
            this.statusLabelTarget.Text = "[Not Set]";
            // 
            // statusLabelIp
            // 
            this.statusLabelIp.Name = "statusLabelIp";
            this.statusLabelIp.Size = new System.Drawing.Size(25, 19);
            this.statusLabelIp.Text = "[IP]";
            // 
            // labelSeparator
            // 
            this.labelSeparator.BorderSides = ((System.Windows.Forms.ToolStripStatusLabelBorderSides)((System.Windows.Forms.ToolStripStatusLabelBorderSides.Left | System.Windows.Forms.ToolStripStatusLabelBorderSides.Right)));
            this.labelSeparator.BorderStyle = System.Windows.Forms.Border3DStyle.Etched;
            this.labelSeparator.Name = "labelSeparator";
            this.labelSeparator.Size = new System.Drawing.Size(4, 19);
            // 
            // toolStripStatusLabel4
            // 
            this.toolStripStatusLabel4.Name = "toolStripStatusLabel4";
            this.toolStripStatusLabel4.Size = new System.Drawing.Size(41, 19);
            this.toolStripStatusLabel4.Text = "Mode:";
            // 
            // statusLabelMode
            // 
            this.statusLabelMode.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.statusLabelMode.Name = "statusLabelMode";
            this.statusLabelMode.Size = new System.Drawing.Size(38, 19);
            this.statusLabelMode.Text = "Mode";
            // 
            // toolStripStatusLabel5
            // 
            this.toolStripStatusLabel5.BorderSides = System.Windows.Forms.ToolStripStatusLabelBorderSides.Left;
            this.toolStripStatusLabel5.BorderStyle = System.Windows.Forms.Border3DStyle.Etched;
            this.toolStripStatusLabel5.Name = "toolStripStatusLabel5";
            this.toolStripStatusLabel5.Size = new System.Drawing.Size(38, 19);
            this.toolStripStatusLabel5.Text = "Midi:";
            // 
            // statusLabelMidi
            // 
            this.statusLabelMidi.Name = "statusLabelMidi";
            this.statusLabelMidi.Size = new System.Drawing.Size(118, 19);
            this.statusLabelMidi.Text = "toolStripStatusLabel6";
            // 
            // forceRightAlignment
            // 
            this.forceRightAlignment.Name = "forceRightAlignment";
            this.forceRightAlignment.Size = new System.Drawing.Size(177, 19);
            this.forceRightAlignment.Spring = true;
            // 
            // statusMenuLabel
            // 
            this.statusMenuLabel.BorderSides = System.Windows.Forms.ToolStripStatusLabelBorderSides.Left;
            this.statusMenuLabel.BorderStyle = System.Windows.Forms.Border3DStyle.Etched;
            this.statusMenuLabel.Name = "statusMenuLabel";
            this.statusMenuLabel.Size = new System.Drawing.Size(45, 19);
            this.statusMenuLabel.Text = "Menu:";
            // 
            // menuStatusText
            // 
            this.menuStatusText.Name = "menuStatusText";
            this.menuStatusText.Size = new System.Drawing.Size(48, 19);
            this.menuStatusText.Text = "Missing";
            // 
            // statusLabelFilters
            // 
            this.statusLabelFilters.BorderSides = System.Windows.Forms.ToolStripStatusLabelBorderSides.Left;
            this.statusLabelFilters.BorderStyle = System.Windows.Forms.Border3DStyle.Etched;
            this.statusLabelFilters.Name = "statusLabelFilters";
            this.statusLabelFilters.Size = new System.Drawing.Size(45, 19);
            this.statusLabelFilters.Text = "Filters:";
            // 
            // filtersTextStatus
            // 
            this.filtersTextStatus.Name = "filtersTextStatus";
            this.filtersTextStatus.Size = new System.Drawing.Size(48, 19);
            this.filtersTextStatus.Text = "Missing";
            // 
            // toolStripStatusLabel3
            // 
            this.toolStripStatusLabel3.BorderSides = System.Windows.Forms.ToolStripStatusLabelBorderSides.Left;
            this.toolStripStatusLabel3.BorderStyle = System.Windows.Forms.Border3DStyle.Etched;
            this.toolStripStatusLabel3.Name = "toolStripStatusLabel3";
            this.toolStripStatusLabel3.Size = new System.Drawing.Size(46, 19);
            this.toolStripStatusLabel3.Text = "Status:";
            // 
            // statusConnectedImg
            // 
            this.statusConnectedImg.Image = ((System.Drawing.Image)(resources.GetObject("statusConnectedImg.Image")));
            this.statusConnectedImg.Name = "statusConnectedImg";
            this.statusConnectedImg.Size = new System.Drawing.Size(16, 19);
            // 
            // statusLabelConnected
            // 
            this.statusLabelConnected.BorderSides = System.Windows.Forms.ToolStripStatusLabelBorderSides.Right;
            this.statusLabelConnected.BorderStyle = System.Windows.Forms.Border3DStyle.Etched;
            this.statusLabelConnected.Name = "statusLabelConnected";
            this.statusLabelConnected.Size = new System.Drawing.Size(83, 19);
            this.statusLabelConnected.Text = "Disconnected";
            // 
            // bottomPanel
            // 
            this.bottomPanel.Controls.Add(this.commandPanel);
            this.bottomPanel.Controls.Add(this.statusStrip1);
            this.bottomPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.bottomPanel.Location = new System.Drawing.Point(0, 503);
            this.bottomPanel.Name = "bottomPanel";
            this.bottomPanel.Size = new System.Drawing.Size(884, 50);
            this.bottomPanel.TabIndex = 4;
            // 
            // bodyPanel
            // 
            this.bodyPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.bodyPanel.Controls.Add(this.logPanel);
            this.bodyPanel.Controls.Add(this.bottomPanel);
            this.bodyPanel.Location = new System.Drawing.Point(0, 53);
            this.bodyPanel.Name = "bodyPanel";
            this.bodyPanel.Size = new System.Drawing.Size(884, 553);
            this.bodyPanel.TabIndex = 1;
            // 
            // imgList32
            // 
            this.imgList32.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imgList32.ImageStream")));
            this.imgList32.TransparentColor = System.Drawing.Color.Transparent;
            this.imgList32.Images.SetKeyName(0, "Spanner");
            this.imgList32.Images.SetKeyName(1, "TargetImg1");
            this.imgList32.Images.SetKeyName(2, "TargetImg3");
            this.imgList32.Images.SetKeyName(3, "GamePlayImg2");
            this.imgList32.Images.SetKeyName(4, "ExtraMacrosImg");
            this.imgList32.Images.SetKeyName(5, "ModeFilesImg32");
            this.imgList32.Images.SetKeyName(6, "ListImg");
            this.imgList32.Images.SetKeyName(7, "GamePlayImg3");
            this.imgList32.Images.SetKeyName(8, "TargetImg3");
            this.imgList32.Images.SetKeyName(9, "ModeImg32");
            this.imgList32.Images.SetKeyName(10, "ModeImg32_Cmd");
            this.imgList32.Images.SetKeyName(11, "ModeImg32_File");
            this.imgList32.Images.SetKeyName(12, "AnalogImg32");
            this.imgList32.Images.SetKeyName(13, "GamePlayImg");
            this.imgList32.Images.SetKeyName(14, "TargetImg2");
            this.imgList32.Images.SetKeyName(15, "RadImg32");
            this.imgList32.Images.SetKeyName(16, "EarthImg32");
            this.imgList32.Images.SetKeyName(17, "SettingsImg32");
            this.imgList32.Images.SetKeyName(18, "SlidersImg32");
            this.imgList32.Images.SetKeyName(19, "ToolsImg32");
            this.imgList32.Images.SetKeyName(20, "ModeCmdImg32");
            this.imgList32.Images.SetKeyName(21, "ModeFullImg32");
            this.imgList32.Images.SetKeyName(22, "EmptyImg32");
            this.imgList32.Images.SetKeyName(23, "FileImg32");
            this.imgList32.Images.SetKeyName(24, "FullImg32");
            this.imgList32.Images.SetKeyName(25, "NoImg32");
            this.imgList32.Images.SetKeyName(26, "OkImg32");
            this.imgList32.Images.SetKeyName(27, "BubbleImg32");
            this.imgList32.Images.SetKeyName(28, "UpIcon32");
            // 
            // MainForm
            // 
            this.AllowDrop = true;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(884, 606);
            this.Controls.Add(this.topToolStrip);
            this.Controls.Add(this.bodyPanel);
            this.DoubleBuffered = true;
            this.HelpButton = true;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MinimumSize = new System.Drawing.Size(715, 250);
            this.Name = "MainForm";
            this.Text = "Universal Remote Console";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.DragDrop += new System.Windows.Forms.DragEventHandler(this.MainForm_DragDrop);
            this.DragEnter += new System.Windows.Forms.DragEventHandler(this.MainForm_DragEnter);
            this.topToolStrip.ResumeLayout(false);
            this.topToolStrip.PerformLayout();
            this.cmsGraph.ResumeLayout(false);
            this.logPanel.ResumeLayout(false);
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).EndInit();
            this.splitContainer1.ResumeLayout(false);
            this.tabControlTop.ResumeLayout(false);
            this.tabPage2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.logChart)).EndInit();
            this.contextMenuHistoryChart.ResumeLayout(false);
            this.tabFullLog.ResumeLayout(false);
            this.contextMenuTabs.ResumeLayout(false);
            this.contextMenuTabs.PerformLayout();
            this.tabPage1.ResumeLayout(false);
            this.panelHelp.ResumeLayout(false);
            this.tabPageMidi.ResumeLayout(false);
            this.commandPanel.ResumeLayout(false);
            this.commandPanel.PerformLayout();
            this.statusStrip1.ResumeLayout(false);
            this.statusStrip1.PerformLayout();
            this.bottomPanel.ResumeLayout(false);
            this.bottomPanel.PerformLayout();
            this.bodyPanel.ResumeLayout(false);
            this.bodyPanel.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

				private System.Windows.Forms.Panel logPanel;
        private System.Windows.Forms.RichTextBox fullLogConsole;
        private System.Windows.Forms.Panel commandPanel;
				private System.Windows.Forms.TextBox editInput;
        private System.Windows.Forms.ImageList imgList;
        private System.Windows.Forms.ContextMenuStrip contextMenuTabs;
        private System.Windows.Forms.ToolStripMenuItem actionSelectAll;
        private System.Windows.Forms.ToolStripMenuItem actionCopy;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
				private System.Windows.Forms.ToolStripMenuItem actionClear;
        private System.Windows.Forms.ToolTip toolTip1;
				private System.Windows.Forms.TabControl tabControlFilters;
				private System.Windows.Forms.SplitContainer splitContainer1;
				private System.Windows.Forms.ContextMenuStrip cmsGraph;
				private System.Windows.Forms.ToolStripMenuItem toolStripMenuItem3;
				private System.Windows.Forms.StatusStrip statusStrip1;
				private System.Windows.Forms.Panel bottomPanel;
				private System.Windows.Forms.ToolStripStatusLabel statusLabelConnected;
				private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel2;
				private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel1;
				private System.Windows.Forms.ToolStripStatusLabel labelSeparator;
				private System.Windows.Forms.ToolStripStatusLabel forceRightAlignment;
				private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel3;
				private System.Windows.Forms.ToolStripStatusLabel statusLabelTarget;
				private System.Windows.Forms.ToolStripStatusLabel statusConnectedImg;
				private System.Windows.Forms.ToolStripStatusLabel statusLabelIp;
				private System.Windows.Forms.Panel bodyPanel;
				private System.Windows.Forms.TabControl tabControlTop;
				private System.Windows.Forms.TabPage tabFullLog;
				private System.Windows.Forms.TabPage tabPage2;
				private System.Windows.Forms.DataVisualization.Charting.Chart logChart;
				private System.Windows.Forms.ContextMenuStrip contextMenuHistoryChart;
				private System.Windows.Forms.ToolStripSeparator toolStripSeparator3;
				private System.Windows.Forms.ToolStripMenuItem historyChartClear;
				private System.Windows.Forms.ToolStripSeparator toolStripSeparator4;
				private System.Windows.Forms.ToolStripMenuItem actionClearAll;
				private System.Windows.Forms.ToolStripStatusLabel statusLabelFilters;
				private System.Windows.Forms.ToolStripStatusLabel filtersTextStatus;
				private System.Windows.Forms.ToolStripStatusLabel statusMenuLabel;
				private System.Windows.Forms.ToolStripStatusLabel menuStatusText;
				private System.Windows.Forms.ToolStripMenuItem historyChartViewMenu;
				private System.Windows.Forms.ImageList imgList32;
				private System.Windows.Forms.TabPage tabPage1;
				private System.Windows.Forms.Panel panelHelp;
				private System.Windows.Forms.RichTextBox richTextBox1;
				private System.Windows.Forms.ToolStripSeparator toolStripSeparator5;
				private System.Windows.Forms.ToolStripMenuItem actionEditFiltersFile;
				private System.Windows.Forms.ToolStripTextBox lalala2;
				private System.Windows.Forms.ToolStripTextBox lalala;
				private System.Windows.Forms.ToolStripSeparator toolStripSeparator6;
				private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel4;
				private System.Windows.Forms.ToolStripStatusLabel statusLabelMode;
				private System.Windows.Forms.ToolStrip topToolStrip;
				private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
				private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel5;
				private System.Windows.Forms.ToolStripStatusLabel statusLabelMidi;
				private System.Windows.Forms.TabPage tabPageMidi;
				private System.Windows.Forms.ListView lvMidiLog;
				private System.Windows.Forms.ColumnHeader cHNum;
				private System.Windows.Forms.ColumnHeader cHLog;
				private System.Windows.Forms.ColumnHeader cHChannel;
				private System.Windows.Forms.ColumnHeader chMode;
				private System.Windows.Forms.ColumnHeader chCode;
				private System.Windows.Forms.ColumnHeader chVelocity;
				private System.Windows.Forms.Label label1;
    }
}

