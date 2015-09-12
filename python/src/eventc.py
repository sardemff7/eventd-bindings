#! /usr/bin/env python

import argparse

from gi.repository import GLib
from gi.repository import Eventc
from gi.repository import EventdEvent as Eventd

parser = argparse.ArgumentParser(description='Basic Python CLI client for eventd')

parser.add_argument('-H', '--host', default='localhost', help='Host to connect to')
parser.add_argument('-w', '--wait', action='store_true', help='Wait the end of the event')
parser.add_argument('-d', '--data', nargs=2, action='append', help='Event data (name, content) to send')
parser.add_argument('category', help='Event category to send')
parser.add_argument('name', help='Event name to send')
args = parser.parse_args()

eventc = Eventc.Connection.new(args.host)
eventc.set_passive(not args.wait)

event = Eventd.Event.new(args.category, args.name)

for d in args.data:
    event.add_data(d[0], d[1])

loop = GLib.MainLoop.new(None, False)

def disconnect(data1 = None, data2 = None):
    print "disconnect"
    eventc.close()
    loop.quit()

def connect(data):
    if args.wait:
        event.connect('ended', disconnect)

    eventc.connect_sync()
    eventc.event(event)

    if not args.wait:
        disconnect(None)

    return False


GLib.idle_add(connect, None)

loop.run()
