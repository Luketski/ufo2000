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

#ifndef _WEBM_WRITER_EBML_ELEMENT_H_
#define _WEBM_WRITER_EBML_ELEMENT_H_


#include <stdio.h>



#include "webm_writer_utils.h"



class EBML_Element
{
  protected:
    WEBM_Element_ID id;
    
    EBML_Element * parent;
    EBML_Element * first_child;
    EBML_Element * last_child;
    
    EBML_Element * next_sibling;
    EBML_Element * prev_sibling;
    
    int is_first_child;
    int is_last_child;
    int has_parent;
    int number_of_children;
    
    
    int has_data;		// only one of these can be set at the same time
    int has_children;	// only one of these can be set at the same time
              // because when the element is written to a file, the children become the element's "data" - there is no place to put the binary data in that case
              // so an element can have either children or data (not sure about empty elements that have neither, but they are probably OK)
    
    byte * data;
    largeval data_length;
    

    byte * Get_Data ();
    EBML_Element * Set_Data (byte *);
    EBML_Element * Set_Has_Data (int);
    EBML_Element * Set_Has_Children (int);
    EBML_Element * Set_Has_Parent (int);
    EBML_Element * Set_Data_Length (largeval);
    
    
  public:
    // these methods return the same element, so they can be chained
    EBML_Element (WEBM_Element_ID _id);
    EBML_Element * Set_Data_String (char * data);
    EBML_Element * Set_Data_StringN (char * data, largeval length, largeval padding = 0); // padding is the amount of space added to the beginning of the data, and increases the data size by that much - the padding can be used for writing the block header of SimpleBlocks (Frames)
    EBML_Element * Set_Data_Plain_Uint (largeval num);
    EBML_Element * Set_Data_Float (float num);

    WEBM_Element_ID Get_ID ();
    largeval Get_Data_Length (); // gives length of stored data
    byte * Get_Data_String (); // it's a binary string and can contain nul-s, use Get_Data_Length() to determine the length
    largeval Get_Data_Plain_Uint ();

    int Has_Data ();
    int Has_Children ();

    // tree structure management
    // the setter functions return the same element, so they can be chained
      // setter functions
    EBML_Element * Set_Prev_Sibling (EBML_Element *);
    EBML_Element * Set_Next_Sibling (EBML_Element *);
    EBML_Element * Set_Is_Last_Child (int);
    EBML_Element * Set_Is_First_Child (int);
    EBML_Element * Set_Parent (EBML_Element *);
    EBML_Element * Increase_Number_Of_Children (int);
    EBML_Element * Decrease_Number_Of_Children (int);
    EBML_Element * Set_First_Child (EBML_Element *);
    EBML_Element * Set_Last_Child (EBML_Element *);
    EBML_Element * Add_Child (EBML_Element * child);
    EBML_Element * Add_Before (EBML_Element * sibling); // add sibling immediately before this element - if another sibling precedes it, the new element will be between this element and the old previous sibling, so the new element will be the *prev_sibling
    EBML_Element * Add_After (EBML_Element * sibling); // add sibling immediately after this element - if another sibling follows it, the new element will be between this element and the old next sibling, so the new element will be the *next_sibling
      // getter functions
    EBML_Element * Get_Prev_Sibling ();
    EBML_Element * Get_Next_Sibling ();
    EBML_Element * Get_First_Child ();
    EBML_Element * Get_Last_Child ();
    EBML_Element * Get_Parent ();
    int Number_Of_Children ();
    int Is_First_Child ();
    int Is_Last_Child ();
    int Has_Parent ();

    // these functions can be used to predict the result before writing the element to disk
    // the result is calculated recursively, including all child elements and data
    largeval Get_Length_Offset_On_Disk (); // relative to element start on disk
    largeval Get_Data_Offset_On_Disk (); // relative to element start on disk
    largeval Get_Total_Length_On_Disk (int debug_depth = 0);
    largeval Get_Data_Length_On_Disk (int debug_depth = 0);
    largeval Write_To_Disk (FILE * file, largeval segment_data_offset); // write all kids and data recursively

    // this will recursively free all its kids and data 
    ~EBML_Element ();

    void Debug_Write_Structure (int depth = 0);
};



#endif