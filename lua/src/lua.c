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
#include <gmodule.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <eventd-plugin.h>

#include <helpers.h>

#define LUA_LIBNAME "liblua" LUA_VERSION_MAJOR "." LUA_VERSION_MINOR "." G_MODULE_SUFFIX

#define EVENTD_BINDINGS_LUA_SCRIPTS_SUFFIX "lua"
#define EVENTD_BINDINGS_LUA_SCRIPTS_DIR EVENTD_PLUGINS_DIR G_DIR_SEPARATOR_S "lua"

struct _EventdPluginContext {
    lua_State *lua;
    const gchar *current_script;
    GList *scripts;
    gint current_action;
    GList *actions;
};

struct _EventdPluginAction {
    gint index;
    GList *script_actions;
};

static int
_eventd_bindings_lua_register_plugin(lua_State *L)
{
    int argc = lua_gettop(L);
    EventdPluginContext *self;
    lua_getglobal(L, "EventdPlugin");
    lua_getfield(L, -1, "context");
    self = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if ( argc != 1 )
    {
        lua_pop(L, 2);
        lua_pushliteral(L, "EventdPlugin.register_plugin requires exactly one argument");
        lua_error(L);
    }
    if ( ! lua_istable(L, 1) )
    {
        lua_pop(L, 2);
        lua_pushliteral(L, "EventdPlugin.register_plugin requires a table as argument");
        lua_error(L);
    }

    lua_getfield(L, -1, "scripts");
    lua_remove(L, -2);
    lua_insert(L, 1);
    lua_setfield(L, -2, self->current_script);
    lua_pop(L, 1);

    self->scripts = g_list_prepend(self->scripts, g_strdup(self->current_script));

    return 0;
}

static void
_eventd_bindings_lua_load_script(EventdPluginContext *self, const gchar *name, const gchar *script)
{
    g_debug("Load script %s", name);
    self->current_script = name;
    if ( luaL_dofile(self->lua, script) != LUA_OK )
    {
        g_warning("Couldn't load script '%s': %s", name, lua_tostring(self->lua, -1));
        lua_pop(self->lua, 1);
    }
    self->current_script = NULL;
}

static inline void
_eventd_bindings_lua_require(EventdPluginContext *self, const gchar *name)
{
    lua_getglobal(self->lua, "require");
    lua_pushstring(self->lua, name);
    lua_call(self->lua, 1, 1);
    lua_pushvalue(self->lua, -1);
    lua_setglobal(self->lua, name);
}

static inline void
_eventd_bindings_lua_lgi_require(EventdPluginContext *self, const gchar *name, gboolean keep_around)
{
    lua_getfield(self->lua, -1, "require");
    lua_pushstring(self->lua, name);
    lua_call(self->lua, 1, 1);
    if ( keep_around )
        lua_pushvalue(self->lua, -1);
    lua_setglobal(self->lua, name);
}

static void _eventd_bindings_lua_uninit(EventdPluginContext *self);
static EventdPluginContext *
_eventd_bindings_lua_init(EventdPluginCoreContext *core, EventdPluginCoreInterface *interface)
{
#ifdef LUA_NEEDS_GLOBAL_LOADING
    /* Some ugly workaround for lua modules that do not link to it */
    static GModule *liblua_module = NULL;
    if ( liblua_module == NULL )
    {
        liblua_module = g_module_open(LUA_LIBNAME, G_MODULE_BIND_LAZY);
        if ( liblua_module == NULL )
        {
            g_warning("Couldn't load liblua");
            return NULL;
        }
        g_module_make_resident(liblua_module);
    }
#endif /* LUA_NEEDS_GLOBAL_LOADING */

    EventdPluginContext *self;
    self = g_new0(EventdPluginContext, 1);

    self->lua = luaL_newstate();
    luaL_openlibs(self->lua);

    _eventd_bindings_lua_require(self, "lgi");
    _eventd_bindings_lua_lgi_require(self, "GLib", FALSE);
    _eventd_bindings_lua_lgi_require(self, "Eventd", FALSE);
    _eventd_bindings_lua_lgi_require(self, "EventdPlugin", TRUE);
    lua_remove(self->lua, -2);

    lua_pushlightuserdata(self->lua, self);
    lua_setfield(self->lua, -2, "context");

    lua_pushcfunction(self->lua, _eventd_bindings_lua_register_plugin);
    lua_setfield(self->lua, -2, "register_plugin");

    lua_newtable(self->lua);
    lua_setfield(self->lua, -2, "scripts");

    lua_pop(self->lua, 1);

    const gchar *env_dir;
    env_dir = g_getenv("EVENTD_BINDINGS_LUA_SCRIPTS_DIR");
    if ( env_dir != NULL )
        eventd_bindings_load_directory(self, env_dir, EVENTD_BINDINGS_LUA_SCRIPTS_SUFFIX, _eventd_bindings_lua_load_script);
    eventd_bindings_load_directory(self, EVENTD_BINDINGS_LUA_SCRIPTS_DIR, EVENTD_BINDINGS_LUA_SCRIPTS_SUFFIX, _eventd_bindings_lua_load_script);

    if ( self->scripts == NULL )
    {
        g_warning("No scripts found, aborting");
        _eventd_bindings_lua_uninit(self);
        return NULL;
    }

    return self;
}

static void
_eventd_bindings_lua_uninit(EventdPluginContext *self)
{
    lua_close(self->lua);

    g_list_free_full(self->scripts, g_free);

    g_free(self);
}

static GOptionGroup *
_eventd_bindings_lua_get_option_group(EventdPluginContext *self)
{
    return NULL;
}

static gint
_eventd_bindings_lua_push_object_with_class(EventdPluginContext *self, gpointer object, const gchar *module, const gchar *klass)
{
    lua_getglobal(self->lua, module);
    lua_getfield(self->lua, -1, klass);
    lua_remove(self->lua, -2);
    //lua_getmetatable(self->lua, -1);
    //lua_insert(self->lua, lua_gettop(self->lua) - 1);
    lua_pushlightuserdata(self->lua, object);
    lua_call(self->lua, 1, 1);
    //lua_setmetatable(self->lua, -1);
    return lua_gettop(self->lua);
}

#define foreach_script_with_return(scripts, name, argc, retc, code, default_return_code, return_code) G_STMT_START { \
        lua_getglobal(self->lua, "EventdPlugin"); \
        lua_getfield(self->lua, -1, "scripts"); \
        lua_remove(self->lua, -2); \
        GList *script_; \
        gchar *script; \
        for ( script_ = scripts ; script_ != NULL ; script_ = g_list_next(script_) ) \
        { \
            script = script_->data; \
            lua_getfield(self->lua, -1, script); \
            lua_getfield(self->lua, -1, name); \
            if ( ! lua_isfunction(self->lua, -1) ) \
            { \
                lua_pop(self->lua, 2); \
                default_return_code \
                goto next; \
            } \
            lua_insert(self->lua, lua_gettop(self->lua) - 1); \
            code \
            lua_call(self->lua, argc, retc); \
        next: \
            return_code \
        } \
        lua_pop(self->lua, 1); \
    } G_STMT_END

#define foreach_script(scripts, name, argc) foreach_script_with_return(scripts, name, argc, 0, {}, {}, {})
#define foreach_script_with_code(scripts, name, argc, code) foreach_script_with_return(scripts, name, argc, 0, code, {}, {})

#define foreach_scripts(name, argc) foreach_script_with_return(self->scripts, name, argc, 0, {}, {}, {})
#define foreach_scripts_with_code(name, argc, code) foreach_script_with_return(self->scripts, name, argc, 0, code, {}, {})
#define foreach_scripts_with_return(name, argc, retc, code, default_return_code, return_code) foreach_script_with_return(self->scripts, name, argc, retc, code, default_return_code, return_code)

static void
_eventd_bindings_lua_start(EventdPluginContext *self)
{
    foreach_scripts("start", 1);
}

static void
_eventd_bindings_lua_stop(EventdPluginContext *self)
{
    foreach_scripts("stop", 1);
}

static EventdPluginCommandStatus
_eventd_bindings_lua_control_command(EventdPluginContext *self, guint64 argc, const gchar * const *argv, gchar **status)
{
    return EVENTD_PLUGIN_COMMAND_STATUS_OK;
}

static void
_eventd_bindings_lua_global_parse(EventdPluginContext *self, GKeyFile *key_file)
{
    gint key_file_;
    key_file_ = _eventd_bindings_lua_push_object_with_class(self, key_file, "GLib", "KeyFile");
    foreach_scripts_with_code("global_parse", 2,
        lua_pushvalue(self->lua, key_file_);
    );
    lua_pop(self->lua, 1);
}

static EventdPluginAction *
_eventd_bindings_lua_action_parse(EventdPluginContext *self, GKeyFile *key_file)
{
    GList *script_actions = NULL;
    lua_newtable(self->lua);

    gint key_file_;
    key_file_ = _eventd_bindings_lua_push_object_with_class(self, key_file, "GLib", "KeyFile");
    foreach_scripts_with_return("action_parse", 2, 1,
        lua_pushvalue(self->lua, key_file_);
    ,
        lua_pushnil(self->lua);
    ,
        if ( ! lua_isnil(self->lua, -1) )
        {
            lua_setfield(self->lua, 1, script);
            script_actions = g_list_prepend(script_actions, script);
        }
        else
            lua_pop(self->lua, 1);
    );
    lua_pop(self->lua, 1);

    if ( script_actions == NULL )
    {
        lua_pop(self->lua, 1);
        return NULL;
    }

    EventdPluginAction *action;
    action = g_slice_new0(EventdPluginAction);
    action->index = ++self->current_action;
    action->script_actions = script_actions;

    lua_getglobal(self->lua, "EventdPlugin");
    lua_getfield(self->lua, -1, "actions");
    if ( lua_isnil(self->lua, -1) )
    {
        lua_pop(self->lua, 1);
        lua_newtable(self->lua);
        lua_pushvalue(self->lua, -1);
        lua_setfield(self->lua, 2, "actions");
    }
    lua_remove(self->lua, 2);

    lua_pushinteger(self->lua, action->index);
    lua_pushvalue(self->lua, 1);
    lua_settable(self->lua, 2);
    lua_pop(self->lua, 2);

    return action;
}

static void
_eventd_bindings_lua_action_free(gpointer data)
{
    EventdPluginAction *action = data;

    g_list_free(action->script_actions);

    g_slice_free(EventdPluginAction, action);
}

static void
_eventd_bindings_lua_actions_free(gpointer data)
{
    g_list_free_full(data, _eventd_bindings_lua_action_free);
}

static void
_eventd_bindings_lua_config_reset(EventdPluginContext *self)
{
    g_list_free_full(self->actions, _eventd_bindings_lua_actions_free);
    self->actions = NULL;
    foreach_scripts("config_reset", 1);
}

static void
_eventd_bindings_lua_event_action(EventdPluginContext *self, EventdPluginAction *action, EventdEvent *event)
{
    gint event_;
    lua_getglobal(self->lua, "EventdPlugin");
    lua_getfield(self->lua, 1, "actions");
    lua_remove(self->lua, 1);
    lua_rawgeti(self->lua, 1, action->index);
    lua_remove(self->lua, 1);

    event_ = _eventd_bindings_lua_push_object_with_class(self, event, "Eventd", "Event");
    foreach_script_with_code(action->script_actions, "event_action", 3,
        lua_getfield(self->lua, 1, script);
        lua_pushvalue(self->lua, event_);
    );
    lua_pop(self->lua, 2);
}

EVENTD_BINDINGS_EXPORT const gchar *eventd_plugin_id = "lua";
EVENTD_BINDINGS_EXPORT
void
eventd_plugin_get_interface(EventdPluginInterface *interface)
{
    eventd_plugin_interface_add_init_callback(interface, _eventd_bindings_lua_init);
    eventd_plugin_interface_add_uninit_callback(interface, _eventd_bindings_lua_uninit);

    eventd_plugin_interface_add_get_option_group_callback(interface, _eventd_bindings_lua_get_option_group);

    eventd_plugin_interface_add_start_callback(interface, _eventd_bindings_lua_start);
    eventd_plugin_interface_add_stop_callback(interface, _eventd_bindings_lua_stop);

    eventd_plugin_interface_add_control_command_callback(interface, _eventd_bindings_lua_control_command);

    eventd_plugin_interface_add_global_parse_callback(interface, _eventd_bindings_lua_global_parse);
    eventd_plugin_interface_add_action_parse_callback(interface, _eventd_bindings_lua_action_parse);
    eventd_plugin_interface_add_config_reset_callback(interface, _eventd_bindings_lua_config_reset);

    eventd_plugin_interface_add_event_action_callback(interface, _eventd_bindings_lua_event_action);
}
