#!/usr/bin/python3

import os
import sys
import time
import errno
import logging
import argparse
from threading import *

import wx

MSG_START = '>'
MSG_END = "\n"
logger = logging.getLogger(__name__)

def parseCmdline():
    parser = argparse.ArgumentParser(description='''
        TODO: insert description.'''
    )
    parser.add_argument('-v', '--verbose', action='store_true', help="Enable verbose output")
    parser.add_argument('-q', '--quiet', action='store_true', help="Output errors only")

    parser.add_argument('file', nargs='?', default='/dev/ttyACM0', help="File to open")

    args = parser.parse_args()

    if args.verbose: loglevel = logging.DEBUG
    elif args.quiet: loglevel = logging.ERROR
    else:            loglevel = logging.INFO

    logging.basicConfig(level=loglevel, format='%(asctime)s %(levelname)s %(message)s')

    return args


class Frame(wx.Frame):
    buf = ""
    def __init__(self, title):
        wx.Frame.__init__(self, None, title=title, size=(800,600))
        #self.Bind(wx.EVT_CLOSE, self.OnClose)

        self.timer = wx.Timer(self)
        self.Bind(wx.EVT_TIMER, self.poll, self.timer)

        panel = wx.Panel(self)
        box = wx.BoxSizer(wx.VERTICAL)

        self.rows = {}
        for key in [ 'E22A0', 'E22A1', 'L303C', 'L303D', 'ML393' ]:
            m_text = wx.StaticText(panel, -1, "Waiting for data")
            m_text.SetFont(wx.Font(14, wx.SWISS, wx.NORMAL, wx.BOLD))
            m_text.SetSize(m_text.GetBestSize())
            box.Add(m_text, 0, wx.ALL, 15)

            self.rows[key] = m_text

        m_close = wx.Button(panel, wx.ID_CLOSE, "Close")
        m_close.Bind(wx.EVT_BUTTON, self.OnClose)
        box.Add(m_close, 0, wx.ALL, 15)

        panel.SetSizer(box)
        panel.Layout()

        self.timer.Start(1)


    def poll(self, event):
        if not hasattr(self, 'fd'):
            self.fd = os.fdopen(os.open(args.file, os.O_NONBLOCK))
        for i in range(10):
            try:
                data = self.fd.readline()
            except IOError as e:
                if e.errno != errno.EAGAIN:
                    logger.debug(e.message)
                return
            if not len(data.strip()): return
            self.split(data)


    def split(self, data):
        entries = data.strip().split(MSG_START)
        for e in entries[:-1]:
            if len(e) == 0: continue
            self.buf += MSG_START + e
            logger.debug(self.buf)
            start, end = self.buf.split(MSG_START, 1)
            if len(end) > 0:
                self.buf = end
                self.parse(start)

        self.parse(entries[-1])


    def parse(self, data):
        try:
            label = data.split()[0]
        except:
            logger.error("Error splitting input line %s", data)
            return

        if not label in self.rows: return
        if "device 1" in data: return

        self.rows[label].SetLabel(data)


    def OnClose(self, event):
        dlg = wx.MessageDialog(self,
            "Do you really want to close this application?",
            "Confirm Exit", wx.OK|wx.CANCEL|wx.ICON_QUESTION)
        result = dlg.ShowModal()
        dlg.Destroy()
        if result != wx.ID_OK: return
        self.Destroy()


class MainApp(wx.App):
    def OnInit(self):
        logger.debug("MainApp init")

        top = Frame("Hello World")
        top.Show()

        return True


if __name__ == '__main__':
    global args
    args = parseCmdline()

    #app = wx.App(redirect=True)
    #top = Frame("Hello World")
    #top.Show()
    app = MainApp(0)
    app.MainLoop()
