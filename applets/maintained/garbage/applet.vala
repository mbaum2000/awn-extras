/*
 * Trash applet written in Vala.
 *
 * Copyright (C) 2008, 2009 Mark Lee <avant-wn@lazymalevolence.com>
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
 * Author : Mark Lee <avant-wn@lazymalevolence.com>
 */

using Cairo;
using Gdk;
using Gtk;
using Awn;
using DesktopAgnostic;
using DesktopAgnostic.Config;

public class GarbageApplet : AppletSimple
{
  public VFS.Trash trash;
  private Client config;
  private string app_name;
  private Menu menu;
  private OverlayText? text_overlay;

  /*const TargetEntry[] targets = {
    { "text/uri-list", 0, 0 },
    { "text/plain",    0, 0 },
    { null }
  };*/

  construct
  {
    this.trash = VFS.trash_get_default ();
    this.trash.file_count_changed.connect (this.trash_changed);
    this.app_name = _ ("Garbage");
    this.map_event.connect (this.on_map_event);
    this.button_press_event.connect (this.on_click);
    this.text_overlay = null;
  }

  public GarbageApplet (string canonical_name, string uid, int panel_id)
  {
    this.canonical_name = canonical_name;
    this.uid = uid;
    this.panel_id = panel_id;
    this.single_instance = true;
    this.config = Awn.Config.get_default_for_applet (this);
    this.render_applet_icon ();
  }

  private bool initialize_dragdrop ()
  {
    TargetEntry[] targets = new TargetEntry[2];
    targets[0].target = "text/uri-list";
    targets[0].flags = 0;
    targets[0].info =  0;
    targets[1].target = "text/plain";
    targets[1].flags = 0;
    targets[1].info =  0;
    drag_source_set (this, ModifierType.BUTTON1_MASK, targets, DragAction.COPY);
    drag_dest_set (this, DestDefaults.ALL, targets, DragAction.COPY);
    this.drag_data_received.connect (this.on_drag_data_received);
    return false;
  }

  private bool on_map_event (Event evt)
  {
    Timeout.add (200, this.initialize_dragdrop);
    return true;
  }

  private void render_applet_icon ()
  {
    uint file_count;

    file_count = this.trash.file_count;
    if (file_count > 0)
    {
      icon_name = "user-trash-full";
    }
    else
    {
      icon_name = "user-trash";
    }
    // set icon
    this.set_icon_name (icon_name);
    // if requested, draw trash count when count > 0
    try
    {
      if (this.config.get_bool (GROUP_DEFAULT, "show_count") && file_count > 0)
      {
        if (this.text_overlay == null)
        {
          unowned Overlayable overlayable;

          // moonbeam says get_icon generally returns Awn.ThemedIcon
          overlayable = this.get_icon () as Overlayable;
          this.text_overlay = new OverlayText ();
          overlayable.add_overlay (this.text_overlay);
        }

        if (!this.text_overlay.active)
        {
          this.text_overlay.active = true;
        }

        this.text_overlay.text = "%u".printf (file_count);
      }
      else if (this.text_overlay != null)
      {
        if (this.text_overlay.active)
        {
          this.text_overlay.active = false;
        }
      }
    }
    catch (GLib.Error err)
    {
      warning ("Rendering error: %s", err.message);
    }
    // set the title as well
    string plural;
    if (file_count == 1)
    {
      plural = _("item");
    }
    else
    {
      plural = _("items");
    }
    this.set_tooltip_text ("%s: %u %s".printf (this.app_name, file_count, plural));
  }
  private bool on_click (EventButton evt)
  {
    switch (evt.button)
    {
      case 1: /* left mouse click */
        try
        {
          string[] argv = new string[] { "xdg-open", "trash:" };
          spawn_on_screen (this.get_screen (),
                           null,
                           argv,
                           null,
                           SpawnFlags.SEARCH_PATH,
                           null,
                           null);
        }
        catch (GLib.Error e)
        {
          // FIXME: Show the user the error somehow.
          warning ("Could not open the trash folder in your file manager: %s",
                   e.message);
        }
        break;
      case 2: /* middle mouse click */
        break;
      case 3: /* right mouse click */
        weak Menu ctx_menu;
        if (this.menu == null)
        {
          MenuItem item;
          this.menu = this.create_default_menu () as Menu;
          item = new MenuItem.with_mnemonic (_ ("_Empty Trash"));
          item.activate.connect (this.on_menu_empty_activate);
          item.show ();
          this.menu.append (item);
        }
        ctx_menu = (Menu)this.menu;
        ctx_menu.set_screen (null);
        ctx_menu.popup (null, null, null, evt.button, evt.time);
        break;
    }
    return true;
  }
  private void on_menu_empty_activate ()
  {
    bool do_empty;
    try
    {
      if (config.get_bool("DEFAULT", "confirm_empty"))
      {
        string msg = _ ("Are you sure you want to empty your trash? It " +
                        "currently contains %u item(s).")
                     .printf (this.trash.file_count);
        MessageDialog dialog = new MessageDialog ((Gtk.Window)this, 0,
                                                  MessageType.QUESTION,
                                                  ButtonsType.YES_NO, msg);
        int response = dialog.run ();
        dialog.destroy ();
        do_empty = (response == ResponseType.YES);
      }
      else
      {
        do_empty = true;
      }
      if (do_empty)
      {
        this.trash.empty ();
      }
    }
    catch (GLib.Error ex)
    {
      warning ("Error occurred when trying to retrieve 'confirm_empty' config option: %s",
               ex.message);
      /* FIXME show error dialog */
    }
  }
  private void trash_changed ()
  {
    this.render_applet_icon ();
  }
  private void on_drag_data_received (DragContext context,
                                      int x,
                                      int y,
                                      SelectionData data,
                                      uint info,
                                      uint time)
  {
    SList<VFS.File> file_uris;

    try
    {
      file_uris = VFS.files_from_uri_list ((string)data.data);
      foreach (weak VFS.File file in file_uris)
      {
        if (file.exists ())
        {
          this.trash.send_to_trash (file);
        }
      }
    }
    catch (GLib.Error err)
    {
      string msg = _ ("Could not send the dragged file(s) to the trash: %s")
        .printf (err.message);
      MessageDialog dialog = new MessageDialog ((Gtk.Window)this, 0,
                                                MessageType.ERROR,
                                                ButtonsType.OK, msg);
      dialog.run ();
      dialog.destroy ();
    }
  }
}

public Applet awn_applet_factory_initp (string canonical_name, string uid, int panel_id)
{
  return new GarbageApplet (canonical_name, uid, panel_id);
}

// vim: set ft=vala et ts=2 sts=2 sw=2 ai :
