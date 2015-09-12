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

#include <glib.h>
#include <helpers.h>

void
eventd_bindings_load_directory(EventdPluginContext *self, const gchar *path, const gchar *suffix, EventdBindingsHelpersScanDirFunc callback)
{
    if ( ! g_file_test(path, G_FILE_TEST_EXISTS|G_FILE_TEST_IS_DIR) )
        return;


    GError *error = NULL;
    GDir *dir;
    dir = g_dir_open(path, 0, &error);
    if ( dir == NULL )
    {
        g_warning("Couldn't open directory '%s': %s", path, error->message);
        g_clear_error(&error);
        return;
    }

    const gchar *name;
    while ( ( name = g_dir_read_name(dir) ) != NULL )
    {
        if ( ! g_str_has_suffix(name, suffix) )
            continue;

        gchar *full_filename;
        full_filename = g_strjoin(G_DIR_SEPARATOR_S, path, name, NULL);

        if ( g_file_test(full_filename, G_FILE_TEST_EXISTS|G_FILE_TEST_IS_REGULAR) )
            callback(self, name, full_filename);

        g_free(full_filename);
    }

    g_dir_close(dir);
}
