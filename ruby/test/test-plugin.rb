

class EventdBindingsRubyTestScript
    def start()
        puts "RUBY START"

    end

    def stop()
        puts "RUBY STOP"
    end

    def control_command(*args)
        puts "RUBY CONTROL COMMAND: " + ( args * ' --- ' )
        return [ EventdPlugin::CommandStatus[:ok], "OK" ]
    end

    def global_parse(key_file)
        puts "RUBY GLOBAL PARSE"
    end

    def event_parse(config_id, key_file)
        puts "RUBY EVENT PARSE"
    end

    def config_reset()
        puts "RUBY CONFIG RESET"
    end

    def event_action(config_id, event)
        puts "RUBY EVENT"
        puts config_id
        puts event.inspect
    end
end

EventdPlugin.register_plugin(EventdBindingsRubyTestScript.new)
