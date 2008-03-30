/*
 * Copyright (C) 2008 Rodney Cryderman <rcryderman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
 *
*/
 
 #ifndef __WEBAPPLET_APPLET
 
#define __WEBAPPLET_APPLET

#include <libawn/awn-applet.h>
#include <libawn/awn-config-client.h>

typedef struct
{
  AwnApplet         *applet;
  GtkWidget         *mainwindow;
  GdkPixbuf         *icon;  
  GtkWidget         *box;
  GtkWidget         *viewer;
  AwnConfigClient		*instance_config;  
  AwnConfigClient		*default_config;  

  gint            applet_icon_height;
  gchar           *applet_icon_name;
  gchar           *uri;
  
  int             width;
  int             height;
  
}WebApplet;

#define APPLET_NAME "webapplet"

#endif 