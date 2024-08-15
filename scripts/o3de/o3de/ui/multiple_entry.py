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
    Dialog to handle the selection of names
    """

    def __init__(self, parent, input_value):

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

        self.input_value = input_value

        if input_value:
            items = [ti.strip() for ti in input_value.split(";")]
            sanitized_items = []
            for item in items:
                if item is not None and len(item.strip())>0:
                    sanitized_items.append(item)

            text_entries = "\n".join(sanitized_items)
        else:
            text_entries = ""

        self._entry = tk.Text(self._main_frame, )
        self._entry.insert(tk.END,text_entries)
        self._entry.grid(sticky=tk.NSEW)

        button_frame = tk.Frame(self._main_frame, borderwidth=0)
        button_frame.columnconfigure(0, weight=0)
        button_frame.rowconfigure(0, weight=0)
        button_frame.rowconfigure(1, weight=0)
        button_frame.grid()

        button_add = tk.Button(button_frame, text="Ok", width=4, command=self._on_ok)
        button_add.grid(row=0, column=0, sticky=tk.E)

        button_remove = tk.Button(button_frame, text="Cancel", width=4, command=self._on_cancel)
        button_remove.grid(row=0, column=1, sticky=tk.E)

        root.grid()

    def _on_ok(self):
        result_string = self._entry.get("1.0", tk.END)
        result_items = [rs.strip() for rs in result_string.split("\n")]
        sanitized_items = set()
        for result_item in result_items:
            if result_item is not None and len(result_item.strip()) > 0:
                sanitized_items.add(result_item)
        self.input_value = ';'.join(sanitized_items)
        self.root.destroy()

    def _on_cancel(self):
        self.root.destroy()

    def get_result(self):
        self.root.grab_set()
        self.root.wait_window()
        return self.input_value
