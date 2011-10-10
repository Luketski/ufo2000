/*
This file is part of UFO2000 (http://ufo2000.sourceforge.net)

Copyright (C) 2000-2001  Alexander Ivanov aka Sanami
Copyright (C) 2002       ufo2000 development team

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <assert.h>
// FIXME: change this to use the UFO2000 assert mechanism
#define ASSERT(x) assert(x)


#include "webm_writer_ebml_element.h"
#include "webm_writer_utils.h"


EBML_Element::EBML_Element (WEBM_Element_ID _id)
{
  ////fprintf (stdout, "webm_got_here 1-1-1\n");
  ////fflush (stdout);
  
  ////fprintf (stdout, "EBML_Element %s (%x) created\n", webm_id_manager->Get_ID_Name (_id), this);
  ////fflush (stdout);
  
  
  id = _id;
  
  parent = 0;
  first_child = 0;
  last_child = 0;
  next_sibling = 0;
  prev_sibling = 0;
  
  is_first_child = 1;
  is_last_child = 1;
  has_parent = 0;

  has_data = 0;
  has_children = 0;
  
  number_of_children = 0;
  
  data = 0;
  data_length = 0;
  
  ////fprintf (stdout, "webm_got_here 1-1-2 (%s, %x)\n", webm_id_manager->Get_ID_Name (Get_ID ()), this);
  ////fflush (stdout);
  
}


byte * EBML_Element::Get_Data ()
{
  return data;
}


EBML_Element * EBML_Element::Set_Data (byte * _data)
{
  data = _data;
  return this;
}


EBML_Element * EBML_Element::Set_Has_Data (int has)
{
  if (has) {
    ASSERT (!Has_Children ());
  }
  has_data = has;
  return this;
}


EBML_Element * EBML_Element::Set_Has_Children (int has)
{
  if (has) {
    ASSERT (!Has_Data ());
  }
  has_children = has;
  return this;
}


EBML_Element * EBML_Element::Set_Has_Parent (int has)
{
  has_parent = has;
  return this;
}


EBML_Element * EBML_Element::Set_Data_Length (largeval len)
{
  data_length = len;
  return this;
}


EBML_Element * EBML_Element::Set_Data_String (char * data)
{
  //fprintf (stdout, "%s (%x) (parent: %s, %x) -> Set_Data_String ()\n", webm_id_manager->Get_ID_Name (Get_ID ()), this, webm_id_manager->Get_ID_Name (Has_Parent () ? Get_Parent ()->Get_ID () : WEBM_ID_Void), parent);
  //fflush (stdout);
  
  // element can't have both children and data at the same time
  ASSERT (!Has_Children ());
  
  largeval length = strlen (data);

  byte * data_pointer = (byte *)realloc (Get_Data (), length);
  ASSERT (data_pointer);
  Set_Data (data_pointer);
  Set_Has_Data (1);
  
  memcpy (data_pointer, data, length);
  Set_Data_Length (length);
  
  return this;
}


EBML_Element * EBML_Element::Set_Data_StringN (char * data, largeval length, largeval padding_before /**= 0**/, largeval padding_after /**= 0**/)
{
  //fprintf (stdout, "%s (%x) (parent: %s, %x) -> Set_Data_StringN (length = %d, padding = %d)\n", webm_id_manager->Get_ID_Name (Get_ID ()), this, webm_id_manager->Get_ID_Name (Has_Parent () ? Get_Parent ()->Get_ID () : WEBM_ID_Void), parent, length, padding);
  //fflush (stdout);
  
  ////fprintf (stdout, "webm_got_here 1-1-3\n");
  ////fflush (stdout);
  
  // element can't have both children and data at the same time
  ASSERT (!Has_Children ());
  
  byte * data_pointer = (byte *)realloc (Get_Data (), padding_before + length + padding_after);
  ASSERT (data_pointer);
  Set_Data (data_pointer);
  Set_Has_Data (1);
  
  memcpy (data_pointer + padding_before, data, length);
  if (padding_after > 0)
  {
	  memset (data_pointer + padding_before + length, 0, padding_after);
  }
  Set_Data_Length (padding_before + length + padding_after);

  //fprintf (stdout, "webm_got_here 1-1-4 (%s, %x)\n", webm_id_manager->Get_ID_Name (Get_ID ()), this);
  //fflush (stdout);
  
  return this;
}


/**
*
*	values are stored in big-endian byte order
*
**/
EBML_Element * EBML_Element::Set_Data_Plain_Uint (largeval num)
{
  //fprintf (stdout, "(parent: %s) %s -> Set_Data_Plain_Uint (%d)\n", webm_id_manager->Get_ID_Name (Has_Parent () ? Get_Parent ()->Get_ID () : WEBM_ID_Void), webm_id_manager->Get_ID_Name (Get_ID ()), num);
  //fflush (stdout);
  
  // element can't have both children and data at the same time
  ASSERT (!Has_Children ());
  
  int length = webm_numsize_plain_uint (num);
  byte * data_pointer = (byte *)realloc (Get_Data (), length);
  ASSERT (data_pointer);
  Set_Data (data_pointer);
  Set_Has_Data (1);
  
  webm_pack_bigendian_uint (data_pointer, num);
  Set_Data_Length (length);
  
  return this;
}


WEBM_Element_ID EBML_Element::Get_ID ()
{
  return id;
}


largeval EBML_Element::Get_Data_Length ()
{
  return data_length;
}


byte * EBML_Element::Get_Data_String ()
{
  return data;
}


largeval EBML_Element::Get_Data_Plain_Uint ()
{
  return *((largeval *)data);
}


int EBML_Element::Has_Data ()
{
  return has_data;
}


int EBML_Element::Has_Children ()
{
  return has_children;
}


EBML_Element * EBML_Element::Set_Prev_Sibling (EBML_Element * _prev_sibling)
{
  prev_sibling = _prev_sibling;
  return this;
}


EBML_Element * EBML_Element::Set_Next_Sibling (EBML_Element * _next_sibling)
{
  next_sibling = _next_sibling;
  return this;
}


EBML_Element * EBML_Element::Set_Is_Last_Child (int _is_last_child)
{
  is_last_child = _is_last_child;
  return this;
}


EBML_Element * EBML_Element::Set_Is_First_Child (int _is_first_child)
{
  is_first_child = _is_first_child;
  return this;
}


EBML_Element * EBML_Element::Set_Parent (EBML_Element * _parent)
{
  ASSERT (_parent != this);
  //fprintf (stdout, "Parent of %s (%x) set to %s (%x)\n", webm_id_manager->Get_ID_Name (Get_ID ()), this, webm_id_manager->Get_ID_Name (_parent->Get_ID ()), _parent);
  //fflush (stdout);
  parent = _parent;
  return this;
}


EBML_Element * EBML_Element::Increase_Number_Of_Children (int by_howmany)
{
  number_of_children += by_howmany;
  return this;
}


EBML_Element * EBML_Element::Decrease_Number_Of_Children (int by_howmany)
{
  number_of_children -= by_howmany;
  return this;
}


EBML_Element * EBML_Element::Set_First_Child (EBML_Element * _first_child)
{
  first_child = _first_child;
  return this;
}


EBML_Element * EBML_Element::Set_Last_Child (EBML_Element * _last_child)
{
  last_child = _last_child;
  return this;
}


EBML_Element * EBML_Element::Add_Child (EBML_Element * child)
{
  ////fprintf (stdout, "webm_got_here 1-1-5\n");
  ////fflush (stdout);
  
  child->Set_Parent (this);
  child->Set_Has_Parent (1);
  
  ////fprintf (stdout, "webm_got_here 1-1-6\n");
  ////fflush (stdout);
  
  if (Number_Of_Children () < 1) {
    // this is the first and only child sofar
    ////fprintf (stdout, "webm_got_here 1-1-6-a-1\n");
    ////fflush (stdout);
    
    Set_First_Child (child);
    
    ////fprintf (stdout, "webm_got_here 1-1-6-a-2\n");
    ////fflush (stdout);
    
    child->Set_Is_First_Child (1);

    ////fprintf (stdout, "webm_got_here 1-1-6-a-3\n");
    ////fflush (stdout);
    
  }
  else {
  	// this is not the first child
    ////fprintf (stdout, "webm_got_here 1-1-6-b-1\n");
    ////fflush (stdout);
  
    child->Set_Prev_Sibling (last_child);
    
    ////fprintf (stdout, "webm_got_here 1-1-6-b-2\n");
    ////fflush (stdout);
    
    //fprintf (stdout, "element_id = %s, number_of_children = %d\n", webm_id_manager->Get_ID_Name (Get_ID ()), Number_Of_Children ());
    //fflush (stdout);
    
    Get_Last_Child ()
      ->Set_Next_Sibling (child)
      ->Set_Is_Last_Child (0);
    ;
    
    ////fprintf (stdout, "webm_got_here 1-1-6-b-3\n");
    ////fflush (stdout);
    
  }
  
  ////fprintf (stdout, "webm_got_here 1-1-7\n");
  ////fflush (stdout);

  child->Set_Is_Last_Child (1);
  Set_Last_Child (child);

  ////fprintf (stdout, "webm_got_here 1-1-8\n");
  ////fflush (stdout);
  
  //fprintf (stdout, "element_id = %s, last_child = %x\n", webm_id_manager->Get_ID_Name (Get_ID ()), last_child);
  //fflush (stdout);	
  
  Increase_Number_Of_Children (1);
  
  Set_Has_Children (1);
  
  ////fprintf (stdout, "webm_got_here 1-1-9\n");
  ////fflush (stdout);
  
  return this;
}


EBML_Element * EBML_Element::Add_Before (EBML_Element * sibling)
{
  if (Is_First_Child ()) {
    // this was the first child, the new element takes its place
    Set_Is_First_Child (0);
    sibling
      ->Set_Is_First_Child (1)
    ;
    if (Has_Parent ()) {
      Get_Parent ()
        ->Set_First_Child (sibling)
      ;
    }
  }
  else {
    // this was not the first child
    sibling
      ->Set_Is_First_Child (0)
      ->Set_Prev_Sibling
      (
        Get_Prev_Sibling ()
          ->Set_Next_Sibling (sibling)
      )
    ;
  }
  
  Set_Prev_Sibling (sibling);
  sibling
    ->Set_Is_Last_Child (0)
    ->Set_Next_Sibling (this)
  ;
  
  if (Has_Parent ()) {
    // this element has a parent
    sibling
      ->Set_Has_Parent (1)
      ->Set_Parent
      (
        Get_Parent ()
          ->Increase_Number_Of_Children (1)
      )
    ;
  }
  else {
    // this element has no parent
    sibling
      ->Set_Has_Parent (0)
    ;
  }
  
  return this;
}


EBML_Element * EBML_Element::Add_After (EBML_Element * sibling)
{
  //fprintf (stdout, "element_id = %s", webm_id_manager->Get_ID_Name (Get_ID ()));
  if (Is_Last_Child ()) {
    // this was the last child, the new element takes its place
    //fprintf (stdout, ", is LAST child");
    Set_Is_Last_Child (0);
    sibling
      ->Set_Is_Last_Child (1)
    ;
    if (Has_Parent ()) {
      //fprintf (stdout, ", HAS A PARENT (%s)", webm_id_manager->Get_ID_Name (Get_Parent ()->Get_ID ()));
      Get_Parent ()
        ->Set_Last_Child (sibling)
      ;
    }
    else {
      //fprintf (stdout, ", HAS NO PARENT");
    }
  }
  else {
    // this was not the last child
    //fprintf (stdout, ", is NOT THE LAST child");
    sibling
      ->Set_Is_Last_Child (0)
      ->Set_Next_Sibling
      (
        Get_Next_Sibling ()
          ->Set_Prev_Sibling (sibling)
      )
    ;
  }
  //fprintf (stdout, "\n");
  //fflush (stdout);	


  Set_Next_Sibling (sibling);
  sibling
    ->Set_Is_First_Child (0)
    ->Set_Prev_Sibling (this)
  ;
  
  if (Has_Parent ()) {
    // this element has a parent
    sibling
      ->Set_Has_Parent (1)
      ->Set_Parent
      (
        Get_Parent ()
          ->Increase_Number_Of_Children (1)
      )
    ;
  }
  else {
    // this element has no parent
    sibling
      ->Set_Has_Parent (0)
    ;
  }
  
  return this;
}


EBML_Element * EBML_Element::Get_Prev_Sibling ()
{
  if (Is_First_Child ()) {
    return 0;
  }
  return prev_sibling;
}


EBML_Element * EBML_Element::Get_Next_Sibling ()
{
  if (Is_Last_Child ()) {
    return 0;
  }
  return next_sibling;
}


EBML_Element * EBML_Element::Get_First_Child ()
{
  if (!Has_Children ()) {
    return 0;
  }
  return first_child;
}


EBML_Element * EBML_Element::Get_Last_Child ()
{
  //fprintf (stdout, "webm_got_here 1-1-6-b-2-1\n");
  //fflush (stdout);
  
  //fprintf (stdout, "element_id = %s, last_child = %x\n", webm_id_manager->Get_ID_Name (Get_ID ()), last_child);
  //fflush (stdout);	
  
  if (!Has_Children ()) {
    //fprintf (stdout, "webm_got_here 1-1-6-b-2-1-a-1\n");
    //fflush (stdout);
    return 0;
  }

  //fprintf (stdout, "webm_got_here 1-1-6-b-2-2\n");
  //fflush (stdout);

  return last_child;
}


EBML_Element * EBML_Element::Get_Parent ()
{
  if (!Has_Parent ()) {
    return 0;
  }
  return parent;
}


int EBML_Element::Number_Of_Children ()
{
  return number_of_children;
}


int EBML_Element::Is_First_Child ()
{
  return is_first_child;
}


int EBML_Element::Is_Last_Child ()
{
  return is_last_child;
}


int EBML_Element::Has_Parent ()
{
  return has_parent;
}


largeval EBML_Element::Get_Length_Offset_On_Disk ()
{
  return webm_id_manager->Get_Signature_Length (Get_ID ());
}


largeval EBML_Element::Get_Data_Offset_On_Disk ()
{
  return Get_Length_Offset_On_Disk () + webm_numsize_u (Get_Data_Length_On_Disk ());
}


largeval EBML_Element::Get_Total_Length_On_Disk (int debug_depth)
{
  //fprintf (stdout, "Get_Total_Length_On_Disk: ");
  for (int i = 0, imax = debug_depth; i < imax; i++) {
    //fprintf (stdout, "  ");
  }
  //fprintf (stdout, "%s (%08x), nkids = %d ^\\\n", webm_id_manager->Get_ID_Name (Get_ID ()), this, Number_Of_Children ());
  //fflush (stdout);
  
  
  // length on disk = size of element ID + size of element length field + size of data or children
  largeval length = 0;
  
  length += webm_id_manager->Get_Signature_Length (Get_ID ());
  
  largeval datasize = Get_Data_Length_On_Disk (debug_depth);
  
  length += webm_numsize_u (datasize) + datasize;

  //fprintf (stdout, "Get_Total_Length_On_Disk: ");
  for (int i = 0, imax = debug_depth; i < imax; i++) {
    //fprintf (stdout, "  ");
  }
  //fprintf (stdout, "%s (%08x), nkids = %d _/ TOTAL LENGTH = %d\n", webm_id_manager->Get_ID_Name (Get_ID ()), this, Number_Of_Children (), length);
  //fflush (stdout);
  
  return length;
}


largeval EBML_Element::Get_Data_Length_On_Disk (int debug_depth)
{
  largeval datasize = 0;
  
  if (Has_Data ()) {
    datasize = Get_Data_Length ();
  }
  else if (Has_Children ()) {
    largeval kids_length = 0;
    EBML_Element * kid = Get_First_Child ();
    while (1) {
      kids_length += kid->Get_Total_Length_On_Disk (debug_depth + 1);
      
      if (kid->Is_Last_Child ()) {
        break;
      }
      kid = kid->Get_Next_Sibling ();
    }
    datasize = kids_length;
  }
  
  return datasize;
}


largeval EBML_Element::Write_To_Disk (FILE * file, largeval segment_data_offset)
{
  //fprintf (stdout, "webm element %s written to file at position %d (segment-relative pos: %d)\n", webm_id_manager->Get_ID_Name (Get_ID ()), ftell (file), ftell (file) - segment_data_offset);
  //fflush (stdout);
  
  uint siglen = webm_id_manager->Get_Signature_Length (Get_ID ());
  ASSERT (fwrite (webm_id_manager->Get_ID_Signature (Get_ID ()), 1, siglen, file) == siglen);
  
  largeval datalen = Get_Data_Length_On_Disk ();
  uint lenlen = webm_numsize_u (datalen);
  byte length [WEBM_MAX_ID_BYTES];
  webm_pack_u (length, datalen);
  ASSERT (fwrite (length, 1, lenlen, file) == lenlen);
  
  if (Has_Data ()) {
    ASSERT (fwrite (Get_Data (), 1, datalen, file) == datalen);
    char content_preview [10];
    int preview_len = (datalen > 10 ? 10 : datalen);
    snprintf (content_preview, preview_len, "%s", Get_Data ());
    char format_string [100];
    sprintf (format_string, "webm element %%s has data, length = %%d, content = ");
    int i, imax;
    for (i = strlen (format_string), imax = i + preview_len * 2; i < imax; i += 2) {
      sprintf (format_string + i, "%02x", content_preview[i]);
    }
    sprintf (format_string + i, "\n");
    //fprintf (stdout, format_string, webm_id_manager->Get_ID_Name (Get_ID ()), datalen, content_preview);
    //fflush (stdout);
  }
  else if (Has_Children ()) {
    EBML_Element * kid = Get_First_Child ();
    int kidn = 0;
    int nkids = Number_Of_Children ();
    while (1) {
      //fprintf (stdout, "webm element %s / kid %d of %d (name = %s) will be written at position %d (segment-relative pos: %d)\n", webm_id_manager->Get_ID_Name (Get_ID ()), kidn++, nkids, webm_id_manager->Get_ID_Name (kid->Get_ID ()), ftell (file), ftell (file) - segment_data_offset);
      //fflush (stdout);
  
      kid->Write_To_Disk (file, segment_data_offset);
      
      if (kid->Is_Last_Child ()) {
        break;
      }
      kid = kid->Get_Next_Sibling ();
    }
  }
  
  return siglen + lenlen + datalen;
}

    
EBML_Element::~EBML_Element ()
{
  if (Has_Data ()) {
    byte * data = Get_Data ();
    if (data) {
      free (data);
      Set_Data (data = 0);
    }
  }
  else if (Has_Children ()) {
    EBML_Element * kid = Get_First_Child ();
    while (1) {
      EBML_Element * kid_to_delete = kid;
      
      int was_last_child = 0;
      if (kid->Is_Last_Child ()) {
        was_last_child = 1;
      }
      else {
        kid = kid->Get_Next_Sibling ();
      }
      
      delete (kid_to_delete);
      kid_to_delete = 0;
    
      Decrease_Number_Of_Children (1);
      
      if (was_last_child) {
        break;
      }
      
      Set_First_Child (kid);
    }
    Set_First_Child (0);
    Set_Last_Child (0);
  }
}


void EBML_Element::Debug_Write_Structure (int depth)
{
  fprintf (stdout, "|");
  for (int i = 0, imax = depth; i < imax; i++) {
    fprintf (stdout, "+");
  }
  fprintf (stdout, "%s (%x) -- children: %d\n", webm_id_manager->Get_ID_Name (Get_ID ()), this, Number_Of_Children ());
  //fflush (stdout);
  
  if (Has_Children ()) {
    EBML_Element * kid = Get_First_Child ();
    while (kid) {
      kid->Debug_Write_Structure (depth + 1);
      if (kid->Is_Last_Child ()) {
        break;
      }
      kid = kid->Get_Next_Sibling ();
    }
  }
}



EBML_Element * EBML_Element::Set_Data_Float (float num)
{
  //fprintf (stdout, "(parent: %s) %s -> Set_Data_Float (%d)\n", webm_id_manager->Get_ID_Name (Has_Parent () ? Get_Parent ()->Get_ID () : WEBM_ID_Void), webm_id_manager->Get_ID_Name (Get_ID ()), num);
  //fflush (stdout);
  
  // element can't have both children and data at the same time
  ASSERT (!Has_Children ());
  
  int length = webm_numsize_float (num);
  byte * data_pointer = (byte *)realloc (Get_Data (), length);
  ASSERT (data_pointer);
  Set_Data (data_pointer);
  Set_Has_Data (1);
  
  webm_pack_bigendian_float (data_pointer, num);
  
  Set_Data_Length (length);
  
  return this;
}

