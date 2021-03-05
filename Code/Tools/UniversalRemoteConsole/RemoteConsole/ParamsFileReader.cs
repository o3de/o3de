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

// Description : * Reads the config file that defines the menus, including the macros, target devices, etc.
//               * It also defines the data structures to store this information


using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace RemoteConsole
{
	namespace ParamsFileInfo
	{
		static class Tools
		{
			private static Dictionary<string, CGroup.EGroupType> mapGroupName2Type = new Dictionary<string, CGroup.EGroupType>();

			public static void AddDefinitionItem(string groupName, string groupDefType)
			{
				CGroup.EGroupType eGroupType = GroupTypeStr2GroupType(groupDefType);
				mapGroupName2Type.Add(groupName, eGroupType);
			}

			public static void ClearDefinitions()
			{
				mapGroupName2Type.Clear();
			}

			public static CGroup.EGroupType GroupName2GroupType(string groupName)
			{
				CGroup.EGroupType eType;
				if (groupName != null && mapGroupName2Type.TryGetValue(groupName, out eType))
				{
					return eType;
				}
				return CGroup.EGroupType.eGT_MacrosMenu;
			}

			public static CGroup.EGroupType GroupTypeStr2GroupType(string typeStr)
			{
				if (typeStr != null)
				{
					string t = typeStr.ToLower();
					if (t == "menumacro") return CGroup.EGroupType.eGT_MacrosMenu;
					else if (t == "menugameplay") return CGroup.EGroupType.eGT_GamePlayMenu;
					else if (t == "buttonmacro") return CGroup.EGroupType.eGT_MacrosButton;
					else if (t == "slidermacro") return CGroup.EGroupType.eGT_MacrosSlider;
					else if (t == "togglemacro") return CGroup.EGroupType.eGT_MacrosToggle;
					else if (t == "menutarget") return CGroup.EGroupType.eGT_Targets;					
				}

				return CGroup.EGroupType.eGT_MacrosMenu;
			}

			public static CGroup.EGroupSubType GroupName2GroupSubType(string groupName)
			{
				return groupName.ToLower() == "targets" ? 
					ParamsFileInfo.CGroup.EGroupSubType.eSGT_Targets :
					ParamsFileInfo.CGroup.EGroupSubType.eSGT_None;
			}
		}

		static class CMidiMapping
		{
			private static Dictionary<int, CEntry> mapMidi2Entry = new Dictionary<int, CEntry>();

			public static void Clear() { mapMidi2Entry.Clear(); }
			public static void Insert(CMidiInfo midiInfo, CEntry entry) 
			{
				if (mapMidi2Entry.ContainsKey(midiInfo.CreateKey()) == false)
					mapMidi2Entry.Add(midiInfo.CreateKey(), entry);
			}
			public static CEntry GetEntry(int key)
			{
				CEntry entry;
				if (mapMidi2Entry.TryGetValue(key, out entry))
					return entry;
				return null;
			}

			private static int CreateKey(CMidiInfo midiInfo)
			{
				if (midiInfo != null)
					return midiInfo.Midi | (midiInfo.Pad << 16);

				return -1;
			}
		}
		public class CEntryTag
		{
			// e.g. 
			// [Macro=ScreenShot]
			// r_getscreenshot 2
			public CEntryTag(CGroup.EGroupType groupType, CGroup.EGroupSubType groupSubType, CEntry e) { GroupType = groupType; Entry = e; GroupSubType = groupSubType; }
			public CGroup.EGroupType		GroupType			{ get; private set; } // e.g. eGT_MacrosMenu
			public CGroup.EGroupSubType GroupSubType	{ get; private set; } // e.g. eSGT_None
			public CEntry								Entry					{ get; private set; } // ptr to entry it belongs to
			public List<string>					ModCmds;														// modified commands (e.g. when t_scale #)

		}

		public class CSliderParams
		{
			public CSliderParams(float min, float max, float delta, float current, bool forceInt) { Min = min; Max = max; Delta = delta; ForceInt = forceInt; CurrentValue = current; }
			public bool ForceInt = false;
			public float Min = 0;
			public float Max = 1;
			public float Delta = 0.2f;
			public float CurrentValue = 0;

			public float Lerp(float t)
			{
				t = Common.Clamp<float>(t, 0f, 1f);
				return (1f - t) * Min + t * Max;
			}
		}

		public class CToggleParams
		{
			public CToggleParams(int on, int off, string groupName, string itemName) { On = on; Off = off; GroupName = groupName; ItemName = itemName; }
			public int On = 0;
			public int Off = 1;
			public string GroupName;
			public string ItemName;
		}

		public class CMidiInfo
		{
			public CMidiInfo(int code, int pad) { Midi = code; Pad = pad; }
			public int Midi { get; private set; }	// Midi ASCI value associated with the key/slider pressed
			public int Pad { get; private set; }	// Midi ASCI value associated with the key/slider pressed

			public int CreateKey() { return CMidiInfo.CreateStaticKey(Midi, Pad); }

			public static int CreateStaticKey(int midiCode, int pad)
			{
				return midiCode | (pad << 16);
			}
		}
		public class CEntry
		{
			public CMidiInfo MidiInfo { get; private set; }
			public string IconPath { get; private set; }
			public string Group { get; private set; }	// e.g. Macro
			public string Name { get; private set; }	// e.g. ScreenShot
			public CGroup.EGroupType GroupType { get; private set; } // e.g. eGT_MacrosMenu
			public CGroup.EGroupSubType GroupSubType { get; private set; } // e.g. eSGT_None
			public List<string> Data { get; private set; } // e.g. r_getscreenshot 2
			public CSliderParams SliderParams;
			public CToggleParams ToggleParams;

			// e.g. 
			// [Macro=ScreenShot]
			// r_getscreenshot 2
			public CEntry(string group, string name, CMidiInfo midiInfo, string iconPath, CSliderParams sliderParams = null, CToggleParams toggleParams = null) 
			{
				IconPath = iconPath;
				MidiInfo = midiInfo;
				Group = group; Name = name; Data = new List<string>();
				GroupType = Tools.GroupName2GroupType(group);
				GroupSubType = Tools.GroupName2GroupSubType(group);
				if (sliderParams != null) SliderParams = sliderParams;
				if (toggleParams != null) ToggleParams = toggleParams;
			}
			
			public string GetDataAsString()
			{
				string str = "";
				foreach (string s in Data) str += s + '\n';
				return str;
			}

			public CEntryTag GenerateEntryTag(CGroup group)
			{
				return new CEntryTag(group.GType, group.GSubType, this);
			}

			public CEntryTag GenerateEntryTag()
			{
				return new CEntryTag(GroupType, GroupSubType, this);
			}
		}

		// Gathers information about a group
		public class CGroup
		{
			public enum EGroupType
			{
				eGT_MacrosMenu = 0,
				eGT_Targets,
				eGT_GamePlayMenu,
				eGT_MacrosButton,
				eGT_MacrosSlider,
				eGT_MacrosToggle
			}

			public enum EGroupSubType
			{
				eSGT_None = 0,
				eSGT_Targets
			}

			public string IconPath { get; private set; }
			public string Name { get; private set; }
			public EGroupType GType { get; private set; }
			public EGroupSubType GSubType { get; private set; }
			public List<CEntry> Entries { get; private set; }
			public bool ShowOnMenu { get; private set; }

			public CGroup(string groupName, EGroupSubType subType, string iconPath, bool showOnMenu)
			{
				IconPath = iconPath;
				Name = groupName; Entries = new List<CEntry>(); GSubType = subType;
				GType = Tools.GroupName2GroupType(groupName);
				ShowOnMenu = showOnMenu;
			}

			public void Add(CEntry entry) { Entries.Add(entry); }
			public CEntry GetEntry(string name)
			{
				string lowerName = name.ToLower();
				foreach (CEntry e in Entries)
				{
					if (e.Name.ToLower() == lowerName)
					{
						return e;
					}
				}
				return null;
			}
			
		}

		// Params file full data
		class CData
		{
			public CData() { Groups = new List<CGroup>(); }
			public List<CGroup> Groups { get; private set; }
			
			public void DeleteGroup(string groupName)
			{
				string lowerGroupName = groupName.ToLower();
				foreach (CGroup g in Groups)
				{
					if (g.Name.ToLower() == lowerGroupName)
					{
						Groups.Remove(g);
						return;
					}
				}
			}
			public CGroup GetGroup(string groupName)
			{
				string lowerGroupName = groupName.ToLower();
				foreach (CGroup g in Groups)
				{
					if (g.Name.ToLower() == lowerGroupName)
					{
						return g;
					}
				}
				return null;
			}

			public CGroup GetTargetsGroup()
			{
				return GetGroup("targets");
			}
			
			public bool AddItem(CEntry item, CGroup.EGroupSubType subType, string iconPath, bool showOnMenu)
			{
				// add item in a group
				CGroup g = GetGroup(item.Group);
				if (g == null)
				{
					g = new CGroup(item.Group, subType, iconPath, showOnMenu);
					//g.SetType(type);
					Groups.Add(g);
				}
				
				g.Add(item);

				return g != null;
			}

			public CEntry GetItem(string group, string name)
			{
				CGroup g = GetGroup(group);
				if (g != null)
				{
					return g.GetEntry(name);
				}

				return null;
			}

			public bool ShouldGroupShowInMenuBar(string group)
			{
				bool ret = false;
				var g = GetGroup(group);
				if (g != null)
				{
					ret = g.ShowOnMenu;
				}

				return ret;
			}
		}
	}

	// -------------------------------------------------------------
	
	class ParamsFileReader
	{
		private string path;

		public ParamsFileReader(string INIPath)
		{
			path = INIPath;
		}

		public ParamsFileInfo.CData GetXmlParams()
		{
			if (System.IO.File.Exists(path) == false)
			{
				return null;
			}

			ParamsFileInfo.CData res = new ParamsFileInfo.CData();
			ParamsFileInfo.CEntry entry = null;
			string iconPath = null;

			System.Xml.XmlDocument xd = new System.Xml.XmlDocument();
			try
			{
				xd.Load(path);
			}
			catch (System.Exception)
			{
				return null;
			}

			// read group types
			ParamsFileInfo.Tools.ClearDefinitions();
			ParamsFileInfo.CMidiMapping.Clear();

			System.Xml.XmlNode nodeDefs = xd.SelectSingleNode("/root/Definitions");
			foreach (System.Xml.XmlNode def in nodeDefs) // for each <Targets>, <Macros>, etc... node
			{
				ParamsFileInfo.Tools.AddDefinitionItem(def.Attributes.GetNamedItem("group").Value, def.Attributes.GetNamedItem("type").Value);
			}

			// read groups
			System.Xml.XmlNode nodeParams = xd.SelectSingleNode("/root/Parameters");
			foreach (System.Xml.XmlNode group in nodeParams) // for each <Targets>, <Macros>, etc... node
			{
				bool isSlidersGroup = group.Name.ToLower() == "sliders";
				bool isToggleGroup = group.Name.ToLower() == "toggles";
				bool showOnMenu = true;

				// check group attributes
				if (group.Attributes != null)
				{
					System.Xml.XmlNode n = group.Attributes.GetNamedItem("icon");
					iconPath = n != null ? n.InnerText : null;
					if (isSlidersGroup || isToggleGroup)
					{
						n = group.Attributes.GetNamedItem("onMenu");
						showOnMenu = n != null && n.InnerText.ToLower().Trim() == "true";
					}
				}
				ParamsFileInfo.CGroup.EGroupSubType subType = ParamsFileInfo.Tools.GroupName2GroupSubType(group.Name);

				foreach (System.Xml.XmlNode node in group) // e.g. for each <Target> node
				{
					if (node.NodeType != System.Xml.XmlNodeType.Element || node.Attributes.Count < 1 )
						continue;

					// header

					// MIDI
					int midi = -1, pad = 0;
					System.Xml.XmlNode n = node.Attributes.GetNamedItem("midi");
					if (n != null && IsNumeric(n.Value.Trim())) midi = int.Parse(n.Value.Trim());
					n = node.Attributes.GetNamedItem("pad");
					if (n != null && IsNumeric(n.Value.Trim())) pad = int.Parse(n.Value.Trim());
					ParamsFileInfo.CMidiInfo midiInfo = new ParamsFileInfo.CMidiInfo(midi,pad);

					// icon
					string entryIconPath = null;
					if (node.Attributes != null)
					{
						System.Xml.XmlNode nIcon = node.Attributes.GetNamedItem("icon");
						entryIconPath = nIcon != null ? nIcon.InnerText : null;
					}

					// Create entry
					entry = new ParamsFileInfo.CEntry(group.Name, node.Attributes.GetNamedItem("name").Value, midiInfo, entryIconPath);
					
					if (midi > -1)
					{
						ParamsFileInfo.CMidiMapping.Insert(midiInfo, entry);
					}

					if (subType == ParamsFileInfo.CGroup.EGroupSubType.eSGT_Targets)
					{
						string addr = "localhost:0";
						try
						{
							string ip = node.Attributes.GetNamedItem("ip").Value;
							string port = node.Attributes.GetNamedItem("port").Value;
							addr = ip + ":" + port;
						}
						catch (System.Exception){ }
						
						entry.Data.Add(addr);
					}
					else
					{
						foreach (System.Xml.XmlNode cvar in node) // e.g. for each <CVar str="r_displayInfo=0"/> node
						{
							entry.Data.Add(cvar.InnerText);
						}

						if (isSlidersGroup)
						{
							GetSliderParams(node, ref entry.SliderParams);
						}
						else if (isToggleGroup)
						{
							GetToggleButtonParams(node, ref entry.ToggleParams);
						}
					}

					// commit entry
					res.AddItem(entry, subType, iconPath, showOnMenu);
					entry = null;
				}
			}
		
			return res;
		}

		// ----------- Helper Functions ----------------
		private bool IsNumeric(string stringToTest)
		{
			float result;
			return float.TryParse(stringToTest, out result);
		}

		private void GetSliderParams(System.Xml.XmlNode node, ref ParamsFileInfo.CSliderParams sliderParams)
		{
			float min = 0f, max = 0f, delta = 0f, current = 0f;
			bool forceInt = false;
			int count = 0;
			System.Xml.XmlNode n = node.Attributes.GetNamedItem("min");
			if (n != null && IsNumeric(n.InnerText.Trim())) { ++count; min = float.Parse(n.InnerText.Trim(), System.Globalization.CultureInfo.InvariantCulture); }
			n = node.Attributes.GetNamedItem("max");
			if (n != null && IsNumeric(n.InnerText.Trim())) { ++count; max = float.Parse(n.InnerText.Trim(), System.Globalization.CultureInfo.InvariantCulture); }
			n = node.Attributes.GetNamedItem("delta");
			if (n != null && IsNumeric(n.InnerText.Trim())) { ++count; delta = float.Parse(n.InnerText.Trim(), System.Globalization.CultureInfo.InvariantCulture); }
			n = node.Attributes.GetNamedItem("forceInt");
			if (n != null) { forceInt = n.InnerText.Trim().ToLower() == "true"; }
			n = node.Attributes.GetNamedItem("default");
			if (n != null) { current = float.Parse(n.InnerText.Trim(), System.Globalization.CultureInfo.InvariantCulture); } else { current = min; }
			if (count == 3)
			{
				current = (current < min) ? min : (current > max) ? max : current;
				sliderParams = new ParamsFileInfo.CSliderParams(min, max, delta, current, forceInt);
			}
		}

		private void GetToggleButtonParams(System.Xml.XmlNode node, ref ParamsFileInfo.CToggleParams toggleParams)
		{
			int on = 1, off = 0;
			string groupName = "default";
			string itemName = "";
			System.Xml.XmlNode n = node.Attributes.GetNamedItem("group");
			groupName = (n != null) ? n.InnerText.Trim() : "default";
			n = node.Attributes.GetNamedItem("name");
			itemName = (n != null) ? n.InnerText.Trim() : "missing name";

			n = node.Attributes.GetNamedItem("on");
			if (n != null && IsNumeric(n.InnerText.Trim())) { on = int.Parse(n.InnerText.Trim(), System.Globalization.CultureInfo.InvariantCulture); }
			n = node.Attributes.GetNamedItem("off");
			if (n != null && IsNumeric(n.InnerText.Trim())) { off = int.Parse(n.InnerText.Trim(), System.Globalization.CultureInfo.InvariantCulture); }

			toggleParams = new ParamsFileInfo.CToggleParams(on, off, groupName, itemName);
		}

		

		// -------------------------------------------------------------
		// Gets the list of all macros of certain type
		// e.g. type = GamePlay
		// [GamePlay=Fly On]
		// -------------------------------------------------------------
#if OLD_STUFF
		public ParamsFileInfo.CData Parse()
		{

			return GetXmlParams();

			ParamsFileInfo.CData	res = new ParamsFileInfo.CData();
			ParamsFileInfo.CEntry entry = null;
			string line;

			Regex rgxWellFormedHeader = new Regex("\\[([\\w-]+)=(.+)\\]");	// e.g. [GroupName it belongs to = Item Name]
			Regex rgxHeader = new Regex("\\[(.*)\\]");											// e.g. [anything]

			// Can we open the parameters file?
			System.IO.StreamReader file = null;
			try
			{
				file = new System.IO.StreamReader(this.path);
			}
			catch (System.Exception)
			{
				return res;
			}

			// Parse it
			while ((line = file.ReadLine()) != null)
			{
				// Is it a comment?
				string l = line.Trim();
				if (l.Length > 0 && l[0] == '#')
				{
					continue;
				}

				Match match = rgxWellFormedHeader.Match(line);
				if (match.Success && match.Groups.Count == 3)
				{
					if (entry != null)
					{
						// commit current entry
						res.AddItem(entry);
						entry = null;
					}

					if (entry == null)
					{
						// line = proper header [GroupType=EntryName] - e.g. [Macro=ScreenShot]
						entry = new ParamsFileInfo.CEntry(match.Groups[1].Value, match.Groups[2].Value);
					}
					
				}
				else if (entry != null && line.Length > 0)
				{
					// is this a new header?
					bool isMatch = rgxHeader.IsMatch(line);
					if (isMatch == false)
					{
						// still data, so I add it e.g. 10.11.110.202 or r_displayInfo=1
						entry.Data.Add(line);
					}
				}
			}

			if (entry != null)
			{
				res.AddItem(entry);
				entry = null;
			}

			file.Close();

			// Adjust Group Types
			if (res != null)
			{
				ParamsFileInfo.CGroup definitions = res.GetGroup("DefGroup");
				if (definitions != null)
				{
					foreach (ParamsFileInfo.CEntry def in definitions.Entries)
					{
						ParamsFileInfo.CGroup group = res.GetGroup(def.Name);	// group to set the type to
						List<string> info = def.Data;	// type
						if (info.Count > 0)
						{
							group.SetType(info[0]);
						}
					}

					// Delete the Definition group
					res.DeleteGroup("DefGroup");
				}

			}

			// Return results
			return res;
		}
#endif
	}
}
