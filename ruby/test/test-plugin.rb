

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

    def action_parse(key_file)
        puts "RUBY EVENT PARSE"
        return 1
    end

    def config_reset()
        puts "RUBY CONFIG RESET"
    end

    def event_action(action, event)
        puts "RUBY EVENT"
        puts action.inspect
        puts event.inspect
    end
end

EventdPlugin.register_plugin(EventdBindingsRubyTestScript.new)
