/*
 * eventd-bindings - Client examples and plugin for eventd bindings
 *
 * Copyright © 2015 Quentin "Sardem FF7" Glidic
 *
 * This file is part of eventd-bindings.
 *
 * eventd-bindings is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * eventd-bindings is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eventd. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>

#include <Python.h>
#include <pygobject.h>

#include <eventd-plugin.h>

#include <helpers.h>

#define EVENTD_BINDINGS_PYTHON_SCRIPTS_SUFFIX "py"
#define EVENTD_BINDINGS_PYTHON_SCRIPTS_DIR EVENTD_PLUGINS_DIR G_DIR_SEPARATOR_S "python"

struct _EventdPluginContext {
    struct {
        PyObject *mGLib;
        PyObject *mEventdEvent;
        PyObject *mEventdPlugin;
    } py;
    const gchar *current_script;
    GHashTable *scripts;
};

static PyObject *
_eventd_bindings_python_register_plugin(PyObject *module, PyObject *args)
{
    PyObject *context = PyObject_GetAttrString(module, "context");
    EventdPluginContext *self = PyCapsule_GetPointer(context, "EventdPlugin.context");

    PyObject *script;
    PyArg_ParseTuple(args, "O", &script);
    Py_INCREF(script);
    g_hash_table_insert(self->scripts, g_strdup(self->current_script), script);

    return Py_None;
}

static void
_eventd_bindings_python_load_script(EventdPluginContext *self, const gchar *name, const gchar *script)
{
    g_debug("Try to load %s", script);
    FILE *f;
    f = fopen(script, "r");
    //PyRun_FileEx(f, script, Py_file_input, NULL, NULL, 1);
    self->current_script = name;
    PyRun_AnyFileEx(f, script, 1);
    self->current_script = NULL;
}

static void
_eventd_bindings_python_free_script(gpointer data)
{
    Py_XDECREF(data);
}

static void
_eventd_bindings_python_free_context(PyObject *context)
{
    Py_XDECREF(context);
}

static void _eventd_bindings_python_uninit(EventdPluginContext *self);
static EventdPluginContext *
_eventd_bindings_python_init(EventdPluginCoreContext *core, EventdPluginCoreInterface *interface)
{
#ifdef PYTHON_NEEDS_GLOBAL_LOADING
    /* Some ugly workaround for python modules that do not link to it */
    static GModule *libpython_module = NULL;
    if ( libpython_module == NULL )
    {
        libpython_module = g_module_open(PYTHON_LIBNAME, G_MODULE_BIND_LAZY);
        if ( libpython_module == NULL )
        {
            g_warning("Couldn't load libpython");
            return NULL;
        }
        g_module_make_resident(libpython_module);
    }
#endif /* PYTHON_NEEDS_GLOBAL_LOADING */

    //Py_SetProgramName(PACKAGE_NAME);
    Py_Initialize();

    EventdPluginContext *self;
    self = g_new0(EventdPluginContext, 1);

    PyObject *__main__;
    __main__ = PyImport_ImportModule("__main__");

    pygobject_init(-1, -1, -1);

    self->py.mGLib = PyImport_ImportModule("gi.repository.GLib");
    self->py.mEventdEvent = PyImport_ImportModule("gi.repository.EventdEvent");
    self->py.mEventdPlugin = PyImport_ImportModule("gi.repository.EventdPlugin");
    PyObject_SetAttrString(__main__, "GLib", self->py.mGLib);
    PyObject_SetAttrString(__main__, "EventdEvent", self->py.mEventdEvent);
    PyObject_SetAttrString(__main__, "EventdPlugin", self->py.mEventdPlugin);

    PyObject *context;
    context = PyCapsule_New(self, "EventdPlugin.context", _eventd_bindings_python_free_context);
    PyObject_SetAttrString(self->py.mEventdPlugin, "context", context);

    PyMethodDef func_def = {
        "register_plugin",
        _eventd_bindings_python_register_plugin,
        METH_VARARGS,
        "Register a script in the " PACKAGE_NAME " python plugin."
    };
    PyObject *func;
    func = PyCFunction_NewEx(&func_def, self->py.mEventdPlugin, self->py.mEventdPlugin);
    PyObject_SetAttrString(self->py.mEventdPlugin, "register_plugin", func);

    self->scripts = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, _eventd_bindings_python_free_script);

    const gchar *env_dir;
    env_dir = g_getenv("EVENTD_BINDINGS_PYTHON_SCRIPTS_DIR");
    if ( env_dir != NULL )
        eventd_bindings_load_directory(self, env_dir, EVENTD_BINDINGS_PYTHON_SCRIPTS_SUFFIX, _eventd_bindings_python_load_script);
    eventd_bindings_load_directory(self, EVENTD_BINDINGS_PYTHON_SCRIPTS_DIR, EVENTD_BINDINGS_PYTHON_SCRIPTS_SUFFIX, _eventd_bindings_python_load_script);

    if ( g_hash_table_size(self->scripts) < 1 )
    {
        g_warning("No scripts found, aborting");
        _eventd_bindings_python_uninit(self);
        return NULL;
    }

    return self;
}

static void
_eventd_bindings_python_uninit(EventdPluginContext *self)
{
    Py_XDECREF(self->py.mEventdPlugin);
    Py_XDECREF(self->py.mEventdEvent);
    Py_XDECREF(self->py.mGLib);

    Py_Finalize();

    g_hash_table_unref(self->scripts);

    g_free(self);
}

static GOptionGroup *
_eventd_bindings_python_get_option_group(EventdPluginContext *self)
{
    return NULL;
}

#define foreach_script \
    GHashTableIter iter; \
    gchar *name; \
    PyObject *script; \
    g_hash_table_iter_init(&iter, self->scripts); \
    while ( g_hash_table_iter_next(&iter, (gpointer *) &name, (gpointer *) &script) )

static void
_eventd_bindings_python_start(EventdPluginContext *self)
{
    foreach_script
        PyObject_CallMethod(script, "start", NULL);
}

static void
_eventd_bindings_python_stop(EventdPluginContext *self)
{
    foreach_script
        PyObject_CallMethod(script, "stop", NULL);
}

static EventdPluginCommandStatus
_eventd_bindings_python_control_command(EventdPluginContext *self, guint64 argc, const gchar * const *argv, gchar **status)
{
    return EVENTD_PLUGIN_COMMAND_STATUS_OK;
}

static void
_eventd_bindings_python_global_parse(EventdPluginContext *self, GKeyFile *key_file)
{
    PyObject *key_file_ = pyg_pointer_new(g_key_file_get_type(), key_file);
    foreach_script
        PyObject_CallMethod(script, "global_parse", "O", key_file_);
}

static void
_eventd_bindings_python_event_parse(EventdPluginContext *self, const gchar *config_id, GKeyFile *key_file)
{
    PyObject *key_file_ = pyg_pointer_new(g_key_file_get_type(), key_file);
    foreach_script
        PyObject_CallMethod(script, "event_parse", "sO", config_id, key_file_);
}

static void
_eventd_bindings_python_config_reset(EventdPluginContext *self)
{
    foreach_script
        PyObject_CallMethod(script, "config_reset", NULL);
}

static void
_eventd_bindings_python_event_action(EventdPluginContext *self, const gchar *config_id, EventdEvent *event)
{
    PyObject *event_ = pygobject_new(G_OBJECT(event));
    foreach_script
        PyObject_CallMethod(script, "event_action", "sO", config_id, event_);
}

EVENTD_BINDINGS_EXPORT const gchar *eventd_plugin_id = "eventd-bindings-python";
EVENTD_BINDINGS_EXPORT
void
eventd_plugin_get_interface(EventdPluginInterface *interface)
{
    eventd_plugin_interface_add_init_callback(interface, _eventd_bindings_python_init);
    eventd_plugin_interface_add_uninit_callback(interface, _eventd_bindings_python_uninit);

    eventd_plugin_interface_add_get_option_group_callback(interface, _eventd_bindings_python_get_option_group);

    eventd_plugin_interface_add_start_callback(interface, _eventd_bindings_python_start);
    eventd_plugin_interface_add_stop_callback(interface, _eventd_bindings_python_stop);

    eventd_plugin_interface_add_control_command_callback(interface, _eventd_bindings_python_control_command);

    eventd_plugin_interface_add_global_parse_callback(interface, _eventd_bindings_python_global_parse);
    eventd_plugin_interface_add_event_parse_callback(interface, _eventd_bindings_python_event_parse);
    eventd_plugin_interface_add_config_reset_callback(interface, _eventd_bindings_python_config_reset);

    eventd_plugin_interface_add_event_action_callback(interface, _eventd_bindings_python_event_action);
}
