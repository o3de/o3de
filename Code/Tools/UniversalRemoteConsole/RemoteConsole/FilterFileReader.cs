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

using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace RemoteConsole
{
		class FilterData
		{
			public string Tag						{ get; private set; }			// tag to search for in the log string
			public string RegExpText;															// customize reg exp search
			public string Label;																	// tab label
			public EMessageType MsgType { get; private set; }			// text type (error, warning, message...)
			public List<Exec> Execute		{ get; private set; }			// Commands to Execute
			public int TextColor = Convert.ToInt32("FFFFFF", 16); // White color by default
			
			public class Exec
			{
				public enum EExecType
				{
					eET_None = 0,
					eET_Macro,
					eET_DosCmd
				}

				public Exec(string typeStr_, string cmd_)
				{
					string s = typeStr_.ToLower();
					if (s == "macro")
						Type = EExecType.eET_Macro;
					else if (s == "doscmd")
						Type = EExecType.eET_DosCmd;
					else
						Type = EExecType.eET_None;

					Command = cmd_;
				}

				public EExecType Type { get; private set; }
				public string Command { get; private set; }
			}

			public FilterData(string tabName_, EMessageType type) 
			{
				this.Tag = tabName_;
				this.Label = tabName_;
				this.MsgType = type;

				Execute = new List<Exec>();
			}

			public void AddExecCommand(Exec e)
			{
				if (e != null)
				{
					if (Execute.Find(x => x.Command.ToLower() == e.Command.ToLower()) == null)
					{
						Execute.Add(e);
					}
				}
			}
		}
	
    class FilterFileReader
    {
			public List<FilterData> CreateStandardFilters()
			{
				List<FilterData> filterDataList = new List<FilterData>();
				FilterData data = new FilterData("[*Error*]", EMessageType.eMT_Error);
				data.Label = "Errors";
				data.TextColor = Convert.ToInt32("FF0000", 16);
				filterDataList.Add(data);
				data = new FilterData("[*Warnings*]", EMessageType.eMT_Warning); 
				data.Label = "Warnings";
				data.TextColor = Convert.ToInt32("0000FF", 16);
				filterDataList.Add(data);

				return filterDataList;
			}

			private int ExtractColor(string text)
			{
				Regex rx = new Regex("#?([0-9A-Fa-f]{6})");
				Match match = rx.Match(text);
				if (match.Success && match.Groups.Count == 2)
				{
					string hexColor = match.Groups[1].Value;
					return Convert.ToInt32(hexColor, 16);
				}
				return 0;
			}

			public List<FilterData> GetXmlFilters(string path)
			{
				if (System.IO.File.Exists(path) == false)
				{
					return null;
				}

				List<FilterData> filterDataList = new List<FilterData>();
				System.Xml.XmlDocument xd = new System.Xml.XmlDocument();
				xd.Load(path);
				System.Xml.XmlNodeList nodelist = xd.SelectNodes("/Filters/Filter");
				foreach (System.Xml.XmlNode node in nodelist) // for each <Filter> node
				{
					FilterData filter = new FilterData(node.Attributes.GetNamedItem("Name").Value, EMessageType.eMT_Message);
					System.Xml.XmlNode n = node.SelectSingleNode("Label");
					if (n != null) { filter.Label = n.InnerText; }
					n = node.SelectSingleNode("Color");
					if (n != null) { filter.TextColor = ExtractColor(n.InnerText); }
					n = node.SelectSingleNode("RegExp");
					if (n != null) { filter.RegExpText = n.InnerText; }
					n = node.SelectSingleNode("Exec");
					if (n != null)
					{
						filter.AddExecCommand(
							new FilterData.Exec(n.Attributes.GetNamedItem("Type").Value,
																	n.InnerText)
						);
					}

					filterDataList.Add(filter);
				}
				return filterDataList;
			}
    }
}
