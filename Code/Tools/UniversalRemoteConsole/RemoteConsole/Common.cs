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
using System.Text;

namespace RemoteConsole
{
	public static class Common
	{
		public static readonly string FiltersFileFullPath = System.IO.Path.Combine( System.IO.Path.GetDirectoryName(System.Windows.Forms.Application.ExecutablePath), "filters.xml");
		public static readonly string MenusFileFullPath = System.IO.Path.Combine( System.IO.Path.GetDirectoryName(System.Windows.Forms.Application.ExecutablePath), "params.xml");
		public static readonly string IconsPath = System.IO.Path.GetDirectoryName(System.Windows.Forms.Application.ExecutablePath) + System.IO.Path.PathSeparator + "Icons" + System.IO.Path.PathSeparator;
		public static T Clamp<T>(this T val, T min, T max) where T : IComparable<T>
		{
			Comparer<T> comp = Comparer<T>.Default;
			if (comp.Compare(val, min) <= 0) return min;
			else if (comp.Compare(val, max) >= 0) return max;
			else return val;
		}

		public static int AddImageFileToImageList(string path, System.Windows.Forms.ImageList imgList)
		{
			if (path != null)
			{
				System.IO.FileInfo fileInfo = new System.IO.FileInfo(path);
				if (fileInfo.Exists)
				{
					System.Drawing.Image img = System.Drawing.Image.FromFile(path);
					imgList.Images.Add(img);
					return imgList.Images.Count - 1;
				}
			}

			return -1;
		}

		public static bool IsRunningOnMono ()
		{
			return Type.GetType ("Mono.Runtime") != null;
		}
	}

	public class CSettings
	{
		public enum EMode
		{
			eM_Full = 0,			// Normal mode (connected to target)
			eM_CommandsOnly,	// Connected to target but not reading the log
			eM_FileOnly,			// Parse a file (not connected to target)
		}

		public bool DebugMidi = false;
		public EMode Mode = EMode.eM_Full;
	}
}

#if __MonoCS__

/**
 * As of 2014/11/28, Mono lacks an implementation of System.Windows.Forms.DataVisualization.Charting.Chart.
 * This code functions as a proxy to work around that.
 */
namespace System.Windows.Forms.DataVisualization.Charting
{
	/**
	 * Various System.Windows.Forms.DataVisualization.Charting.*Collection member methods throw NotImplemented on use.
	 * JunkCollection provides the same method interface with implicit conversion operators to avoid complicating code.
	 */
	public class JunkCollection<T> : List<T>
	{
		public static implicit operator ChartAreaCollection(JunkCollection<T> list)
		{
			return new ChartAreaCollection ();
		}
		public static implicit operator LegendCollection(JunkCollection<T> list)
		{
			return new LegendCollection ();
		}
		public static implicit operator SeriesCollection(JunkCollection<T> list)
		{
			return new SeriesCollection ();
		}

		static T s_value;

		public T this[string key]
		{
			get { return s_value; }
			set { }
		}

		public void Add(string ignore)
		{
		}
	}

	public class Chart : System.Windows.Forms.Control, System.ComponentModel.ISupportInitialize
	{
		public ChartDashStyle BorderlineDashStyle;
		public JunkCollection< ChartArea > ChartAreas = new JunkCollection< ChartArea > ();
		public JunkCollection< Legend > Legends = new JunkCollection< Legend > ();
		public JunkCollection< Series > Series = new JunkCollection< Series > ();

		#region System.ComponentModel.ISupportInitialize
		public void BeginInit()
		{
		}

		public void EndInit()
		{
		}
		#endregion
	}
}

#endif
