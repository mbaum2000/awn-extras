/*
 * Copyright (c) 2007 Nicolas de BONFILS
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 *
 *
 *
 * Thanks to Miika-Petteri Matikainen for his help, his ideas and some piece of code 
 * (especially for the digital clok, and the function wich let uses a format for the date&time)
 *
 */

/*
 * INCLUDES
 */

#include "clock-applet.h"
#include "utils.h"
	
/*
 * STATIC FUNCTION DEFINITIONS
 */
static void init (Clock *clock);
static void init_pref(Clock *applet);

static gboolean time_handler (Clock *clock);
static void update_time (Clock *applet);

// Drawing
static void draw_digital_clock(Clock *clock, cairo_t *cr, int width, int height);
static void draw_analogic_clock(Clock *clock, cairo_t *cr, int width, int height);

// Events
static void on_awn_height_changed (AwnApplet *app, guint height, Clock *applet);
static void on_awn_orient_changed (AwnApplet *appt, guint orient, Clock *applet);
static gboolean _expose_event (GtkWidget *widget, GdkEventExpose *expose, gpointer data);
static gboolean _button_release_event (GtkWidget *widget, GdkEventButton *event, gpointer *data);


/**
 * Create new applet
 */
Clock*
clock_applet_new (AwnApplet *applet)
{
	Clock *clock = g_new0 (Clock, 1);
	clock->applet = applet;
	
	clock->height = awn_applet_get_height(applet) * 2;
        awn_applet_add_preferences (applet,  "/schemas/apps/awn-clock/prefs", NULL);

	init(clock);

	return clock;
}

/**
 * Initialize the new applet
 */
static void
init (Clock *clock)
{
	clock->tooltips = gtk_tooltips_new ();
	g_object_ref (clock->tooltips);
	gtk_object_sink (GTK_OBJECT (clock->tooltips));

	init_pref(clock);

	printf ("signal\n");

	// connect to button events
	g_signal_connect (G_OBJECT (clock->applet), "button-release-event", G_CALLBACK (_button_release_event), (gpointer)clock);
	g_signal_connect (G_OBJECT (clock->applet), "expose-event", G_CALLBACK (_expose_event), (gpointer)clock);
	
	// connect to height and orientation changes
	g_signal_connect (G_OBJECT (clock->applet), "height-changed", G_CALLBACK (on_awn_height_changed), (gpointer)clock);
	g_signal_connect (G_OBJECT (clock->applet), "orientation-changed", G_CALLBACK (on_awn_orient_changed), (gpointer)clock);

	if (clock->show_second)
		clock->timeout_id = gtk_timeout_add (SECOND_INTERVAL, (GtkFunction) time_handler, clock);
	else
		clock->timeout_id = gtk_timeout_add (MINUTE_INTERVAL, (GtkFunction) time_handler, clock);

}

/*
 * Initialize the preferences
 * TODO : move to GConf
 */
static void 
init_pref(Clock *clock)
{
	clock->y_offset = 0;

	clock->type = CLOCK_ANALOGIC;
	
	clock->show_second = TRUE;
	clock->show_date = TRUE;
	clock->b_24h_mode = TRUE;
	
	clock->font_size = 13;
	clock->font_face = "Sans";
	clock->font_color = "#000000";
	
	clock->rectangle_color = "#cccccc";

	clock->format_date = g_strdup("%d/%m/%Y");
	clock->format_time = g_strdup("%H:%M:%S");
	clock->utc = FALSE;
	

    	/*clock->show_second = awn_applet_gconf_get_bool (clock->applet, "show_second", NULL);
    	clock->show_date = awn_applet_gconf_get_bool (clock->applet, "show_date", NULL);
    	clock->b_24h_mode = awn_applet_gconf_get_bool (clock->applet, "mode_24h", NULL);
	clock->font_size = awn_applet_gconf_get_int (clock->applet, "font_size", NULL);
	clock->font_face = awn_applet_gconf_get_string (clock->applet, "font_face", NULL);*/
	
	/* // load svgs and pngs - not yet implemented
	#ifdef HAVE_SVG
		if (!awn_load_svg (clock->object, clock->dgconf->clock_face, &clock->bg_svg_handle, &error))
	#endif
			awn_load_png (dock, clock->object, clock->dgconf->clock_face, &clock->object->bg_pixbuf, &error);

	#ifdef HAVE_SVG
		if (!awn_load_svg (clock->object, clock->dgconf->clock_shadow, &clock->shadow_svg_handle, &error))
	#endif
			awn_load_png (dock, clock->object, clock->dgconf->clock_shadow, &clock->shadow_pixbuf, &error);

	#ifdef HAVE_SVG
		if (!awn_load_svg (clock->object, clock->dgconf->clock_marks, &clock->mark_svg_handle, &error))
	#endif
			awn_load_png (dock, clock->object, clock->dgconf->clock_marks, &clock->mark_pixbuf, &error);

	#ifdef HAVE_SVG
		if (!awn_load_svg (clock->object, clock->dgconf->clock_glass, &clock->glass_svg_handle, &error))
	#endif
			awn_load_png (dock, clock->object, clock->dgconf->clock_glass, &clock->glass_pixbuf, &error);

	#ifdef HAVE_SVG
		if (!awn_load_svg (clock->object, clock->dgconf->clock_hour_hand, &clock->hour_hand_svg_handle, &error))
	#endif
			awn_load_png (dock, clock->object, clock->dgconf->clock_hour_hand, &clock->hour_hand_pixbuf, &error);

	#ifdef HAVE_SVG
		if (!awn_load_svg (clock->object, clock->dgconf->clock_minute_hand, &clock->minute_hand_svg_handle, &error))
	#endif
			awn_load_png (dock, clock->object, clock->dgconf->clock_minute_hand, &clock->minute_hand_pixbuf, &error);

	#ifdef HAVE_SVG
		if (!awn_load_svg (clock->object, clock->dgconf->clock_second_hand, &clock->second_hand_svg_handle, &error))
	#endif
			awn_load_png (dock, clock->object, clock->dgconf->clock_second_hand, &clock->second_hand_pixbuf, &error);
	*/
}

static void
update_time(Clock *clock)
{
	time_t    g_timeOfDay;
	struct    tm*  g_pTime;

	time (&g_timeOfDay);
	if (clock->utc)
		g_pTime = gmtime(&g_timeOfDay);
	else
		g_pTime = localtime(&g_timeOfDay);
	
	if (g_pTime == NULL)
		g_critical("Error: localtime");

	clock->seconds 	= g_pTime->tm_sec;
	clock->minutes 	= g_pTime->tm_min;
	clock->hours   	= g_pTime->tm_hour;

	// if we use 12h mode, or 24h mode
	if (!clock->b_24h_mode && !clock->type == CLOCK_DIGITAL)
		clock->hours = (clock->hours >= 12) ? clock->hours - 11 : clock->hours;

	clock->day   		= g_pTime->tm_mday;
	clock->month 		= g_pTime->tm_mon + 1;
	clock->year  		= g_pTime->tm_year + 1900;
		
	if (strftime(clock->txt_time, sizeof(clock->txt_time), clock->format_time, g_pTime) == 0)
		g_critical("Error: strftime returned 0 for time");

	if (strftime(clock->txt_date, sizeof(clock->txt_date), clock->format_date, g_pTime) == 0)
		g_critical("Error: strftime returned 0 for date");

}

static void
draw_digital_clock(Clock *clock, cairo_t *cr, int width, int height)
{
	//cairo_text_extents_t extents_time, extents_date;
	double r,g,b;		//for the colour
	PangoFontDescription *pDesc = NULL;
	PangoLayout *pLayout_time = NULL, *pLayout_date = NULL;
	PangoRectangle extents_time, extents_date;
	PangoRectangle extents_logical_time, extents_logical_date;

	pLayout_time = pango_cairo_create_layout(cr);
	if (!pLayout_time)
		return;

	pLayout_date = pango_cairo_create_layout(cr);
	if (!pLayout_date)
		clock->show_date = FALSE;

	pDesc = pango_font_description_new();
	if (!pDesc)
		return;

	update_time(clock);

	// Clear the background to transparent
	cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.0f);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);

	pango_font_description_set_absolute_size(pDesc, PANGO_SCALE*clock->font_size);
	pango_font_description_set_family_static(pDesc, clock->font_face);
	pango_font_description_set_weight(pDesc, PANGO_WEIGHT_BOLD);
	pango_font_description_set_style(pDesc, PANGO_STYLE_NORMAL);
	pango_layout_set_font_description(pLayout_time, pDesc);
	pango_layout_set_font_description(pLayout_date, pDesc);
	pango_font_description_free(pDesc);
	// Pour l'heure
	pango_layout_set_text(pLayout_time, clock->txt_time, -1);
	pango_layout_get_pixel_extents(pLayout_time, &extents_logical_time, &extents_time);
	//Pour la date -> sert a faire les calculs
	pango_layout_set_text(pLayout_date, clock->txt_date, -1);
	pango_layout_get_pixel_extents(pLayout_date, &extents_logical_date, &extents_date);

	int 	padding_top_bottom = 4,
		padding_left_right = 4,
		height_rect = 2*padding_top_bottom + extents_time.height,
	 	width_rect = 2*padding_left_right + extents_time.width,
	 	radius = 14,
		// TODO : fix the x and y coordonates with the height and the width of the applet
	 	x_rect = 6,
		y_rect = 68;
		
	// Si on affiche la date, alors les coordonnée sont modofié en fonction de la taille
	// A noter, tout depend de Pango, si on modifie la police ou autre, alors tout est automatiquement ajusté
	if ( clock->show_date )
	{
		height_rect += extents_date.height;
		width_rect = 2*padding_left_right + MAX(extents_time.width, extents_date.width);
		y_rect -= extents_date.height;
	}
	
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	
	awn_cairo_rounded_rect(cr, 
			       x_rect,
			       y_rect,
			       width_rect,
			       height_rect,
			       radius,
			       ROUND_ALL);
	
	convert_color (clock->rectangle_color, &r, &g, &b);
	cairo_set_source_rgba (cr,r, g, b, .7);
	cairo_fill_preserve (cr);
	// cairo_set_source_rgba (cr, 0.5, 0, 0, 0.5);
	cairo_stroke (cr);

	cairo_save (cr);
	convert_color (clock->font_color, &r, &g, &b);
	cairo_set_source_rgba (cr, r,g,b,1);
	
	int x_time = x_rect + ( width_rect/2 - extents_time.width/2 );
	
	cairo_move_to (cr,  x_time, y_rect + padding_top_bottom );
	pango_cairo_show_layout(cr, pLayout_time);

	// Write the date if allow in the conf
	if ( clock->show_date )
	{
		int x_date = x_rect + ( width_rect/2 - extents_date.width/2 );
		cairo_move_to (cr, x_date, y_rect + padding_top_bottom + extents_date.height + 2);
		pango_cairo_show_layout(cr, pLayout_date);
	}
	
	cairo_restore (cr);
	
	g_object_unref(pLayout_time);
	g_object_unref(pLayout_date);
}

static void
draw_analogic_clock(Clock *clock, cairo_t *cr, int width, int height)
{
	double r, g, b;

	const PangoFontDescription *pDesc = NULL;
	const PangoLayout *pLayout = NULL;
	PangoRectangle extents;
	PangoRectangle extents_logical;


	update_time(clock);

	//Transparence
	cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.0f);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);

	cairo_translate (cr, 0, 18);

	if ( clock->show_date )
	{
		pLayout = pango_cairo_create_layout(cr);
		if (!pLayout)
			clock->show_date = FALSE;

		pDesc = pango_font_description_new();
		if (!pDesc)
			clock->show_date = FALSE;

		pango_font_description_set_absolute_size(pDesc, PANGO_SCALE*clock->font_size);
		pango_font_description_set_family_static(pDesc, clock->font_face);
		pango_font_description_set_weight(pDesc, PANGO_WEIGHT_BOLD);
		pango_font_description_set_style(pDesc, PANGO_STYLE_NORMAL);
		pango_layout_set_font_description(pLayout, pDesc);
		pango_font_description_free(pDesc);
		pango_layout_set_text(pLayout, clock->txt_date, -1);
		pango_layout_get_pixel_extents(pLayout, &extents_logical, &extents);
	}
	
	gboolean full_draw_clock = TRUE;
	// It's a full drawing clock : no svg, no bitmap
	// Use only this one for the moment, the other one has problems yet
	if ( full_draw_clock )
	{
		cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
		
		cairo_move_to(cr, 0, 0);
		awn_cairo_rounded_rect(cr, 
				       1,	//x_rect,
				       16,	//y_rect,
				       MAX(74, extents.width+3),	//width_rect,
				       71,	//height_rect,
				       14,	//radius,
				       ROUND_ALL);
				       
		convert_color (clock->rectangle_color, &r, &g, &b);
		cairo_set_source_rgba (cr, r,g,b, .7);
		cairo_fill_preserve (cr);
		//cairo_set_source_rgba (cr, r,g,b, 0);
		cairo_stroke(cr);

		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

		// Write the date
		if ( clock->show_date )
		{
			cairo_move_to (cr, 3, 18);
			convert_color( clock->font_color, &r, &g, &b);;
			cairo_set_source_rgba(cr, r, g, b, 1.0);
			pango_cairo_show_layout(cr, pLayout);
		}
		
		cairo_new_path (cr);//use it to avoid that a stroke appear from "nowhere"

		cairo_translate( cr, 10, 30 );
		cairo_scale (cr, 55, 55);

		double minutes, hours, seconds;
		// compute the angles for the indicators of our clock
		minutes = clock->minutes * M_PI / 30;
		hours = clock->hours * M_PI / 6 + ((clock->minutes * M_PI / 30)/12);
		seconds = clock->seconds * M_PI / 30;

		cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		cairo_set_line_width(cr, 0.1);

		// translate to the center of the rendering context and draw a black clock outline
		cairo_set_source_rgba(cr, 0, 0, 0, .75);
		//TODO : improve the location, by using the width of the text
		cairo_translate(cr, 0.6, 0.5);
		cairo_arc(cr, 0, 0, 0.4, 0, M_PI * 2);
		cairo_stroke(cr);

		// draw a white dot on the current second.
		cairo_set_source_rgba(cr, 1, 1, 1, 0.6);
		cairo_arc(cr, sin(seconds) * 0.4, -cos(seconds) * 0.4, 0.05, 0, M_PI * 2);
		cairo_fill(cr);

		// draw a white dot on the "quarter".
		cairo_set_source_rgba(cr, 1, 1, 1, 0.9);
		cairo_arc(cr, sin(M_PI/2) * 0.4, -cos(M_PI/2) * 0.4, 0.05, 0, M_PI * 2);
		cairo_fill(cr);

		// draw a white dot on the "half-hour".
		cairo_set_source_rgba(cr, 1, 1, 1, 0.9);
		cairo_arc(cr, sin(M_PI) * 0.4, -cos(M_PI) * 0.4, 0.05, 0, M_PI * 2);
		cairo_fill(cr);

		// draw a white dot on the "quarter to hour".
		cairo_set_source_rgba(cr, 1, 1, 1, 0.9);
		cairo_arc(cr, sin(3*M_PI/2) * 0.4, -cos(3*M_PI/2) * 0.4, 0.05, 0, M_PI * 2);
		cairo_fill(cr);

		// draw a white dot on the "hour".
		cairo_set_source_rgba(cr, 1, 1, 1, 0.9);
		cairo_arc(cr, sin(0) * 0.4, -cos(0) * 0.4, 0.05, 0, M_PI * 2);
		cairo_fill(cr);

		// draw the minutes indicator
		cairo_set_source_rgba(cr, 0.2, 0.2, 1, 0.6);
		cairo_move_to(cr, 0, 0);
		cairo_line_to(cr, sin(minutes) * 0.4, -cos(minutes) * 0.4);
		cairo_stroke(cr);

		// draw the hours indicator
		cairo_move_to(cr, 0, 0);
		cairo_line_to(cr, sin(hours) * 0.2, -cos(hours) * 0.2);
		cairo_stroke(cr);	
		
	} else { 
		// This clock uses bitmap or SVG, 
		// but don't use it yet because there is bug => the location of the hour hand
		// there are many things to improve too, like the size of the differents parts

		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		
		cairo_save (cr);

		awn_cairo_rounded_rect(cr, 
				       4,	//x_rect,
				       4,	// y_rect,
				       90,	//width_rect,
				       90,	//height_rect,
				       14,	//radius,
				       ROUND_ALL);

		convert_color (clock->rectangle_color, &r, &g, &b);
		cairo_set_source_rgba (cr,r, g, b, .7);
		cairo_fill_preserve (cr);
		cairo_stroke (cr);
		
		// TODO : improve it, relativ to the width and height of the applet
		int dim_carre = 90;

		cairo_translate (cr, dim_carre / 2.0, dim_carre / 2.0);
		cairo_rotate (cr, -M_PI/2.0f);

		// hour hand
		// TODO : fix bugs, it location
		cairo_save (cr);
		cairo_rotate (cr, (M_PI/12.0f) * clock->hours );		//+ ((M_PI/720.0f) * (int)clock->minutes) );

		if (clock->hour_hand_pixbuf != NULL)
		{
			gdk_cairo_set_source_pixbuf (cr, clock->hour_hand_pixbuf, 0, 0);
			cairo_paint (cr);
		}
		#ifdef HAVE_SVG
		else if (clock->hour_hand_svg_handle != NULL)
			rsvg_handle_render_cairo (clock->hour_hand_svg_handle, cr);
		#endif
		else {
			convert_color ("#00FF11", &r, &g, &b);
			cairo_set_source_rgba (cr, r, g, b, 1.0);
			cairo_set_line_width (cr, 3.5);
			cairo_move_to (cr, 0.5, 0.5);
			cairo_rel_line_to (cr, 0.25*dim_carre, 0.0);
			cairo_stroke (cr);
		}
		cairo_restore (cr);


		// minute hand
		cairo_save (cr);
		cairo_rotate (cr, (M_PI/30.0f) * clock->minutes);
		if (clock->minute_hand_pixbuf != NULL)
		{
			gdk_cairo_set_source_pixbuf (cr, clock->minute_hand_pixbuf, 0, 0);
			cairo_paint (cr);
		}
		#ifdef HAVE_SVG
		else if (clock->minute_hand_svg_handle != NULL)
			rsvg_handle_render_cairo (clock->minute_hand_svg_handle, cr);
		#endif
		else
		{
			convert_color ("#00FF11", &r, &g, &b);
			cairo_set_source_rgba (cr, r, g, b, 1.0);
			cairo_set_line_width (cr, 2.0);
			cairo_move_to (cr, 0.5, 0.5);
			cairo_rel_line_to (cr, 0.35*dim_carre, 0.0);
			cairo_stroke (cr);
		}
		cairo_restore (cr);

		// second hand
		if (clock->show_second)
		{
			cairo_save (cr);
			cairo_rotate (cr, (M_PI/30.0f) * (int)clock->seconds);
			if (clock->second_hand_pixbuf != NULL)
			{
				gdk_cairo_set_source_pixbuf (cr, clock->second_hand_pixbuf, 0, 0);
				cairo_paint (cr);
			}
			#ifdef HAVE_SVG
			else if (clock->second_hand_svg_handle != NULL)
				rsvg_handle_render_cairo (clock->second_hand_svg_handle, cr);
			#endif
			else
			{
				convert_color ("#00FF11", &r, &g, &b);
				cairo_set_source_rgba (cr, r, g, b, 1.0);
				cairo_set_line_width (cr, 0.75);
				cairo_move_to (cr, 0.5, 0.5);
				cairo_rel_line_to (cr, 0.4*dim_carre, 0.0);
				cairo_stroke (cr);
			}
		}

		// inner circle
		cairo_save (cr);
		convert_color ("#ffffff", &r, &g, &b);
		cairo_set_source_rgba (cr, r, g, b, 1.0);
		cairo_set_line_width (cr, 0.25);
		cairo_arc (cr, 0.5, 0.5, 3.5, 0, 2 * M_PI);
		cairo_fill_preserve (cr);
		cairo_restore (cr);

		/*if (object->foreground_surface != NULL)
		{
			cairo_set_source_surface (cr, object->foreground_surface, 0.0f, 0.0f);
			cairo_paint (cr);
		}*/
		
		// Restore the context, if not the text date wil rotate with the second hand !!!
		cairo_restore (cr);

		if ( clock->show_date )
		{
			//Use it to put the text horizontale and left to right
			cairo_rotate (cr, (M_PI/180.0f) * 90.0f);
			
			// the x and y work correctly but are strange, why -35 degrees ^^
			// TODO : improve the location calcul
			cairo_move_to (cr, -35, 22);
			convert_color( clock->font_color, &r, &g, &b);;
			cairo_set_source_rgba(cr, r, g, b, 1.0);
			pango_cairo_show_layout(cr, pLayout);
		}
	}
}

static gboolean
_expose_event (GtkWidget *widget, GdkEventExpose *expose, gpointer data)
{
	Clock *clock = (Clock *)data;
	cairo_t *cr = NULL;
	gint width, height;

	if (!GDK_IS_DRAWABLE (widget->window))
	{
		g_warning("pas drawable !!\n");
		return FALSE;
	}

	cr = gdk_cairo_create (widget->window);
	if (!cr)
		return FALSE;

	gtk_widget_get_size_request (widget, &width, &height);

	// All the commenting code is to do the reflection, but I don't succed to do it,
	// if someone can send me a patch, I will be happy :) 
	
	/*cairo_surface_t *surface;
	cairo_t *crr = NULL;
	int yy = awn_applet_get_height(clock->applet);
	gint x = 0;
	gint y = yy + (yy - awn_applet_get_height(clock->applet)) / 2;
	gint y_offset = 16;
	// Clearing 
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.0);
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,  awn_applet_get_height(clock->applet)*2, awn_applet_get_height(clock->applet)*2);
	crr = cairo_create(surface);*/
	
	if( clock->type == CLOCK_ANALOGIC )
		draw_analogic_clock(clock, cr, width, height);
	else if( clock->type == CLOCK_DIGITAL )
		draw_digital_clock(clock, cr, width, height);
	
	/*cairo_destroy(crr);
	// Setting coordinates
	cairo_translate(cr, x, y);
	cairo_set_source_surface(cr, surface, 0, 0);
	cairo_paint(cr);
	// Clipping bottom
	// TODO fix
	cairo_rectangle(cr, 0, awn_applet_get_height(clock->applet), awn_applet_get_height(clock->applet)*2, y_offset - 7);
	cairo_clip(cr);
	
	// Mirroring
	cairo_translate(cr,
			awn_applet_get_height(clock->applet),
			awn_applet_get_height(clock->applet) *2 * 2 - 24 +
			(awn_applet_get_height(clock->applet) - awn_applet_get_height(clock->applet)*2) / 2);
	cairo_scale(cr, 1, -1);
	cairo_translate(cr,
			-awn_applet_get_height(clock->applet),
			-awn_applet_get_height(clock->applet) - (awn_applet_get_height(clock->applet)*2 - awn_applet_get_height(clock->applet)));	
	// Reflection
	cairo_set_source_surface(cr, surface, 0, 0);	
	cairo_paint_with_alpha(cr, 0.33);
	cairo_surface_destroy(surface);
	cairo_destroy(cr);
	// Clean up
	cairo_destroy (cr);*/

	return TRUE;
}

static gboolean
time_handler (Clock *clock)
{
	gtk_widget_queue_draw (GTK_WIDGET(clock->applet));
	return TRUE;
}

/**
 * Called on applet height changes
 * -set height and redraw applet
 */
static void
on_awn_height_changed (AwnApplet *applet, guint height, Clock *clock)
{
	clock->height = height;
	gtk_widget_queue_draw (GTK_WIDGET (clock->applet));
	//update_icons (applet);
}

/**
 * Called on applet orientation changes
 * -set orientation and redraw applet
 */
static void
on_awn_orient_changed (AwnApplet *applet, guint orient, Clock *clock)
{
  clock->orient = orient;
  gtk_widget_queue_draw (GTK_WIDGET (clock->applet));
}


/**
 * Event for button released
 * -calls popup_menu
 */
static gboolean
_button_release_event (GtkWidget *widget, GdkEventButton *event, gpointer *data)
{
	Clock *clock = (Clock *)data;
	if (event->button == 3)
	{
	  	// create context menu
	}else if (event->button == 1) {
		g_debug("click");
		
		//Try to use the libawn popup, but it don't show
		//So I do mine => for the next version
		/*GtkWindow * w = awn_applet_dialog_new( clock->applet );
		awn_applet_dialog_postion_reset(AWN_APPLET_DIALOG(w));
		gtk_widget_show(GTK_WIDGET(w));*/
	}else
		return FALSE;

	return TRUE;
}

