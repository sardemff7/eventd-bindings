
class EventdBindingsPythonTestScript:
    def start(self):
        print("PYTHON START");

    def stop(self):
        print("PYTHON STOP");

    def globali_parse(self, key_file):
        if ( key_file.has_group("Server") ):
            print("PYTHON HAS GROUP");

    def action_parse(self, key_file):
        return 1;

    def event_action(self, action, event):
        print("PYTHON EVENT ACTION");
        print(action);
        print(event.get_data("test"));

EventdPlugin.register_plugin(EventdBindingsPythonTestScript())

print("PYTHON");
