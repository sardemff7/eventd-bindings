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

#ifndef __EVENTD_BINDINGS_HELPERS_H__
#define __EVENTD_BINDINGS_HELPERS_H__

#include <eventd-plugin.h>

G_BEGIN_DECLS

typedef void (*EventdBindingsHelpersScanDirFunc)(EventdPluginContext *self, const gchar *name, const gchar *path);

void eventd_bindings_load_directory(EventdPluginContext *self, const gchar *path, const gchar *suffix, EventdBindingsHelpersScanDirFunc callback);

G_END_DECLS

#endif /* __EVENTD_BINDINGS_HELPERS_H__ */
