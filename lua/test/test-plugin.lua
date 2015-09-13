
local EventdBindingsLuaTestScript = {}
EventdBindingsLuaTestScript.__index = EventdBindingsLuaTestScript

function EventdBindingsLuaTestScript.new()
  local self = setmetatable({}, EventdBindingsLuaTestScript)
  return self
end

function EventdBindingsLuaTestScript:start()
    print("LUA START");
end
function EventdBindingsLuaTestScript:stop()
    print("LUA STOP");
end

function EventdBindingsLuaTestScript:global_parse(key_file)
    print("LUA GLOBAL PARSE")
    print(key_file:get_start_group())
    if ( key_file:has_group("Server") ) then
        print("LUA HAS GROUP");
    end
end

function EventdBindingsLuaTestScript:event_action(config_id, event)
    print("LUA EVENT ACTION");
    print(config_id);
    print(event:get_data("test"));
end

setmetatable(EventdBindingsLuaTestScript, { __call = EventdBindingsLuaTestScript.new })

EventdPlugin.register_plugin(EventdBindingsLuaTestScript())

print("LUA");
