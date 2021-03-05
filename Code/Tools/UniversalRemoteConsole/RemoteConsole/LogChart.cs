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

namespace RemoteConsole
{
	class LogChart
	{
		private System.Windows.Forms.DataVisualization.Charting.Chart chart;

		public LogChart(System.Windows.Forms.DataVisualization.Charting.Chart theChart)
		{
			chart = theChart;
			if (chart != null)
			{
				chart.ChartAreas.Clear();
				chart.ChartAreas.Add("Full");
			}
		}

		public System.Windows.Forms.DataVisualization.Charting.SeriesCollection GetSeries() { return chart.Series; }
		public void ClearSeries() { if (chart!=null) chart.Series.Clear();	}
		public void AddSeries(string seriesName)
		{
			if (chart != null && !Common.IsRunningOnMono())
			{
				chart.Series.Add(seriesName);
				for (var i = 0; i < LogFilterManager.SingleFilterHistory.BufferSize; ++i)
				{
					chart.Series[seriesName].Points.Add(new System.Windows.Forms.DataVisualization.Charting.DataPoint(i, 0));
				}
				chart.Series[seriesName].ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.Line;
				chart.ChartAreas["Full"].AxisY.IntervalAutoMode = System.Windows.Forms.DataVisualization.Charting.IntervalAutoMode.VariableCount;
				chart.ChartAreas["Full"].AxisY.Maximum = 10;
				chart.ChartAreas["Full"].AxisY.Minimum = 0;
				chart.ChartAreas["Full"].AxisX.Minimum = 0;
			}
		}

		public void ClearData()
		{
			if (chart != null)
			{
				foreach (var serie in chart.Series)
				{
					for (var i = 0; i < LogFilterManager.SingleFilterHistory.BufferSize; ++i)
					{
						serie.Points[i].SetValueY(0);
					}
				}
			}
		}

		public void Refresh()
		{
			if (chart != null) chart.Refresh();
		}

		public void SetYValue(string seriesName, int pointIndex, float value)
		{
			if (chart != null && !Common.IsRunningOnMono()) chart.Series[seriesName].Points[pointIndex].SetValueY(value);
		}

		public void ShowHideLegend(string seriesName, bool isChecked)
		{
			if (chart != null)
			{
				if (System.Windows.Forms.Control.ModifierKeys == System.Windows.Forms.Keys.Control)
				{
					// Select All
					foreach (var s in chart.Series)
					{
						s.IsVisibleInLegend = s.Enabled = isChecked;
					}
				}
				else if (System.Windows.Forms.Control.ModifierKeys == System.Windows.Forms.Keys.Shift)
				{
					// Toggle All
					foreach (var s in chart.Series)
					{
						s.IsVisibleInLegend = s.Enabled = !isChecked;
					}
				}

				// Toggle Selected
				var selected = chart.Series[seriesName];
				if (selected != null)
				{
					selected.IsVisibleInLegend = selected.Enabled = isChecked;
				}
			}
		}
	}
}
