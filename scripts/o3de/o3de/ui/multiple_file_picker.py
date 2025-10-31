#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import tkinter as tk
from tkinter import filedialog


class Dialog(object):
    """
    Dialog to handle the selection of multiple files
    """

    def __init__(self, parent, file_filters=None, initial_list=""):

        root = self.root = tk.Toplevel(parent)
        root.title('Configure Files')
        root.resizable(True,True)

        px = parent.winfo_rootx()
        py = parent.winfo_rooty()
        root.geometry(f'400x200+{px}+{py}')

        root.columnconfigure(0, weight=1)
        root.rowconfigure(0, weight=1)

        self._main_frame = tk.Frame(self.root, borderwidth=2, relief=tk.SOLID)
        self._main_frame.columnconfigure(0, weight=1)
        self._main_frame.rowconfigure(0, weight=1)
        self._main_frame.grid(padx=8, pady=8, sticky=tk.NSEW)

        self.file_list_box = tk.Listbox(self._main_frame, selectmode=tk.SINGLE)
        self.file_list_box.grid(sticky=tk.NSEW)

        if initial_list:
            self.items = set([ti.strip() for ti in initial_list.split(";")])
        else:
            self.items = set()

        for item in self.items:
            self.file_list_box.insert(tk.END, item)

        self.file_filters = file_filters

        button_frame = tk.Frame(self._main_frame, borderwidth=0)
        button_frame.columnconfigure(0, weight=0)
        button_frame.rowconfigure(0, weight=0)
        button_frame.rowconfigure(1, weight=0)
        button_frame.grid(row=0, column=1)

        button_add = tk.Button(button_frame, text="+", width=4, command=self._choose_file)
        button_add.grid(sticky=tk.N)

        button_remove = tk.Button(button_frame, text="-", width=4, command=self._remove_file)
        button_remove.grid(sticky=tk.N)

        root.grid()

    def _choose_file(self):
        filename = filedialog.askopenfilename(filetypes=self.file_filters)
        if filename not in self.items:
            self.items.add(filename)
            self.file_list_box.insert(0,filename)

    def _remove_file(self):
        selected_list = self.file_list_box.curselection()
        for selected in selected_list:
            selected_item = self.file_list_box.get(selected)
            self.items.remove(selected_item)
            self.file_list_box.delete(selected)

    def get_result(self):
        self.root.grab_set()
        self.root.wait_window()
        cleaned_items = [item for item in self.items if len(item)>0]
        result_string = ";".join(cleaned_items)
        return result_string
