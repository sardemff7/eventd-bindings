
class EventdBindingsPythonTestScript:
    def start(self):
        print("PYTHON START");
    def stop(self):
        print("PYTHON STOP");
    def global_parse(self, key_file):
        if ( key_file.has_group("Server") ):
            print("PYTHON HAS GROUP");
    def event_action(self, config_id, event):
        print("PYTHON EVENT ACTION");
        print(config_id);
        print(eventd.get_data("test"));

EventdPlugin.register_plugin(EventdBindingsPythonTestScript())

print("PYTHON");
