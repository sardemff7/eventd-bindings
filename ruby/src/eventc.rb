#! /usr/bin/env ruby

require 'gir_ffi'

GirFFI.setup :Eventd
GirFFI.setup :Eventc

host = 'localhost'

$wait = false

category = nil
name = nil

data = {}

while ( arg = ARGV.shift )
    case ( arg )
    when '-h', '--host'
        host = ARGV.shift
    when '-w', '--wait'
        $wait = true
    when '-d', '--data'
        data[ARGV.shift] = ARGV.shift
    else
        if ( category.nil? )
            category = arg
        else
            name = arg
        end
    end
end

$eventc = Eventc::Connection.new(host)
$eventc.set_passive(!$wait)

$event = Eventd::Event.new(category, name)
data.each_pair { |n,v| $event.add_data(n, v) }

$loop = GLib::MainLoop.new(nil, false)

Signal.trap("INT") do
  $loop.quit() if ( $loop.is_running() )
end

def connect()
    $event.connect('ended', :disconnect) if ( $wait )
    p "connect"
    $eventc.connect_sync()
    p "connected"
    $eventc.event($event)
    disconnect() unless ( $wait )
    return false
end

def disconnect()
    $eventc.close()
    $loop.quit()
end

GLib::idle_add(GLib::PRIORITY_DEFAULT_IDLE, :connect, nil, nil)

$loop.run()
