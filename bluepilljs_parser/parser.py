#!/usr/bin/python3

import os
import re
import sys
import math
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
    values = {}
    def __init__(self, title):
        wx.Frame.__init__(self, None, title=title, size=(800,600))
        #self.Bind(wx.EVT_CLOSE, self.OnClose)

        self.timer = wx.Timer(self)
        self.Bind(wx.EVT_TIMER, self.poll, self.timer)

        panel = wx.Panel(self)
        box = wx.BoxSizer(wx.VERTICAL)

        self.rows = {}
        #for key in [ 'E22A0', 'E22A1', 'L303C', 'L303D', 'ML393' ]:
        for key in [ 'E22A0', 'L303D', 'ML393' ]:
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
            try:
                self.fd = os.fdopen(os.open(args.file, os.O_NONBLOCK))
            except IOError as e:
                if e.errno != errno.EBUSY:
                    logger.debug(e)
                return
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
            start, end = self.buf.split(MSG_START, 1)
            if len(end) > 0:
                self.buf = end
                self.parse(start)

        self.parse(entries[-1])


    def parse(self, data):
        if len(data) == 0: return
        try:
            fields = data.split()
            label = fields[0]
        except:
            logger.error("Error splitting input line %s", data)
            return

        if not label in self.rows: return
        if not label in self.values: self.values[label] = {}
        if label == 'E22A0':
            if len(data) != 10:
                logger.info("Incorrect length data (%d) from %s: '%s'", len(data), label, data)
                return

            angle = (float(fields[1]) - 512) * math.pi / 512
            x = math.sin(angle) * 2**16
            y = math.cos(angle) * 2**16
            z = 0
        else:
            if not re.match('\w{5} (-\d{4}|-?\d{5}) (-\d{4}|-?\d{5}) (-\d{4}|-?\d{5})', data):
                logger.info("Incorrect length data (%d) from %s: '%s'", len(data), label, data)
                return

            x = float(fields[1])
            y = float(fields[2])
            z = float(fields[3])
            if label != 'ML393':
                x *= 2**6
                y *= 2**6
            angle = math.atan2(x, y)
            if z != 0:
                logger.warn("%s Non-zero Z value %d", label, z)

        self.values[label]['angle'] = angle
        self.values[label]['x'] = x
        self.values[label]['y'] = y

        self.rows[label].SetLabel("{0}: {1: 3.2f} {2: 5.2f} {3: 5.2f}".format(label, math.degrees(angle), x, y))

        tmp = ""
        for label in self.rows:
            if self.rows[label].GetLabel() == "Waiting for data": return
            tmp += self.rows[label].GetLabel() + ' '
        log.write(tmp + "\n")



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
    global log
    log = open("parser.log", "w")
    #app = wx.App(redirect=True)
    #top = Frame("Hello World")
    #top.Show()
    app = MainApp(0)
    app.MainLoop()
