#! /usr/bin/env lua

local lgi = require 'lgi'
local GLib = lgi.GLib
local Eventd = lgi.EventdEvent
local Eventc = lgi.Eventc

local host = 'localhost'
local wait = false
local data = {}
local category = nil
local name = nil

while ( #arg > 0 ) do
    local a = arg[1]
    table.remove(arg, 1)
    if ( ( a == '-h' ) or ( a == '--host' ) ) then
        host = arg[1]
        table.remove(arg, 1)
    elseif ( ( a == '-w' ) or ( a == '--wait' ) ) then
        wait = true
    elseif ( ( a == '-d' ) or ( a == '--data' ) ) then
        data[arg[1]] = arg[2]
        table.remove(arg, 1)
        table.remove(arg, 1)
    elseif ( category == nil ) then
        category = a
    elseif ( name == nil ) then
        name = a
    else
        usage()
    end
end

local eventc = Eventc.Connection.new(host)
eventc:set_passive(not wait)

local event = Eventd.Event.new(category, name)

if ( #data > 0 ) then
    for k, v in data do
        event:add_data(k, v)
    end
end

local loop = GLib.MainLoop.new(nil, false)

function disconnect(data1, data2)
    print("disconnect")
    eventc:close()
    loop:quit()
end

function connect(data)
    if wait then
        event:connect('ended', disconnect)
    end

    eventc:connect_sync()
    eventc:event(event)

    if not wait then
        disconnect(nil)
    end

    return false
end

GLib.idle_add(GLib.PRIORITY_DEFAULT_IDLE, connect, nil)

loop:run()
