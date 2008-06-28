#!/usr/bin/env python
#-*- coding: utf-8 -*-
#
# Copyright (c) 2008 sharkbaitbobby <sharkbaitbobby+awn@gmail.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
#
#To-Do applet - icon file

import cairo
from math import pi

#Setup the colors - lists going from darkest to lightest
#These are the Tango Desktop Project Color Palatte
#(The last set of numbers is the color of the number, added by yours truly)
colors = {}
colors['butter'] = [[252,233,79],[237,212,0],[196,160,0],[64,64,64]]
colors['chameleon'] = [[138,226,52],[115,210,22],[78,154,6],[255,255,255]]
colors['orange'] = [[252,175,62],[245,121,0],[206,92,0],[255,255,255]]
colors['skyblue'] = [[114,159,207],[52,101,164],[32,74,135],[255,255,255]]
colors['plum'] = [[173,127,168],[117,80,123],[92,53,102],[255,255,255]]
colors['chocolate'] = [[233,185,110],[193,125,17],[143,89,2],[255,255,255]]
colors['scarletred'] = [[239,41,41],[204,0,0],[164,0,0],[255,255,255]]
colors['aluminium1'] = [[238,238,236],[211,215,207],[186,189,182],[64,64,64]]
colors['aluminium2'] = [[136,138,133],[85,87,83],[46,52,54],[255,255,255]]

def icon(size,settings,color):
  surface = cairo.ImageSurface(cairo.FORMAT_ARGB32,size,size)
  cr = cairo.Context(surface)
  
  #Get some data
  num_items = 0
  for x in settings['items']:
    if x!='':
      num_items += 1
  
  #Get the needed data based on the icon type
  
  #progress: border shows progress, number shows percent completed
  if settings['icon-type']=='progress':
    if len(settings['progress'])==0:
      number = '100%'
    else:
      progress = settings['progress']
      progress = float(sum(progress))/float(num_items)
      progress = int(progress)
      number = str(progress)+'%'
  
  #progress-items: border shows progress, number shows # items
  elif settings['icon-type']=='progress-items':
    if len(settings['progress'])==0:
      number = '100%'
    else:
      progress = settings['progress']
      number = str(len(progress))
      progress = float(sum(progress))/float(num_items)
      progress = int(progress)
      number = str(progress)+'%'
  
  #items: number shows # items
  elif settings['icon-type']=='items':
    number = str(num_items)
  
  #Draw the outer circle
  cr.set_source_rgba(float(color[2][0])/255.0,float(color[2][1])/255.0,\
    float(color[2][2])/255.0,settings['icon-opacity']/100.0)
  cr.arc(24.0,24.0,23,0,2*pi)
  cr.stroke()
  cr.close_path()
  
  cr.set_line_width(2)
  
  #Draw the inner border - either showing progress or not
  if settings['icon-type'] in ['progress','progress-items']:
    #(Probably) medium background
    cr.set_source_rgba(float(color[1][0])/255.0,float(color[1][1])/255.0,\
      float(color[1][2])/255.0,settings['icon-opacity']/100.0)
    cr.arc(24.0,24.0,22,0,2*pi)
    cr.stroke()
    cr.close_path()
    
    #(Probably) lightest foreground to indicate progress
    cr.set_source_rgba(float(color[0][0])/255.0,float(color[0][1])/255.0,\
      float(color[0][2])/255.0,settings['icon-opacity']/100.0)
    #Crazy maths here
    #http://en.wikipedia.org/wiki/Radians saved my life here :)
    cr.arc(24.0,24.0,22,((3.0*pi)/2.0),((3.0*pi)/2.0)+2*pi*(progress/100.0))
    cr.stroke()
    cr.close_path()
  
  
  #Draw the middle circle normalls
  else:
    cr.set_source_rgba(float(color[0][0])/255.0,float(color[0][1])/255.0,\
      float(color[0][2])/255.0,settings['icon-opacity']/100.0)
    cr.arc(24.0,24.0,22,0,2*pi)
    cr.stroke()
    cr.close_path()
  
  #Draw the inside circle and fill it
  cr.set_source_rgba(float(color[1][0])/255.0,float(color[1][1])/255.0,\
    float(color[1][2])/255.0,settings['icon-opacity']/100.0)
  cr.arc(24.0,24.0,21,0,2*pi)
  cr.clip()
  cr.paint()
  
  #Draw the number
  #Set up the font of the number
  cr.select_font_face("monospace",cairo.FONT_SLANT_NORMAL,\
    cairo.FONT_WEIGHT_BOLD)
  cr.set_source_rgba(float(color[3][0])/255.0,float(color[3][1])/255.0,\
    float(color[3][2])/255.0,settings['icon-opacity']/100.0)
  
  #Actually draw the number
  #determine the size and position of the number based on its digits
  #Note that the # of "digits" includes any % sign
  
  #TODO: Isn't there a better way to do this???
  #1-digit number
  if len(number) == 1:
    cr.move_to(13,38)
    cr.set_font_size(38)
    cr.show_text(number)
  
  #2-digit number
  elif len(number) == 2:
    cr.move_to(8,33)
    cr.set_font_size(26)
    cr.show_text(number)
  
  #3+-digit number
  elif len(number) == 3:
    cr.move_to(7,30)
    cr.set_font_size(19)
    cr.show_text(number)
  
  #100% or this user is crazy (>= 1000 items)
  else:
    cr.move_to(6,29)
    cr.set_font_size(15)
    cr.show_text(number)
  
  #Finish the drawing
  cr.close_path()
  cr.set_line_width(1)
  cr.stroke()
  
  del cr
  return surface