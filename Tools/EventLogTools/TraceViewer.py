#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

from EventLogger.Reader import Reader
from EventLogger.Utils import EventNameHash, PrologId

from tkinter import filedialog
from tkinter import ttk
from tkinter import *

import tkinter
import struct

AssertId = EventNameHash("Assert")
ErrorId = EventNameHash("Error")
MessageId = EventNameHash("Message")
PrintfId = EventNameHash("Printf")
WarningId = EventNameHash("Warning")


class TraceViewer(object):

    def __init__(self):
        self._tab_content = {}

        self._window = window = tkinter.Tk()
        window.title('Trace Viewer')
        window.minsize(640, 480)

        # file menu
        self._file_menu = file_menu = Menubutton(window, text='File')
        file_menu.menu = Menu(file_menu, tearoff=0)
        file_menu['menu'] = file_menu.menu
        file_menu.pack(side=TOP, anchor=W)

        file_menu.menu.add_command(label='Open...', command=self._open_file)
        file_menu.menu.add_command(label='Exit', command=window.quit)

        # main content
        self._main_view = main_view = ttk.Notebook(window)
        main_view.pack(fill=BOTH, expand=1)

        # setup a debug tab for internal logging
        self._add_new_tab('Debug')

    def run(self):
        self._window.mainloop()

    def _reset(self):
        for tab in self._main_view.winfo_children():
            tab.destroy()
        self._add_new_tab('Debug')

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
        debug_content = self._tab_content['Debug']
        debug_content.insert(END, f'{message}\n')

    def _append_current_message(self, message):
        if self._active_content:
            self._active_content.insert(END, f'{message}\n')

    def _append_current_tagged_message(self, message, tag):
        if self._active_content:
            start = self._active_content.index('insert linestart')
            self._active_content.insert(END, f'{message}\n')
            self._active_content.tag_add(tag, start, 'insert lineend')

    def _open_file(self):
        log_file_types = [
            ('log files','*.azel'),
            ('log files','*.bin'),
            ('all files','*.*')
        ]

        log_file = filedialog.askopenfilename(initialdir='../../', title='Select log file', filetypes=log_file_types)
        if not log_file:
            self._append_debug_message('Invalid file')
            return

        self._reset()

        log_reader = Reader()
        status = log_reader.read_log_file(log_file)
        if status == Reader.ReadStatus_InsufficientFileSize:
            self._append_debug_message('File size too small to contain Event Logger information')
            return
        elif status == Reader.ReadStatus_InvalidFormat:
            self._append_debug_message('Invalid Event Logger format detected')
            return

        log_header = log_reader.get_log_header()
        self._append_debug_message(f'Log File: {log_file}')
        self._append_debug_message(f'Format: {log_header.get_format()}')
        self._append_debug_message(f'Version: {log_header.get_version()}')

        tagged_messages = { AssertId : 'Assert', ErrorId : 'Error', WarningId : 'Warning' }

        has_event = (status == Reader.ReadStatus_Success)
        while has_event:
            event_id = log_reader.get_event_name()

            if event_id == PrologId:
                tab_name = f'Thread {log_reader.get_thread_id()}'

                if tab_name in self._tab_content:
                    self._active_content = self._tab_content[tab_name]

                else:
                    self._add_new_tab(tab_name)

            elif event_id in tagged_messages:
                self._append_current_tagged_message(log_reader.get_event_string(), tagged_messages[event_id])

            elif event_id in (PrintfId, MessageId):
                self._append_current_message(log_reader.get_event_string())

            else:
                self._append_current_message(f'Event ID {event_id}, Size {log_reader.get_event_size()}')

            has_event = log_reader.next()

def main():
    trace_viewer = TraceViewer()
    trace_viewer.run()

if __name__ == '__main__':
    main()
