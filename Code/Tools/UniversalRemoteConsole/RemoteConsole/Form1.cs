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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : * Main Screen (Form)


using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Windows.Forms;
using System.Text.RegularExpressions;
using System.IO;

namespace RemoteConsole
{
	public delegate void DelSendCommandToTarget(ParamsFileInfo.CEntryTag tag);
	public delegate void DelSettingsChange(CSettings newSettings);
	
  public partial class MainForm : Form, IRemoteConsoleClientListener
  {
		private const string kAppTitle = "Universal Remote Console v1.0";

		private CSettings currentSettings = new CSettings();

		private Timer timer = new Timer();

    private RemoteConsole rc;
    private List<string> recentCommands = new List<string>();
    private int currRecent = 0;

		private Regex rgxRemoveSpaceBegin = new Regex("^\\s*");
		private Regex rgxRemoveNewLine = new Regex("\\n");
		private Regex rgxGetIpPort = new Regex("([.\\w]+):([\\d]+)");
    
    private DateTime lastModificationMacros;
		private DateTime lastModificationFilters;
		private string ipAddress = "localhost";
		private string currentHostName = "localhost";
		private string currentPort = "";
		
		private ParamsFileInfo.CData paramsFileData;
		private LogBuffer logBuffer = new LogBuffer();
		private LogChart logChartMgr;
		private LogFilterManager logFilterMgr = new LogFilterManager();

		private List<Midi.InputDevice> midiInputDevice;
		private List<ListViewItem> scheduledMidiLogs = new List<ListViewItem>();

		private FormButtons  formButtons  = new FormButtons();
		private FormSliders  formSliders  = new FormSliders();
		private FormSettings formSettings = new FormSettings();
		private FormAbout    formAbout    = new FormAbout();

		private bool bConfigFilesOk = true;

		private object lockerMidiLog = new object();

		public void DelSendCommandToTarget(ParamsFileInfo.CEntryTag tag)
		{
			sendMenuCommand(tag);
		}

		public void DelSettingsChange(CSettings newSettings)
		{
			currentSettings.DebugMidi = newSettings.DebugMidi;
			if (currentSettings.Mode != newSettings.Mode)
				ModifyMode(newSettings.Mode);
		}

		public MainForm()
		{
      InitializeComponent();				
      updateConfig(true);

			rc = new RemoteConsole();
			rc.SetListener(this);
      updateIp();
      rc.Start();
			currentPort = rc.Port.ToString();
							
			logChartMgr = new LogChart(logChart);
			logFilterMgr.SetFullLogTab(tabFullLog);

			formSettings.SetSettingsChangeDelegate(DelSettingsChange);

			InitMidi();
		}

		// ---- Main Events ----
		// ----------------------------------------------------------
		private void Form1_Load(object sender, EventArgs e)
		{
			timer.Interval = 100;
			timer.Tick += new EventHandler(onTimer);
			timer.Enabled = true;
		}

    public void OnLogMessage(SLogMessage message)
    {
			System.Drawing.Color col = System.Drawing.Color.White;
        switch (message.Type)
        {
					case EMessageType.eMT_Message: col = System.Drawing.Color.White; break;
					case EMessageType.eMT_Warning: col = System.Drawing.Color.Yellow; message.Message = "[WARNING] " + message.Message; break;
					case EMessageType.eMT_Error: col = System.Drawing.Color.Red; message.Message = "[ERROR] " + message.Message; break;
        }
				addLine(message.Message, col, message.Type);		
    }

    public void OnAutoCompleteDone(List<string> autoCompleteList)
    {
        AutoCompleteStringCollection col = new AutoCompleteStringCollection();
        for (int i = 0; i < autoCompleteList.Count; ++i)
            col.Add(autoCompleteList[i]);
        editInput.AutoCompleteCustomSource = col;
    }

    public void OnConnected()
    {
        setConnected(true);
    }

    public void OnDisconnected()
    {
        setConnected(false);
    }

    protected override void OnFormClosing(FormClosingEventArgs e)
    {
        rc.Stop();
    }
			
		// ---- Update ----
		// ----------------------------------------------------------
		private void onTimer(Object myObject, EventArgs myEventArgs)
		{
			List<FilterData.Exec> toBeExecuted = new List<FilterData.Exec>();
			rc.PumpEvents();

			// 
			if (currentSettings.Mode != CSettings.EMode.eM_Full)
			{
				logBuffer.clear();
				updateConfig(false);
				updateFilters(false);
				return;
			}

			logBuffer.addToTextBox(fullLogConsole, false);

			updateConfig(false);
			updateFilters(false);

			logFilterMgr.UpdateTabs(logBuffer, ref toBeExecuted);

			updateGraph();

			logBuffer.clear();

			// Apply Commands
			System.Threading.Tasks.Parallel.ForEach(toBeExecuted, exec =>
				{
					switch (exec.Type)
					{
						case FilterData.Exec.EExecType.eET_Macro:
							ParamsFileInfo.CEntryTag tag = GetCommandTagFromMacroName(exec.Command);
							if (tag != null)
								sendMenuCommand(tag);
							break;
						case FilterData.Exec.EExecType.eET_DosCmd:
							System.Diagnostics.Process.Start("cmd.exe", @"/C " + exec.Command);
							break;
					}
				}
			);

			// midi logs
			UpdateMidiLogUi();
		}

		private void WarnAboutMissingFiles()
		{
			if (bConfigFilesOk == true)
			{
				bConfigFilesOk = false;
				const string message =
							 "Please place filters.xml and params.xml in the same folder as the executable.";
				const string caption = "Missing Config Files!!";
				var result = MessageBox.Show(message, caption,
																		 MessageBoxButtons.OK,
																		 MessageBoxIcon.Error);
			}
		}

		private void updateConfig(bool force)
		{
			System.IO.FileInfo fileInfo = new System.IO.FileInfo(Common.MenusFileFullPath);

			if (fileInfo.Exists == false)
			{
				menuStatusText.Text = "Missing";

				WarnAboutMissingFiles();
				return;
			}

			bConfigFilesOk = true;

			if (!force && fileInfo.LastWriteTime == lastModificationMacros)
				return;

			// Read Params File
			ParamsFileReader paramsFileReader = new ParamsFileReader(Common.MenusFileFullPath);
			paramsFileData = paramsFileReader.GetXmlParams();

			// Clear tools bar
			this.topToolStrip.Items.Clear();

			// Put groups into tool bar

			// Default ones
			CreateStandardMenus();

			// Custom Menus
			foreach (ParamsFileInfo.CGroup g in paramsFileData.Groups)
			{
				CreateMenuComponent(g);
			}

			lastModificationMacros = fileInfo.LastWriteTime;

			menuStatusText.Text = paramsFileData.Groups.Count.ToString();
		}

		private void updateFilters(bool force)
		{
			System.IO.FileInfo fileInfo = new System.IO.FileInfo(Common.FiltersFileFullPath);

			if (fileInfo.Exists == false)
			{
				filtersTextStatus.Text = "Missing";
				return;
			}

			if (!force && fileInfo.LastWriteTime == lastModificationFilters)
				return;

			FilterFileReader filterReader = new FilterFileReader();
			List<FilterData> filters = null;

			// stats
			int totalNumFilters = 0;

			// clear tabs
			tabControlFilters.TabPages.Clear();
			logFilterMgr.Clear();

			logChartMgr.ClearSeries();
			logChartMgr.AddSeries(LogFilterManager.TotalCountSeriesName);
			logFilterMgr.StatsHistory.AddSeries(LogFilterManager.TotalCountSeriesName);

			// Create Default tabs
			filters = filterReader.CreateStandardFilters();
			foreach (FilterData filter in filters)
			{
				logFilterMgr.AddFilter(filter, tabControlFilters, this.contextMenuTabs);
				logFilterMgr.StatsHistory.AddSeries(filter.Label);
				logChartMgr.AddSeries(filter.Label);
			}
			totalNumFilters += filters.Count;

			// Create custom filters
			filters = filterReader.GetXmlFilters(Common.FiltersFileFullPath);
			foreach (FilterData filter in filters)
			{
				logFilterMgr.AddFilter(filter, tabControlFilters, this.contextMenuTabs);
				logFilterMgr.StatsHistory.AddSeries(filter.Label);
				logChartMgr.AddSeries(filter.Label);
			}
			totalNumFilters += filters.Count;


			lastModificationFilters = fileInfo.LastWriteTime;
			filtersTextStatus.Text = totalNumFilters.ToString();
		}

		private void updateGraph()
		{
			LogFilterManager.History history = logFilterMgr.StatsHistory;
			for (int i = 0; i < history.Data.Count; ++i)
			{
				LogFilterManager.SingleFilterHistory h = history.Data[i];

				int idx = 0;

				h.ResetIterator();
				do
				{
					logChartMgr.SetYValue(h.Name, idx++, h.Buffer[h.Iterator]);

					if (idx >= LogFilterManager.SingleFilterHistory.BufferSize)
						idx = LogFilterManager.SingleFilterHistory.BufferSize - 1; // it should never happen, but just in case

				} while (h.NextIterator());
			}
			logChartMgr.Refresh();

		}

		// ---- Log Context menu ----
		// ----------------------------------------------------------
		private void actionClear_Click(object sender, EventArgs e)
		{
			Control ctrl = ((ContextMenuStrip)(((ToolStripMenuItem)sender).Owner)).SourceControl;
			logFilterMgr.ExecuteContextMenu(ctrl, LogFilterManager.EContextMenuAction.eCM_Clear);
		}

		private void actionClearAll_Click(object sender, EventArgs e)
		{
			//logFilterMgr.ClearTextBoxes();
			logFilterMgr.ExecuteContextMenu(null, LogFilterManager.EContextMenuAction.eCM_ClearAllLogs);
		}

		private void actionCopy_Click(object sender, EventArgs e)
		{
			Control ctrl = ((ContextMenuStrip)(((ToolStripMenuItem)sender).Owner)).SourceControl;
			logFilterMgr.ExecuteContextMenu(ctrl, LogFilterManager.EContextMenuAction.eCM_Copy);
		}

		private void actionSelectAll_Click(object sender, EventArgs e)
		{
			Control ctrl = ((ContextMenuStrip)(((ToolStripMenuItem)sender).Owner)).SourceControl;
			logFilterMgr.ExecuteContextMenu(ctrl, LogFilterManager.EContextMenuAction.eCM_SelectAll);
		}

		private void actionEditFiltersFile_Click(object sender, EventArgs e)
		{
			// edit filters file
			System.Diagnostics.Process.Start(/*"notepad.exe", */Common.FiltersFileFullPath);
		}

		// ---- Chart Context menu ----
		// ----------------------------------------------------------
		private void historyChartClear_Click(object sender, EventArgs e)
		{
			logFilterMgr.ClearHistory();
			logChartMgr.ClearData();
		}

		private void contextMenuHistoryChart_Opening(object sender, CancelEventArgs e)
		{
			// Clear Series
			historyChartViewMenu.DropDownItems.Clear();

			// Populate series items
			foreach (var s in logChartMgr.GetSeries())
			{
				ToolStripMenuItem item = new ToolStripMenuItem(s.Name);
				item.CheckOnClick = true;
				item.ToolTipText = "Click to Toggle. Ctrl-Click to Toggle all. Shift-Click makes selected item the only one selected/deselected";
				item.Click += new System.EventHandler(this.showHideLegend);

				if (s.IsVisibleInLegend)
				{
					item.Checked = true;
				}
				historyChartViewMenu.DropDownItems.Add(item);
			}
		}

		private void showHideLegend(object sender, EventArgs e)
		{
			System.Windows.Forms.ToolStripMenuItem b = (System.Windows.Forms.ToolStripMenuItem)sender;
			if (b != null)
			{
				logChartMgr.ShowHideLegend(b.Text, b.Checked);
			}
		}

		// ---- Menu Selection ----
		// ----------------------------------------------------------
		private void editInput_KeyUp(object sender, KeyEventArgs e)
		{
			if (e.KeyCode == Keys.Enter)
			{
				sendUserTypedCommand(editInput.Text);
				editInput.Text = "";
				currRecent = 0;
			}
			if (e.KeyCode == Keys.Up && e.Shift)
			{
				if (recentCommands.Count - currRecent > 0)
				{
					editInput.Text = recentCommands[recentCommands.Count - ++currRecent];
					editInput.SelectionStart = editInput.Text.Length;
				}
			}
			if (e.KeyCode == Keys.Down && e.Shift)
			{
				if (currRecent > 1)
				{
					editInput.Text = recentCommands[recentCommands.Count - --currRecent];
					editInput.SelectionStart = editInput.Text.Length;
				}
			}
		}

		private void editIpTB_KeyUp(object sender, KeyEventArgs e)
		{
			if (e.KeyCode == Keys.Enter)
			{
				ipAddress = ((ToolStripTextBox)sender).Text;
				updateIp();
			}
			else if (e.KeyCode == Keys.Escape)
			{
				((ToolStripTextBox)sender).Undo();
			}
		}

		private void portTB_KeyPressed(object sender, KeyPressEventArgs e)
		{
			e.Handled = !char.IsDigit(e.KeyChar) && !char.IsControl(e.KeyChar);
		}

		private void portTB_KeyUp(object sender, KeyEventArgs e)
		{
			if (e.KeyCode == Keys.Enter)
			{
				currentPort = ((ToolStripTextBox)sender).Text;
				updateIp();
				rc.SetPort(currentPort);
				UpdateUiStatus();
			}
			else if (e.KeyCode == Keys.Escape)
			{
				((ToolStripTextBox)sender).Undo();
			}
		}

		private void ModifyMode(CSettings.EMode newMode)
		{
			currentSettings.Mode = newMode;
			UpdateUiStatus();

			if (newMode == CSettings.EMode.eM_FileOnly)
			{
				rc.Stop();
			}
			else
			{
				rc.Start();
			}
		}

		private void genericMenu_ItemClicked(object sender, ToolStripItemClickedEventArgs e)
		{
			// Select a preset target
			if (e.ClickedItem.Tag != null)
			{
				ParamsFileInfo.CEntryTag tag = (ParamsFileInfo.CEntryTag)e.ClickedItem.Tag;
				sendMenuCommand(tag);
			}
			else
			{
				// edit macros file
				System.Diagnostics.Process.Start(Common.MenusFileFullPath);
			}
		}

		private void macroButton_ItemClicked(object sender, EventArgs e)
		{
			// Select a preset target
			System.Windows.Forms.ToolStripButton b = (System.Windows.Forms.ToolStripButton)sender;
			if (b.Tag != null)
			{
				ParamsFileInfo.CEntryTag tag = (ParamsFileInfo.CEntryTag)b.Tag;
				sendMenuCommand(tag);
			}
		}

		// ---- Helper Functions ----
		// ----------------------------------------------------------
		private void ParseIpPort(string ipPort, ref string ip, ref string port)
		{
			Match match = rgxGetIpPort.Match(ipPort);
			if (match.Success && match.Groups.Count == 3)
			{
				ip = match.Groups[1].Value;
				port = match.Groups[2].Value;
			}
			else
			{
				ip = "localhost";
				port = "0";
			}
		}

		private void sendMenuCommand(ParamsFileInfo.CEntryTag tag)
		{
			List<string> commands = (tag.ModCmds == null || tag.ModCmds.Count == 0) ? tag.Entry.Data : tag.ModCmds;

			if (tag.GroupSubType == ParamsFileInfo.CGroup.EGroupSubType.eSGT_Targets)
			{
				if (commands.Count > 0)
				{
					ParseIpPort(commands[0], ref ipAddress, ref currentPort);
					currentHostName = tag.Entry.Name;
					updateIp();
					rc.SetPort(currentPort);
					//editIpTB.Text = tag;
					UpdateUiStatus();
					return;
				}
			}

			switch (tag.GroupType)
			{
				case ParamsFileInfo.CGroup.EGroupType.eGT_MacrosMenu:
				case ParamsFileInfo.CGroup.EGroupType.eGT_MacrosButton:
				case ParamsFileInfo.CGroup.EGroupType.eGT_MacrosSlider:
				case ParamsFileInfo.CGroup.EGroupType.eGT_MacrosToggle:
					{
						for (int k = 0; k < commands.Count; ++k)
							rc.ExecuteConsoleCommand(commands[k]);
					}
					break;
				case ParamsFileInfo.CGroup.EGroupType.eGT_GamePlayMenu:
					{
						for (int k = 0; k < commands.Count; ++k)
							rc.ExecuteGameplayCommand(commands[k]);
					}
					break;
			}
		}

		private System.Drawing.Image GetTargetImage(string targetName)
		{
			int idx = imgList.Images.IndexOfKey("UnknownTarget");
			if (targetName.Length > 0)
			{
				string tgt = targetName;
				if (targetName[0] == '[' && targetName[targetName.Length-1] == ']')
				{
					tgt = targetName.Substring(1, targetName.Length - 2);
				}

				if (tgt.ToLower() == "pc")
					idx = imgList.Images.IndexOfKey("WinLogoImg16");
				else if (tgt.ToLower() == "provo")
					idx = imgList.Images.IndexOfKey("provo");
				else if (tgt.ToLower() == "xenia")
					idx = imgList.Images.IndexOfKey("xenia");
				else if (tgt.ToLower() == "custom_ip")
					idx = imgList.Images.IndexOfKey("IpImg");
				else if (tgt.ToLower() == "custom_port")
					idx = imgList.Images.IndexOfKey("PortImg");
			}

			return imgList.Images[idx];
		}

		private void CreateStandardMenus()
		{
			int imgIdx = -1;
			System.Windows.Forms.ToolStripDropDownButton menuItem = new System.Windows.Forms.ToolStripDropDownButton();

			// ---------- Tools -----------------
			System.Windows.Forms.ToolStripDropDownButton windowsItem = new System.Windows.Forms.ToolStripDropDownButton();
			windowsItem.Text = "Tools";
			imgIdx = imgList32.Images.IndexOfKey("ToolsImg32");
			if (imgIdx >= 0)
			{
				windowsItem.DisplayStyle = ToolStripItemDisplayStyle.ImageAndText;
				windowsItem.Image = imgList32.Images[imgIdx];
			}

			ContextMenuStrip windowsMenuItem = new ContextMenuStrip();
			windowsMenuItem.ImageList = imgList32;
			// Sliders
			windowsMenuItem.Items.Add("Sliders");
			windowsMenuItem.Items[windowsMenuItem.Items.Count - 1].Tag = CSettings.EMode.eM_Full;
			windowsMenuItem.Items[windowsMenuItem.Items.Count - 1].ToolTipText = "Sliders window";
			imgIdx = imgList32.Images.IndexOfKey("SlidersImg32");
			if (imgIdx > -1) windowsMenuItem.Items[windowsMenuItem.Items.Count - 1].ImageIndex = imgIdx;
			windowsMenuItem.Items[windowsMenuItem.Items.Count - 1].Click += new System.EventHandler(this.openSlidersWindowBt_Click);
			
			// Toggle Buttons
			windowsMenuItem.Items.Add("Toggle Buttons");
			windowsMenuItem.Items[windowsMenuItem.Items.Count - 1].Tag = CSettings.EMode.eM_Full;
			windowsMenuItem.Items[windowsMenuItem.Items.Count - 1].ToolTipText = "Opens the Toggle Buttons window";
			imgIdx = imgList32.Images.IndexOfKey("UpIcon32");
			if (imgIdx > -1) windowsMenuItem.Items[windowsMenuItem.Items.Count - 1].ImageIndex = imgIdx;
			windowsMenuItem.Items[windowsMenuItem.Items.Count - 1].Click += new System.EventHandler(this.openToggleButtonsWindow_Click);

			// Settings
			windowsMenuItem.Items.Add("Settings");
			windowsMenuItem.Items[windowsMenuItem.Items.Count - 1].Tag = CSettings.EMode.eM_Full;
			windowsMenuItem.Items[windowsMenuItem.Items.Count - 1].ToolTipText = "Opens the settings window";
			imgIdx = imgList32.Images.IndexOfKey("SettingsImg32");
			if (imgIdx > -1) windowsMenuItem.Items[windowsMenuItem.Items.Count - 1].ImageIndex = imgIdx;
			windowsMenuItem.Items[windowsMenuItem.Items.Count - 1].Click += new System.EventHandler(this.openSettingsWindow_Click);

			// About
			windowsMenuItem.Items.Add("About");
			windowsMenuItem.Items[windowsMenuItem.Items.Count - 1].Tag = CSettings.EMode.eM_Full;
			windowsMenuItem.Items[windowsMenuItem.Items.Count - 1].ToolTipText = "Opens the about window";
			imgIdx = imgList32.Images.IndexOfKey("BubbleImg32");
			if (imgIdx > -1) windowsMenuItem.Items[windowsMenuItem.Items.Count - 1].ImageIndex = imgIdx;
			windowsMenuItem.Items[windowsMenuItem.Items.Count - 1].Click += (sender, e) => { formAbout.Show(); };

			// Add Menu to Stripe
			windowsItem.DropDown = windowsMenuItem;
			this.topToolStrip.Items.Add(windowsItem);

			// ------------------------------------- Toggle Window ------------------------------------------------------------------
			if (paramsFileData.ShouldGroupShowInMenuBar("toggles"))
			{
				System.Windows.Forms.ToolStripButton toggleMenu = new System.Windows.Forms.ToolStripButton();
				toggleMenu.Text = "Toggles";
				imgIdx = imgList32.Images.IndexOfKey("UpIcon32");
				if (imgIdx >= 0)
				{
					toggleMenu.DisplayStyle = ToolStripItemDisplayStyle.ImageAndText;
					toggleMenu.Image = imgList32.Images[imgIdx];
				}
				toggleMenu.Tag = CSettings.EMode.eM_Full;
				toggleMenu.ToolTipText = "Opens the Toggle Buttons window";
				toggleMenu.Click += new System.EventHandler(this.openToggleButtonsWindow_Click);

				// Add Menu to Stripe
				this.topToolStrip.Items.Add(toggleMenu);
			}

			// ------------------------------------- Sliders Window ------------------------------------------------------------------
			if (paramsFileData.ShouldGroupShowInMenuBar("sliders"))
			{
				System.Windows.Forms.ToolStripButton slidersMenu = new System.Windows.Forms.ToolStripButton();
				slidersMenu.Text = "Sliders";
				imgIdx = imgList32.Images.IndexOfKey("SlidersImg32");
				if (imgIdx >= 0)
				{
					slidersMenu.DisplayStyle = ToolStripItemDisplayStyle.ImageAndText;
					slidersMenu.Image = imgList32.Images[imgIdx];
				}
				slidersMenu.Tag = CSettings.EMode.eM_Full;
				slidersMenu.ToolTipText = "Opens the Toggle Buttons window";
				slidersMenu.Click += new System.EventHandler(this.openSlidersWindowBt_Click);

				// Add Menu to Stripe
				this.topToolStrip.Items.Add(slidersMenu);
			}
		}

		private void CreateMenuComponent(ParamsFileInfo.CGroup group)
		{
			if (group != null && group.Name.Length > 0)
			{
				bool isTargetGroup = group.GType == ParamsFileInfo.CGroup.EGroupType.eGT_Targets;
				bool isMacroGroup = group.GType == ParamsFileInfo.CGroup.EGroupType.eGT_MacrosMenu;

				// Create menu item
				switch (group.GType)
				{
					case ParamsFileInfo.CGroup.EGroupType.eGT_MacrosMenu:
					case ParamsFileInfo.CGroup.EGroupType.eGT_GamePlayMenu:
					case ParamsFileInfo.CGroup.EGroupType.eGT_Targets:
						{
							System.Windows.Forms.ToolStripDropDownButton menuItem = new System.Windows.Forms.ToolStripDropDownButton();
									
							// Create and Populate DropDown menu
							ContextMenuStrip contextMenu = new ContextMenuStrip();
							foreach (ParamsFileInfo.CEntry item in group.Entries)
							{
								contextMenu.Items.Add(item.Name);
								contextMenu.Items[contextMenu.Items.Count - 1].Tag = item.GenerateEntryTag(group);
								contextMenu.Items[contextMenu.Items.Count - 1].ToolTipText = item.GetDataAsString();
								if (isTargetGroup)
								{
									contextMenu.Items[contextMenu.Items.Count - 1].Image = GetTargetImage(item.Name);
								}
							}

							if (isTargetGroup)
							{
								ToolStripSeparator sep = new System.Windows.Forms.ToolStripSeparator();
								contextMenu.Items.Add(sep);

								contextMenu.Items.Add("Custom IP");
								contextMenu.Items[contextMenu.Items.Count - 1].Enabled = false;
								contextMenu.Items[contextMenu.Items.Count - 1].Image = GetTargetImage("custom_ip");

								// Add edit IP text box
								ToolStripTextBox ip = new ToolStripTextBox("editIpTb");
								ip.AcceptsReturn = true;
								ip.BackColor = System.Drawing.Color.LightGray;
								ip.BorderStyle = BorderStyle.FixedSingle;
								ip.KeyUp += new System.Windows.Forms.KeyEventHandler(this.editIpTB_KeyUp);
								contextMenu.Items.Add(ip);
								contextMenu.Items[contextMenu.Items.Count - 1].ToolTipText = "Set custom IP";

								ToolStripSeparator sep2 = new System.Windows.Forms.ToolStripSeparator();
								contextMenu.Items.Add(sep2);
								
								contextMenu.Items.Add("Custom Port");
								contextMenu.Items[contextMenu.Items.Count - 1].Enabled = false;
								contextMenu.Items[contextMenu.Items.Count - 1].Image = GetTargetImage("custom_port");
								
								// Add edit Port text box
								ToolStripTextBox port = new ToolStripTextBox("portTb");
								port.AcceptsReturn = true;
								port.BackColor = System.Drawing.Color.LightGray;
								port.BorderStyle = BorderStyle.FixedSingle;
								port.Text = "4600";
								port.KeyUp += new System.Windows.Forms.KeyEventHandler(this.portTB_KeyUp);
								port.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.portTB_KeyPressed);
								contextMenu.Items.Add(port);
								contextMenu.Items[contextMenu.Items.Count - 1].ToolTipText = "Set custom port";
							}

							// Register ItemClicked Event
							contextMenu.ItemClicked += new System.Windows.Forms.ToolStripItemClickedEventHandler(this.genericMenu_ItemClicked);
							// Assign drop down menu element to menu
							menuItem.DropDown = contextMenu;
							menuItem.Name = "menu" + group.Name;
							menuItem.Text = group.Name;

							int imgIndex = -1;
							if (group.IconPath != null)
							{
								string fullPath = Common.IconsPath + group.IconPath;
								imgIndex = Common.AddImageFileToImageList(fullPath, imgList32);
							}

							if (imgIndex == -1)
							{
								// Hard coded images
								if (group.Name.ToLower() == "maps") imgIndex = imgList32.Images.IndexOfKey("EarthImg32");
								else if (isMacroGroup) imgIndex = imgList32.Images.IndexOfKey("RadImg32");
								else if (group.Name.ToLower() == "gameplays") imgIndex = imgList32.Images.IndexOfKey("GamePlayImg");
								else if (isTargetGroup) imgIndex = imgList32.Images.IndexOfKey("TargetImg2");
								else imgIndex = imgList32.Images.IndexOfKey("ExtraMacrosImg");
							}

							if (imgIndex >=0)
							{
								menuItem.DisplayStyle = ToolStripItemDisplayStyle.ImageAndText;
								menuItem.Image = imgList32.Images[imgIndex];
							}

							// Add menu to menu strip
							this.topToolStrip.Items.Add(menuItem);
						}
						break;
					case ParamsFileInfo.CGroup.EGroupType.eGT_MacrosButton:
						{
							// Generate a Button per entry
							foreach (ParamsFileInfo.CEntry item in group.Entries)
							{
								System.Windows.Forms.ToolStripButton button = new System.Windows.Forms.ToolStripButton();
								// Assign drop down menu element to menu
								//menuItem.Name = "menu" + group.name;
								button.Text = item.Name;
								button.DisplayStyle = ToolStripItemDisplayStyle.Text;
								button.Enabled = true;
								button.Visible = true;
								button.Tag = item.GenerateEntryTag(group);
								button.ToolTipText = item.GetDataAsString();
								// Register ItemClicked Event
								button.Click += new System.EventHandler(this.macroButton_ItemClicked);
								// Add menu to menu strip
								this.topToolStrip.Items.Add(button);

								if (item.IconPath != null)
								{
									string fullPath = Common.IconsPath + item.IconPath;
									int imgIndex = Common.AddImageFileToImageList(fullPath, imgList32);
									if (imgIndex >= 0)
									{
										button.DisplayStyle = ToolStripItemDisplayStyle.ImageAndText;
										button.Image = imgList32.Images[imgIndex];
									}
								}

								// Generate button
// 								System.Windows.Forms.Button b = new System.Windows.Forms.Button();
// 								b.Text = item.Name;
// 								b.Tag = item.GenerateEntryTag(group);
// 								b.FlatStyle = FlatStyle.Flat;
// 								b.Click += new System.EventHandler(this.macroButton_ItemClicked);
// 
// 								formButtons.AddButton(b);
							}
						}
						break;
					case ParamsFileInfo.CGroup.EGroupType.eGT_MacrosSlider:
						{
							// Set up slider window
							formSliders.SetEntries(group.Entries, DelSendCommandToTarget);
						}
						break;
					case ParamsFileInfo.CGroup.EGroupType.eGT_MacrosToggle:
						{
							// Set up slider window
							formButtons.SetEntries(group.Entries, DelSendCommandToTarget);
						}
						break;
					default :
						break;
				}

				// Create separator
				//System.Windows.Forms.ToolStripSeparator separator = new System.Windows.Forms.ToolStripSeparator();
				// Add separator to menu strip
				//this.topToolStrip.Items.Add(separator);
			}
		}

    private string getCurrHostName()
    {
			return currentHostName;
    }

		private ParamsFileInfo.CEntryTag GetCommandTagFromMacroName(string macroName)
		{
			ParamsFileInfo.CGroup group = paramsFileData.GetGroup("Macro");
			ParamsFileInfo.CEntry entry = paramsFileData.GetItem("Macro", macroName);
			if (group != null && entry != null)
			{
				return entry.GenerateEntryTag(group);
			}
			return null;
		}

		private void addLine(string line, System.Drawing.Color color, EMessageType msgType)
    {
        // remove white spaces at the beginning and new line
        line = rgxRemoveSpaceBegin.Replace(line, "");
        line = rgxRemoveNewLine.Replace(line, "");
        logBuffer.addLine(line, color, msgType);
      }

		private void UpdateUiStatus()
		{
			// Host, Ip, etc...
			string currHost = getCurrHostName();
			statusLabelTarget.Text = currHost;
			statusLabelTarget.Image = GetTargetImage(currHost);
			statusLabelIp.Text = getCurrentHostIp();
			this.Text = kAppTitle + " (" + currHost + ")";
			if (rc != null)
			{
				statusLabelIp.Text += ":" + rc.Port.ToString();
				this.Text += ":" + rc.Port.ToString();
			}

			// Mode
			int idx = -1;
			switch (currentSettings.Mode)
			{
				case CSettings.EMode.eM_Full:
					statusLabelMode.Text = "[Net] Full Mode";
					idx = imgList32.Images.IndexOfKey("FullImg32");

					break;
				case CSettings.EMode.eM_CommandsOnly:
					statusLabelMode.Text = "[Net] Command Mode";
					idx = imgList32.Images.IndexOfKey("EmptyImg32");
					break;
				case CSettings.EMode.eM_FileOnly:
					statusLabelMode.Text = "[Offline] File Mode";
					idx = imgList32.Images.IndexOfKey("FileImg32");
					break;
			}
			if (idx > -1) statusLabelMode.Image = imgList32.Images[idx];
		}
    private void setConnected(bool connected)
    {
			UpdateUiStatus();
      if (connected)
			{
					statusLabelConnected.Text = "Connected";
					statusConnectedImg.Image = imgList.Images[1];
//                 rc.ExecuteGameplayCommand("SetViewMode:" + lastViewMode.ToString());
//                 rc.ExecuteGameplayCommand("SetFlyMode:" + lastFlyMode.ToString());
      }
      else
      {
					statusConnectedImg.Image = imgList.Images[0];
					statusLabelConnected.Text = "Connecting...";
      }
    }

    private void sendUserTypedCommand(string command)
    {
        if (recentCommands.Count == 0 || recentCommands[recentCommands.Count - 1] != command)
            recentCommands.Add(command);
        rc.ExecuteConsoleCommand(command);
    }

		private string getCurrentHostIp()
		{
			return ipAddress;
		}

    private void updateIp()
    {
			if (paramsFileData == null)
				return;

			// Find current host name from target group
			ParamsFileInfo.CGroup targetGroup = paramsFileData.GetTargetsGroup();
			if (targetGroup != null)
			{
				string fullAddr = ipAddress + ":" + currentPort;
				foreach (ParamsFileInfo.CEntry e in targetGroup.Entries)
				{
					currentHostName = "";
					if (e.Data.Count > 0 && e.Data[0] == fullAddr)
					{
						currentHostName = e.Name;
						break;
					}
				}
			}

			rc.SetServer(getCurrentHostIp());
    }

		private void MainForm_DragDrop(object sender, DragEventArgs e)
		{
			if (e.Data.GetDataPresent(DataFormats.FileDrop))
			{
				string[] files = (string[])e.Data.GetData(DataFormats.FileDrop);
				if (files.Length > 0)
				{
					ProcessLogFile(files[0]);
				}
			}
		}

		private void MainForm_DragEnter(object sender, DragEventArgs e)
		{
			if (currentSettings.Mode == CSettings.EMode.eM_FileOnly && e.Data.GetDataPresent(DataFormats.FileDrop))
				e.Effect = DragDropEffects.All;
			else
				e.Effect = DragDropEffects.None;
		}

		private void ProcessLogFile(string filePath)
		{
			updateFilters(true);

			List<FilterData.Exec> toBeExecuted = new List<FilterData.Exec>();
			LogBuffer fileBuffer = new LogBuffer();
			// read file and add process each line
			System.IO.StreamReader file = null;
			try
			{
				file = new System.IO.StreamReader(filePath);
			}
			catch (System.Exception)
			{
				return;
			}

			// Parse it
			Regex rgxWarning = new Regex("\\[Warning\\]");
			Regex rgxError = new Regex("\\[Error\\]");

			EMessageType msgType = EMessageType.eMT_Message;
			string line;
			while ((line = file.ReadLine()) != null)
			{
				// Parse line to infer message type
				if (rgxError.IsMatch(line))
					msgType = EMessageType.eMT_Error;
				else if (rgxWarning.IsMatch(line))
					msgType = EMessageType.eMT_Warning;
				else
					msgType = EMessageType.eMT_Message;

				fileBuffer.addLine(line, System.Drawing.Color.Black, msgType);
			}

			fileBuffer.addToTextBox(fullLogConsole, false, true); // add full text to the full log view
			logFilterMgr.UpdateTabs(fileBuffer, ref toBeExecuted);
		}

		private void openSlidersWindowBt_Click(object sender, EventArgs e)
		{
			formSliders.Show();
		}

		private void openSettingsWindow_Click(object sender, EventArgs e)
		{
			formSettings.Show();
		}

		private void openToggleButtonsWindow_Click(object sender, EventArgs e)
		{
			formButtons.Show();
		}

		// -----------------------------------------------------------------------------------------
		// -----------------------------------------------------------------------------------------
		// MIDI
		// -----------------------------------------------------------------------------------------
		// -----------------------------------------------------------------------------------------

		private void InitMidi()
		{
			lvMidiLog.AutoResizeColumns(ColumnHeaderAutoResizeStyle.ColumnContent);
			lvMidiLog.AutoResizeColumns(ColumnHeaderAutoResizeStyle.HeaderSize);
			lvMidiLog.Items.Clear();

			if (Midi.InputDevice.InstalledDevices.Count == 0)
			{
				UpdateMidiUi();
				return;
			}

			midiInputDevice = new List<Midi.InputDevice>();
			for (int i = 0; i < Midi.InputDevice.InstalledDevices.Count; ++i )
			{
				if (Midi.InputDevice.InstalledDevices[i] == null)
				{
					Console.WriteLine("No input devices {0}", i);
					continue;
				}
				else
				{
					midiInputDevice.Add(Midi.InputDevice.InstalledDevices[i]);
					Console.WriteLine("Adding MIDI Input Device {0} - {1}", i, midiInputDevice[midiInputDevice.Count - 1].Name);
				}
				midiInputDevice[i].MidiTrigger += new Midi.InputDevice.MidiHandler(this.OnMidi);
				try
				{
					midiInputDevice[i].Open();
					midiInputDevice[i].StartReceiving();
				}
				catch (System.Exception)
				{
					Console.WriteLine("[Error] Device {0} - {1} is already open!!", i, midiInputDevice[midiInputDevice.Count - 1].Name);
				}				
			}

			UpdateMidiUi();			
		}

		private void UpdateMidiUi()
		{
			int numDevConnected = midiInputDevice != null ? midiInputDevice.Count : 0;

			statusLabelMidi.Text = numDevConnected.ToString() + (numDevConnected != 1 ? " Devices" : " Device");
			int idx = imgList32.Images.IndexOfKey(numDevConnected > 0 ? "OkImg32" : "NoImg32");
			if (idx > -1) statusLabelMidi.Image = imgList32.Images[idx];
		}

		public void OnMidi(Midi.MidiMessage msg)
		{
			bool buttonPad = (msg.Mode == 9); // button pad with button pressed
			bool sliderPad = (msg.Mode == 11);
			bool isSlider = (msg.Code < 8 || (msg.Code > 15 && msg.Code < 24)) && sliderPad; ;
			bool isButtonPressed = buttonPad || (sliderPad && !isSlider && msg.Velocity == 127);

			if (!isSlider && !isButtonPressed)
				return;

			ParamsFileInfo.CEntry entry = null;
			ParamsFileInfo.CEntryTag tag = null;

			//Console.WriteLine("MSG: CH {0} - Mode {1} - Code {2} - Vel {3} {4} {5}", msg.Channel, msg.Mode, msg.Code, msg.Velocity, isSlider ? "- Slider" : "", isButtonPressed ? "- Button" : "");

			int pad = buttonPad ? 0 : sliderPad ? 1 : -1;

			int key = ParamsFileInfo.CMidiInfo.CreateStaticKey(msg.Code, pad);
			entry = ParamsFileInfo.CMidiMapping.GetEntry(key);
			if (entry != null)
			{
				tag = entry.GenerateEntryTag();
				if (tag != null)
				{
					if (isButtonPressed)
					{
						sendMenuCommand(tag);
					}
					else if (isSlider && entry.SliderParams != null)
					{
						float t = (float)msg.Velocity / 127f;
						float v = entry.SliderParams.Lerp(t);
						// Send commands
						tag.ModCmds = new List<string>();
						for (int k = 0; k < tag.Entry.Data.Count; ++k)
						{
							string s = v.ToString().Replace(',', '.');
							tag.ModCmds.Add(tag.Entry.Data[k].Replace("#", s));
						}
						sendMenuCommand(tag);
					}
				}
			}

			MidiLogRequest(isSlider ? "Slider" : isButtonPressed ? "Button" : "", msg);
		}

		private void MidiLogRequest(string text, Midi.MidiMessage msg)
		{
			if (currentSettings.DebugMidi == true)
			{
				ListViewItem lvi = new ListViewItem(lvMidiLog.Items.Count.ToString());
				lvi.SubItems.Add("["+msg.Device.Name+"] " + text);
				lvi.SubItems.Add(msg.Channel != -1 ? msg.Channel.ToString() : "");
				lvi.SubItems.Add(msg.Channel != -1 ? msg.Mode.ToString() : "");
				lvi.SubItems.Add(msg.Channel != -1 ? msg.Code.ToString() : "");
				lvi.SubItems.Add(msg.Channel != -1 ? msg.Velocity.ToString() : "");
				
				lock (lockerMidiLog)
				{
					scheduledMidiLogs.Add(lvi);
				}
			}
		}

		private void UpdateMidiLogUi()
		{
			if (currentSettings.DebugMidi == true && scheduledMidiLogs.Count > 0)
			{
				lock (lockerMidiLog)
				{
					foreach (var log in scheduledMidiLogs)
					{
						lvMidiLog.Items.Add(log);
					}
					scheduledMidiLogs.Clear();
				}
				lvMidiLog.EnsureVisible(lvMidiLog.Items.Count - 1);
			}
		}

        private void topToolStrip_ItemClicked(object sender, ToolStripItemClickedEventArgs e)
        {

        }
    }
}
