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

#include <glib.h>
#include <glib-object.h>

#include <ruby.h>

#include <eventd-plugin.h>

#include <helpers.h>

#define EVENTD_BINDINGS_RUBY_SCRIPTS_SUFFIX "rb"
#define EVENTD_BINDINGS_RUBY_SCRIPTS_DIR EVENTD_PLUGINS_DIR G_DIR_SEPARATOR_S "ruby"

struct _EventdPluginContext {
    struct {
        VALUE mFFI;
        VALUE mGirFFI;
        VALUE mGLib;
        VALUE mEventdEvent;
        VALUE mEventdPlugin;

        VALUE cPointer;
        VALUE cKeyFile;
        VALUE cEvent;

        struct {
            ID new;
            ID wrap;

            ID start;
            ID stop;

            ID control_command;

            ID global_parse;
            ID event_parse;
            ID config_reset;

            ID event_action;
        } id;
    } rb;
    const gchar *current_script;
    GHashTable *scripts;
};

static inline EventdPluginContext *
_eventd_bindings_ruby_get_context(void)
{
    VALUE self_;
    self_ = rb_const_get_at(rb_cObject, rb_intern("EVENTD_BINDINGS_RUBY_CONTEXT"));
    return (EventdPluginContext *) NUM2ULL(self_);
}

static VALUE
_eventd_bindings_ruby_register_plugin(VALUE ruby_self, VALUE klass)
{
    EventdPluginContext *self = _eventd_bindings_ruby_get_context();

    VALUE *data;
    data = g_slice_new(VALUE);
    *data = klass;

    const gchar *p = g_utf8_strrchr(self->current_script, -1, '.');
    g_hash_table_insert(self->scripts, g_strndup(self->current_script, p - self->current_script), data);

    return Qnil;
}

static void
_eventd_bindings_ruby_load_script(EventdPluginContext *self, const gchar *name, const gchar *script)
{
    VALUE script_ = rb_str_new_cstr(script);
    int state;

    self->current_script = name;
    rb_load_protect(script_, 0, &state);
    self->current_script = NULL;

    if ( state != 0 )
    {
        /* got exception */
        VALUE exception = rb_errinfo(); /* get last exception */
        VALUE s = rb_funcall(exception, rb_intern("to_s"), 0);
        g_warning("Couldn't load script '%s': %s", script, StringValueCStr(s));
        rb_set_errinfo(Qnil);
    }
}


static VALUE
_eventd_bindings_ruby_load_stuff(VALUE dummy)
{
    EventdPluginContext *self = _eventd_bindings_ruby_get_context();

    ruby_init_loadpath();

    VALUE require = rb_intern("require");
    VALUE setup = rb_intern("setup");
    VALUE gem;

    gem = rb_funcall(rb_mKernel, require, 1, rb_str_new_cstr("rubygems"));
    rb_funcall(gem, require, 1, rb_str_new_cstr("ffi"));
    rb_funcall(gem, require, 1, rb_str_new_cstr("gir_ffi"));

    self->rb.mFFI = rb_const_get_at(rb_cObject, rb_intern("FFI"));
    self->rb.mGirFFI = rb_const_get_at(rb_cObject, rb_intern("GirFFI"));
    self->rb.mGLib = rb_const_get_at(rb_cObject, rb_intern("GLib"));
    self->rb.mEventdEvent = rb_funcall(self->rb.mGirFFI, setup, 1, rb_str_new_cstr("EventdEvent"));
    self->rb.mEventdPlugin = rb_funcall(self->rb.mGirFFI, setup, 1, rb_str_new_cstr("EventdPlugin"));

    self->rb.cPointer = rb_const_get_at(self->rb.mFFI, rb_intern("Pointer"));
    self->rb.cKeyFile = rb_const_get_at(self->rb.mGLib, rb_intern("KeyFile"));
    self->rb.cEvent = rb_const_get_at(self->rb.mEventdEvent, rb_intern("Event"));

#define set_id(n) self->rb.id.n = rb_intern(#n)
    set_id(new);
    set_id(wrap);

    set_id(start);
    set_id(stop);

    set_id(control_command);

    set_id(global_parse);
    set_id(event_parse);
    set_id(config_reset);

    set_id(event_action);
#undef set_id

    return Qnil;
}

#define _eventd_bindings_ruby_get_object(klass, data) _eventd_bindings_ruby_get_object_(self, self->rb.c##klass, data)

static inline VALUE
_eventd_bindings_ruby_get_object_(EventdPluginContext *self, VALUE klass, gpointer data)
{
    VALUE pointer = rb_funcall(self->rb.cPointer, self->rb.id.new, 1, ULL2NUM((guint64) data));
    return rb_funcall(klass, self->rb.id.wrap, 1, pointer);
}

static void
_eventd_bindings_ruby_free_script(gpointer data)
{
    g_slice_free(VALUE, data);
}

static void _eventd_bindings_ruby_uninit(EventdPluginContext *self);
static EventdPluginContext *
_eventd_bindings_ruby_init(EventdPluginCoreContext *core, EventdPluginCoreInterface *interface)
{
    int state = ruby_setup();
    if ( state != 0 )
        return NULL;

    ruby_script(PACKAGE_NAME);

    EventdPluginContext *self;
    self = g_new0(EventdPluginContext, 1);

    self->scripts = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, _eventd_bindings_ruby_free_script);

    rb_define_global_const("EVENTD_BINDINGS_RUBY_CONTEXT", ULL2NUM((guint64) self));

    rb_protect(_eventd_bindings_ruby_load_stuff, Qnil, &state);
    if ( state != 0 )
    {
        VALUE exception = rb_errinfo(); /* get last exception */
        VALUE s = rb_funcall(exception, rb_intern("to_s"), 0);
        g_warning("Couldn't load sstuff: %.*s", (int)RSTRING_LEN(s), StringValuePtr(s));
        rb_set_errinfo(Qnil);
        _eventd_bindings_ruby_uninit(self);
        return NULL;
    }

    rb_define_module_function(self->rb.mEventdPlugin, "register_plugin", _eventd_bindings_ruby_register_plugin, 1);


    const gchar *env_dir;
    env_dir = g_getenv("EVENTD_BINDINGS_RUBY_SCRIPTS_DIR");
    if ( env_dir != NULL )
        eventd_bindings_load_directory(self, env_dir, EVENTD_BINDINGS_RUBY_SCRIPTS_SUFFIX, _eventd_bindings_ruby_load_script);
    eventd_bindings_load_directory(self, EVENTD_BINDINGS_RUBY_SCRIPTS_DIR, EVENTD_BINDINGS_RUBY_SCRIPTS_SUFFIX, _eventd_bindings_ruby_load_script);

    if ( g_hash_table_size(self->scripts) < 1 )
    {
        g_warning("No scripts found, aborting");
        _eventd_bindings_ruby_uninit(self);
        return NULL;
    }

    return self;
}

static void
_eventd_bindings_ruby_uninit(EventdPluginContext *self)
{
    ruby_cleanup(0);

    g_hash_table_unref(self->scripts);

    g_free(self);
}

#define foreach_script \
    GHashTableIter iter; \
    gchar *name; \
    VALUE *script; \
    g_hash_table_iter_init(&iter, self->scripts); \
    while ( g_hash_table_iter_next(&iter, (gpointer *) &name, (gpointer *) &script) )

static void
_eventd_bindings_ruby_start(EventdPluginContext *self)
{
    foreach_script
        rb_funcall(*script, self->rb.id.start, 0);
}

static void
_eventd_bindings_ruby_stop(EventdPluginContext *self)
{
    foreach_script
        rb_funcall(*script, self->rb.id.stop, 0);

}

static EventdPluginCommandStatus
_eventd_bindings_ruby_control_command(EventdPluginContext *self, guint64 argc, const gchar * const *argv, gchar **status)
{
    VALUE *script;
    script = g_hash_table_lookup(self->scripts, argv[0]);
    if ( script == NULL )
    {
        *status = g_strdup_printf("No script '%s' found", argv[0]);
        return EVENTD_PLUGIN_COMMAND_STATUS_COMMAND_ERROR;
    }

    VALUE *args;
    args = g_new(VALUE, argc - 1);
    guint64 i;
    for ( i = 1 ; i < argc ; ++i )
        args[i - 1] = rb_str_new_cstr(argv[i]);

    VALUE r_;
    VALUE status_ = Qnil;

    r_ = rb_funcallv(*script, self->rb.id.control_command, argc - 1, args);

    if ( rb_type_p(r_, T_ARRAY) )
    {
        status_ = rb_ary_entry(r_, 1);
        r_ = rb_ary_entry(r_, 0);
    }

    if ( ! ( FIXNUM_P(r_) && ( ( NIL_P(status_) || RB_TYPE_P(status_, T_STRING) ) ) ) )
        goto error;

    EventdPluginCommandStatus r;
    GEnumValue *enum_value;
    r = NUM2ULL(r_);
    enum_value = g_enum_get_value(g_type_class_ref(EVENTD_PLUGIN_TYPE_COMMAND_STATUS), r);
    if ( enum_value == NULL )
        goto error;

    if ( ( ! NIL_P(status_) ) && ( rb_str_strlen(status_) > 0 ) )
        *status = g_strdup(StringValueCStr(status_));

    return r;

error:
    *status = g_strdup_printf("Wrong return value from script '%s'", argv[0]);
    return EVENTD_PLUGIN_COMMAND_STATUS_EXEC_ERROR;
}

static void
_eventd_bindings_ruby_global_parse(EventdPluginContext *self, GKeyFile *key_file)
{
    VALUE key_file_ = _eventd_bindings_ruby_get_object(KeyFile, key_file);

    foreach_script
        rb_funcall(*script, self->rb.id.global_parse, 1, key_file_);
}

static void
_eventd_bindings_ruby_event_parse(EventdPluginContext *self, const gchar *config_id, GKeyFile *key_file)
{
    VALUE config_id_ = rb_str_new_cstr(config_id);
    VALUE key_file_ = _eventd_bindings_ruby_get_object(KeyFile, key_file);

    foreach_script
        rb_funcall(*script, self->rb.id.event_parse, 2, config_id_, key_file_);
}

static void
_eventd_bindings_ruby_config_reset(EventdPluginContext *self)
{
    foreach_script
        rb_funcall(*script, self->rb.id.config_reset, 0);
}

static void
_eventd_bindings_ruby_event_action(EventdPluginContext *self, const gchar *config_id, EventdEvent *event)
{
    VALUE config_id_ = rb_str_new_cstr(config_id);
    VALUE event_ = _eventd_bindings_ruby_get_object(Event, event);

    foreach_script
        rb_funcall(*script, self->rb.id.event_action, 2, config_id_, event_);
}

EVENTD_BINDINGS_EXPORT const gchar *eventd_plugin_id = "eventd-bindings-ruby";
EVENTD_BINDINGS_EXPORT
void
eventd_plugin_get_interface(EventdPluginInterface *interface)
{
    eventd_plugin_interface_add_init_callback(interface, _eventd_bindings_ruby_init);
    eventd_plugin_interface_add_uninit_callback(interface, _eventd_bindings_ruby_uninit);

    eventd_plugin_interface_add_start_callback(interface, _eventd_bindings_ruby_start);
    eventd_plugin_interface_add_stop_callback(interface, _eventd_bindings_ruby_stop);

    eventd_plugin_interface_add_control_command_callback(interface, _eventd_bindings_ruby_control_command);

    eventd_plugin_interface_add_global_parse_callback(interface, _eventd_bindings_ruby_global_parse);
    eventd_plugin_interface_add_event_parse_callback(interface, _eventd_bindings_ruby_event_parse);
    eventd_plugin_interface_add_config_reset_callback(interface, _eventd_bindings_ruby_config_reset);

    eventd_plugin_interface_add_event_action_callback(interface, _eventd_bindings_ruby_event_action);
}
