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

﻿/* 
 * Project: Universal Remote Console
 * File: LogFilterManager.cs
 * Author: Dario Sancho (2014)
 * 
 * Description: 
 *	Manages the display of log data
 *	Adds log display controls to the Main Form, keeps statistic data buffer, 
 *	handles context menus on display log controls
 */

#define USE_LIST_BOX

using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.Windows.Forms;

namespace RemoteConsole
{
  class LogFilterManager
  {
		public const string TotalCountSeriesName = "Total";
		private const string strFullLogTabName = "Full Log";
		
		public	History StatsHistory					{ get; private set; }
		private TabPage fullLogTabPtr;
		private List<LogData> customFilters;
		private List<LogData> standardFilters;
		
		public enum EContextMenuAction
		{
			eCM_SelectAll = 0,
			eCM_Clear,
			eCM_Copy,
			eCM_ClearAllLogs,
		}

		class LogData
		{
			public TabPage TabPtr								{ get; private set; }	
			public LogDisplayControl ControlPtr { get; private set; }
			public FilterData Data							{ get; private set; }
			public int LineCountPerTick					{ get; private set; }

			public LogData(LogDisplayControl ctrl, TabPage tab, FilterData data)
			{
				TabPtr = tab;
				ControlPtr = ctrl;
				Data = data;
				LineCountPerTick = 0;
			}

			public void AddLine(string line)
			{
				ControlPtr.AddLine(line);
				++LineCountPerTick;
			}

			public void ClearPerTickCounter() { LineCountPerTick = 0; }

			public void UpdateFilterLabel()
			{
				TabPtr.Text = Data.Label + "[" + ControlPtr.GetNumItems() + "]";
			}
			public void Clear()
			{
				ControlPtr.Clear();
				LineCountPerTick = 0;
				UpdateFilterLabel();
			}
		}

		public class SingleFilterHistory
		{
			public const int BufferSize = 600;
			public string Name				{ get; private set; }
			public int Back						{ get; private set; }
			public int Front					{ get; private set; }
			public List<float> Buffer { get; private set; }

			public int Iterator				{ get; private set; }

			public SingleFilterHistory(string name) 
			{
				Name = name;
				Buffer = new List<float>(BufferSize);
				for (int i = 0; i < BufferSize; ++i)
				{
					Buffer.Add(0);
				}
				Back = 0; Front = 0; 
			}
			public void Clear()
			{
				Back = Front = 0;
				ResetIterator();
				for (int i = 0; i < BufferSize; ++i)
						Buffer[i] = 0;
			}

			public void Add(float value)
			{
				Buffer[Front++] = value;
				if (Front >= BufferSize)
				{
					Front = 0;
				}

				if (Front <= Back)
					Back = Front + 1;

				if (Back >= BufferSize)
					Back = 0;
			}

			public void ResetIterator() { Iterator = Back; }
			public bool NextIterator() 
			{
				if (++Iterator >= BufferSize)
					Iterator = 0;
				
				return (Iterator != Front);
			}
		}

		public class History
		{
			public List<SingleFilterHistory> Data { get; private set; }

			public History() { Data =  new List<SingleFilterHistory>(); }

			public void Clear() 
			{
				foreach (var h in Data)
					h.Clear();
			}
			public void AddSeries(string name)
			{
				if (GetSeries(name) == null)
				{
					Data.Add(new SingleFilterHistory(name));
				}
			}
			public SingleFilterHistory GetSeries(string name)
			{
				string lowerName = name.ToLower();
				foreach (var h in Data)
				{
					if (h.Name.ToLower() == lowerName)
					{
						return h;
					}
				}
				return null;
			}
		}
		
		public LogFilterManager()
    {
			customFilters		= new List<LogData>();
			standardFilters = new List<LogData>();
			StatsHistory		= new History();
    }

		public void SetFullLogTab(TabPage tp) { fullLogTabPtr = tp;	}

		public void Clear()
		{
			customFilters.Clear();
			standardFilters.Clear();
			StatsHistory.Data.Clear();
		}

		public void ClearHistory()
		{
			StatsHistory.Clear();
		}

		public void AddFilter(FilterData data, TabControl targetTabControl, ContextMenuStrip contextMenuStrip)
		{		
			// Create Tab
			TabPage tab = new TabPage(data.Label);

			// Create Control to display the log info
#if USE_LIST_BOX
			LogDisplayControl ctl = new LogDisplayControl(LogDisplayControl.EType.eT_ListView, contextMenuStrip, data.TextColor);		
#else
			LogDisplayControl ctl = new LogDisplayControl(LogDisplayControl.EType.eT_RichTextBox, contextMenuStrip, data.TextColor);		
#endif
			// Add tab to tab controller
			tab.Controls.Add(ctl.GetControl());
			targetTabControl.TabPages.Add(tab);
			targetTabControl.Dock = DockStyle.Fill;

			// keep track
			LogData logData = new LogData(ctl, tab, data);

			if (data.MsgType == EMessageType.eMT_Message)
				customFilters.Add(logData);
			else
				standardFilters.Add(logData);
		}

		public bool UpdateTabs(LogBuffer buffer, ref List<FilterData.Exec> execList)
		{
			bool res = false;
			int totalCount = 0;

			if (buffer.IsBufferEmpty() == false)
			{
				// Full Log Tab
				fullLogTabPtr.Text = "Full Log [" + ((RichTextBox)fullLogTabPtr.Controls[0]).Lines.Length.ToString() + "]";
				
				// Standard Tabs				
				foreach (LogData filter in standardFilters)
				{
					List<string> lines = buffer.GetLinesInBuffer(filter.Data.MsgType);
					foreach (string line in lines)
					{
						filter.AddLine(line);
						totalCount++;
						res = true;
					}
					filter.UpdateFilterLabel();
				}
												
				// Custom Tabs
				List<string> lines1 = buffer.GetLinesInBuffer(EMessageType.eMT_Message);
				foreach (LogData filter in customFilters)
				{
					foreach (string line in lines1)
					{
						bool found = false;
						// Check Label
						if (line.Contains(filter.Data.Tag))
						{
							found = true;
						}
						// Check RegExp
						else if (filter.Data.RegExpText != null && filter.Data.RegExpText.Length > 0)
						{
							Regex rgx = new Regex(filter.Data.RegExpText);
							if (rgx.IsMatch(line))
							{
								found = true;
							}
						}

						if (found)
						{
							filter.AddLine(line);
							totalCount++;
							res = true;

							// Apply Exec Commands
							if (filter.Data.Execute != null)
							{
								foreach (FilterData.Exec e in filter.Data.Execute)
								{
									if (e.Type != FilterData.Exec.EExecType.eET_None)
										execList.Add(e);
								}
							}
						}
						
					}
					filter.UpdateFilterLabel();
				}
			}

			foreach (var f in standardFilters)
			{
				if (f.Data.Label != null)
					StatsHistory.GetSeries(f.Data.Label).Add(f.LineCountPerTick);

				f.ClearPerTickCounter();
			}

			foreach (var f in customFilters)
			{
				if (f.Data.Label != null)
					StatsHistory.GetSeries(f.Data.Label).Add(f.LineCountPerTick);

				f.ClearPerTickCounter();
			}

			var h = StatsHistory.GetSeries(LogFilterManager.TotalCountSeriesName);
			if (h != null)
				h.Add(totalCount);

			return res;
		}

		private void ExecuteContextMenu_InternalFullLog(Control ctrl, EContextMenuAction action)
		{
			switch (action)
			{
				case EContextMenuAction.eCM_SelectAll:
					((RichTextBox)ctrl).SelectAll();
					return;
				case EContextMenuAction.eCM_Clear:
					((RichTextBox)ctrl).Clear();
					return;
				case EContextMenuAction.eCM_Copy:
					((RichTextBox)ctrl).Copy();
					return;
			}
		}

		private void ExecuteContextMenu_Internal(LogData data, EContextMenuAction action)
		{
			switch (action)
			{
				case EContextMenuAction.eCM_SelectAll:
					data.ControlPtr.SelectAll();
					return;
				case EContextMenuAction.eCM_Clear:
					data.Clear();
					return;
				case EContextMenuAction.eCM_Copy:
					data.ControlPtr.Copy();
					return;
			}
		}

		public void ExecuteContextMenu(Control ctrl, EContextMenuAction action)
		{
			if (action == EContextMenuAction.eCM_ClearAllLogs)
			{
				((RichTextBox)fullLogTabPtr.Controls[0]).Clear();
				fullLogTabPtr.Text = strFullLogTabName + " [0]";

				foreach (LogData data in customFilters)
					data.Clear();
				foreach (LogData data in standardFilters)
					data.Clear();

				return;
			}

			if (fullLogTabPtr.Controls[0] == ctrl)
			{
				ExecuteContextMenu_InternalFullLog(ctrl, action);
				fullLogTabPtr.Text = strFullLogTabName + " [0]";
				return;
			}

			foreach (LogData data in customFilters)
			{
				Control c = data.ControlPtr.GetControl();
				if ( c== ctrl)
				{
					ExecuteContextMenu_Internal(data, action);
					return;
				}
			}

			foreach (LogData data in standardFilters)
			{
				Control c = data.ControlPtr.GetControl();
				if (c == ctrl)
				{
					ExecuteContextMenu_Internal(data, action);
					return;
				}
			}
		}
  }
}
