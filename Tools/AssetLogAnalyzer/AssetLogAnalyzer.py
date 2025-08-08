#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

from tkinter import filedialog
from tkinter import ttk
from tkinter import *
import tkinter
import pathlib
import time
from datetime import datetime

from os import listdir
from os.path import isfile, join

class Assets(object):
    processing = {}
    processed = {}
    collisions = {}

class AssetStats(object):
    num_assets=0
    num_assets_processing=0
    total_time=0.0
    start_processing_time=0.0

    def __init__(self):
        self.num_assets=0
        self.num_assets_processing=0
        self.total_time=0.0
        self.start_processing_time=0.0

    def Start(self, timestamp):
        self.num_assets = self.num_assets + 1
        if self.num_assets_processing == 0:
            self.start_processing_time = timestamp
        self.num_assets_processing = self.num_assets_processing + 1

    def Stop(self, timestamp):
        if self.num_assets_processing > 0:
            self.num_assets_processing = self.num_assets_processing - 1
            if self.num_assets_processing == 0:
                self.total_time = self.total_time + (timestamp - self.start_processing_time)

    def ForceStop(self, timestamp):
        if self.num_assets_processing > 0:
            self.total_time = self.total_time + (timestamp - self.start_processing_time)

    def AddTime(self, seconds, asset_path):
        # TODO store the longest processing times
        pass
        #self.total_time = self.total_time + seconds

        # detect if still processing assets of this type
        # accumulate time if still processing assets of this type

class PlatformStats(object):
    num_assets=0
    total_time=0.0
    num_assets_processing=0
    start_processing_time=0.0
    types={}

    def __init__(self):
        self.num_assets=0
        self.total_time=0.0
        self.num_assets_processing=0
        self.start_processing_time=0.0
        self.types={}

    def Start(self, asset_type, timestamp):
        self.num_assets = self.num_assets + 1
        if self.num_assets_processing == 0:
            self.start_processing_time = timestamp
        self.num_assets_processing = self.num_assets_processing + 1

        if asset_type not in self.types:
            self.types[asset_type] = AssetStats()
        self.types[asset_type].Start(timestamp)

    def AddTime(self, seconds, asset_type, asset_path):
        if asset_type not in self.types:
            self.types[asset_type] = AssetStats()
        self.types[asset_type].AddTime(seconds, asset_path)

    def Stop(self, timestamp, asset_type=None):
        if self.num_assets_processing > 0:
            self.num_assets_processing = self.num_assets_processing - 1
            if self.num_assets_processing == 0:
                self.total_time = self.total_time + (timestamp - self.start_processing_time)
        
        if asset_type:
            if asset_type not in self.types:
                self.types[asset_type] = AssetStats()
            self.types[asset_type].Stop(timestamp)

    def ForceStop(self, timestamp):
        # TODO store the longest processing asset paths
        if self.num_assets_processing > 0:
            self.total_time = self.total_time + (timestamp - self.start_processing_time)

        for asset_type, asset_stats in self.types.items():
            asset_stats.ForceStop(timestamp)

class Stats(object):
    start = 0
    end=0
    total_time=0
    num_assets=0
    num_assets_processing=0
    start_processing_time=0.0
    platforms={}

    def __init__(self):
        self.start = 0
        self.end = 0
        self.total_time = 0
        self.num_assets = 0
        self.num_assets_processing = 0
        self.start_processing_time = 0.0
        self.platforms={
            'android':PlatformStats(),
            'common':PlatformStats(),
            'ios':PlatformStats(),
            'linux':PlatformStats(),
            'mac':PlatformStats(),
            'pc':PlatformStats(),
            'server':PlatformStats(),
            'all':PlatformStats()
        }

    def Start(self, timestamp):
        self.num_assets = self.num_assets + 1
        if self.num_assets_processing == 0:
            self.start_processing_time = timestamp
        self.num_assets_processing = self.num_assets_processing + 1

    def Stop(self, timestamp):
        if self.num_assets_processing > 0:
            self.num_assets_processing = self.num_assets_processing - 1
            if self.num_assets_processing == 0:
                self.total_time = self.total_time + (timestamp - self.start_processing_time)
    
    def ForceStop(self):
        if self.num_assets_processing > 0:
            self.total_time = self.total_time + (self.end - self.start_processing_time)

class AssetLogAnalyzer(object):

    def __init__(self):
        self._tab_content = {}

        self._window = window = tkinter.Tk()
        window.title('Asset Log Analyzer')
        window.minsize(640, 480)

        self._menubar = menubar = Menu(window)

        # file menu
        self._file_menu = file_menu = Menu(menubar, tearoff=0)
        file_menu.add_command(label='Open...', command=self._open_file)
        file_menu.add_separator()
        file_menu.add_command(label='Exit', command=window.quit)
        menubar.add_cascade(label='File', menu=file_menu)

        self._edit_menu = edit_menu = Menu(menubar, tearoff=0)
        edit_menu.add_command(label='Find', command=self._find)
        menubar.add_cascade(label='Edit', menu=edit_menu)

        window.config(menu=menubar)

        # main content
        self._main_view = main_view = ttk.Notebook(window)
        main_view.pack(fill=BOTH, expand=1)

        # setup a debug tab for internal logging
        self._add_new_tab('Logs')

    def run(self):
        self._window.mainloop()

    def _reset(self):
        for tab in self._main_view.winfo_children():
            tab.destroy()
        self._tab_content = {}
        self._add_new_tab('Logs')

    def _add_new_tab(self, name):
        tab = ttk.Frame(self._main_view)
        vscrollbar = Scrollbar(tab)
        hscrollbar = Scrollbar(tab, orient='horizontal')

        text_box = Text(tab, yscrollcommand=vscrollbar.set, xscrollcommand=hscrollbar.set, wrap=NONE)
        text_box.tag_config('Assert', foreground='red3', background='gray80')
        text_box.tag_config('Error', foreground='red2')
        text_box.tag_config('Warning', foreground='SteelBlue3')

        vscrollbar.config(command=text_box.yview)
        hscrollbar.config(command=text_box.xview)

        vscrollbar.pack(side=RIGHT, fill=Y)
        hscrollbar.pack(side=BOTTOM, fill=X)
        text_box.pack(fill=BOTH, expand=1)

        self._main_view.add(tab, text=name)

        self._tab_content[name] = text_box
        self._active_content = text_box

    def _append_debug_message(self, message):
        debug_content = self._tab_content['Logs']
        debug_content.insert(END, f'{message}\n')

    def _append_message(self, message, tab_content=None):
        tab_content = tab_content or self._active_content
        if tab_content:
            if message.endswith('\n'):
                tab_content.insert(END, f'{message}')
            else:
                tab_content.insert(END, f'{message}\n')

    def _append_category_message(self, category, message):
        tag = None
        if 'ERROR' in message: 
            tag = 'Error'
        elif 'WARNING' in message:
            tag = 'Warning'

        if category not in self._tab_content:
            self._add_new_tab(category)

        if tag:
            self._append_tagged_message(message, 'Error', self._tab_content[category])
        else:
            self._append_message(message, self._tab_content[category])

    def _append_tagged_message(self, message, tag, tab_content=None):
        tab_content = tab_content or self._active_content
        if tab_content:
            start = tab_content.index('insert linestart')
            if message.endswith('\n'):
                tab_content.insert(END, f'{message}')
            else:
                tab_content.insert(END, f'{message}\n')
            tab_content.tag_add(tag, start, 'insert lineend')

    def _find(self):
        if self._main_view.focus_get() == self._active_content:
            self._append_debug_message('Active tab is focused')
        else:
            self._append_debug_message('Debug tab is focused')

    def _open_file(self, log_file_path=None):
        log_file_types = [
            ('log files','*.log'),
            ('all files','*.*')
        ]

        if not log_file_path:
            log_file_path = filedialog.askopenfilename(initialdir='../../', title='Select log file', filetypes=log_file_types)
            if not log_file_path:
                self._append_debug_message('Invalid file or no file selected')
                return

        self._reset()

        self._append_debug_message(f"Opening {log_file_path} and related logs...")
        log_file_path = pathlib.Path(log_file_path)

        # find all related logs
        log_file_stem = log_file_path.name.split('.')[0]

        log_file_names = [f for f in listdir(log_file_path.parent) if isfile(join(log_file_path.parent,f)) and f.split('.')[0] == log_file_stem]
        if log_file_names:
            self._append_debug_message(f"Found related logs:")
            for related_log_path in log_file_names:
                self._append_debug_message(f"   {related_log_path}")
        else:
            self._append_debug_message(f"No related logs found")
            return

        log_file_names.sort()

        self._append_debug_message(f"Analyzing logs...")
        self._window.after(1000, self._analyze_logs, log_file_path, log_file_names)

    def _analyze_logs(self, log_file_path, log_file_names):
        assets = Assets()
        stats = Stats()
        last_descending_error_time = 0

        # load stats from all the logs
        for log_file_name in log_file_names:
            with open(join(log_file_path.parent,log_file_name),"r") as f:
                for line in f:
                    elements = line.split('~~')
                    if len(elements) > 1:
                        _, timestamp, message_type, id, category, message = elements

                        timestamp = float(timestamp) / 1000.0

                        if stats.start == 0:
                            stats.start = timestamp

                        if timestamp > stats.end:
                            stats.end = timestamp
                        elif timestamp < stats.end:
                            if last_descending_error_time != stats.end:
                                last_descending_error_time = stats.end
                                self._append_debug_message(f'{log_file_name} has descending timestamp {timestamp} latest is {stats.end}')

                        if category == 'AssetProcessor':
                            if message.startswith('Processing'):
                                # Example 
                                # ~~1705121798664~~1~~0000000000004AB0~~AssetProcessor~~Processing relative path with spaces.ext (pc)...
                                relative_path = message[message.find(' ')+1:message.rfind(' ')]
                                platform = message[message.rfind('(') + 1:message.rfind(')')]

                                asset_type = relative_path[relative_path.rfind('.'):].lower()
                                relative_path_with_platform = f"{relative_path}_{platform}"
                                if relative_path_with_platform in assets.processing:
                                    # it's possible a file with the same path is found in multiple places and one will overwrite the 
                                    # other in the cache
                                    # without knowing the absolute path we cannot say if it's being re-processed
                                    if relative_path_with_platform in assets.collisions:
                                        assets.collisions[relative_path_with_platform]['num_instances'] = assets.collisions[relative_path_with_platform]['num_instances'] + 1
                                    else:
                                        assets.collisions[relative_path_with_platform] = {
                                            'num_instances':1,
                                            'absolute_paths':[]
                                        }

                                stats.platforms[platform].Start(asset_type, timestamp)
                                stats.Start(timestamp)
                                assets.processing[relative_path_with_platform] = timestamp

                            elif message.startswith('Processed') or message.startswith('Failed'):
                                # Example
                                # ~~1705121800624~~1~~0000000000003864~~AssetProcessor~~Processed "absolute path with spaces.ext" ("pc")... 
                                # ~~1705121800624~~1~~0000000000003864~~AssetProcessor~~Failed absolute path with spaces.ext, ("pc")... 
                                if message.startswith('Processed'):
                                    absolute_path, platform = message.split('"')[1::2]
                                else:
                                    absolute_path = message[message.find(' ')+1:message.rfind(',')]
                                    platform = message[message.rfind('(') + 1:message.rfind(')')]
                                    # TODO record the failed asset

                                asset_type = absolute_path[absolute_path.rfind('.'):].lower()
                                absolute_path_with_platform = f"{absolute_path}_{platform}"

                                stats.platforms[platform].Stop(timestamp, asset_type)
                                stats.Stop(timestamp)

                                found_asset = False

                                for processing_path in assets.processing.keys():
                                    if processing_path in absolute_path_with_platform:
                                        processing_time = timestamp - assets.processing[processing_path]
                                        assets.processed[processing_path] = processing_time
                                        stats.platforms[platform].AddTime(processing_time, asset_type, absolute_path)

                                        del assets.processing[processing_path]
                                        found_asset = True
                                        break

                                found_collision = False
                                for collision_paths in assets.collisions.keys():
                                    if collision_paths in absolute_path_with_platform:
                                        assets.collisions[collision_paths]['absolute_paths'].append(absolute_path_with_platform)
                                        found_collision = True
                                        break

                                if not found_asset and not found_collision:
                                    self._append_debug_message(f"{absolute_path} ({platform}) finished processing but was not found in processing list.")

                        dt = datetime.fromtimestamp(timestamp)
                        message_time = dt.isoformat()
                        message = f"{message_time} {message}"

                        self._append_category_message(category, message) 
                    else:
                        self._append_category_message('Missing Category', message)
        
        # how many assets were processed per platform and how long did each take?
        # for each platform which 5 assets took the longest?
        # for each platform which asset types took the longest
        # which asset types took the longest
        # Overview
        # Platform , # assets , Processing time
        # ------------------------------------
        # pc,    1000,  1h 35m 13s 
        # android,   1000,   2h 35m 13s 
        # ------------------------------------
        # total,   1000,  3h 35m 13s 
        #
        # Platform Overview: pc
        # Asset Type, # assets, Processing time
        # ------------------------------------
        # .fbx, 1000, 2h 35m 13s 
        # .exr, 1000, 2h 35m 13s 
        # ------------------------------------
        # Top 4 longest individual Processing jobs
        # Processing time, Asset
        # ------------------------------------
        # 2h 35m 13s, blah.fbx 

        stats.ForceStop()

        def formatTime(seconds):
            return time.strftime("%Hh %Mm %Ss", time.gmtime(seconds))

        start_time = datetime.fromtimestamp(stats.start)
        end_time = datetime.fromtimestamp(stats.end)
        total_time = end_time - start_time
        self._append_debug_message(f"Done")
        self._append_debug_message("")

        self._append_debug_message(f"Overview")
        self._append_debug_message(f"Log start time {start_time.isoformat()}")
        self._append_debug_message(f"Log end time {end_time.isoformat()}")
        self._append_debug_message(f"Log total time {formatTime(total_time.seconds)}")
        self._append_debug_message("")

        self._append_debug_message(f"Platform, # Assets, Processing Time")
        for platform,platform_stats in stats.platforms.items():
            if platform_stats.num_assets > 0:
                # make sure we stop any final process timers in case the log is partial
                platform_stats.ForceStop(stats.end)
                self._append_debug_message(f"{platform}, {platform_stats.num_assets}, {formatTime(platform_stats.total_time)}")
        self._append_debug_message(f"Total, {stats.num_assets}, {formatTime(stats.total_time)}")

        for platform,platform_stats in stats.platforms.items():
            if platform_stats.num_assets > 0:
                self._append_debug_message("")
                self._append_debug_message(f"Platform Overview: {platform}")
                self._append_debug_message(f"Type, # Assets, Processing Time")

                # sort asset types by longest time
                for asset_type in sorted(platform_stats.types, key=lambda k: platform_stats.types[k].total_time, reverse=True):
                    num_assets = platform_stats.types[asset_type].num_assets
                    total_time = platform_stats.types[asset_type].total_time
                    self._append_debug_message(f"{asset_type}, {num_assets}, {formatTime(total_time)}")

        self._append_debug_message("")
        self._append_debug_message(f"Num processed: {len(assets.processed)}")
        self._append_debug_message(f"Num processing: {len(assets.processing)}")


def main():
    asset_log_analyzer = AssetLogAnalyzer()
    asset_log_analyzer.run()

if __name__ == '__main__':
    main()
