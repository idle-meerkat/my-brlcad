#
#			E D I T _ M E N U . T C L
#
# Author -
#	Bob Parker
#
# Source -
#	The U. S. Army Research Laboratory
#	Aberdeen Proving Ground, Maryland  21005
#
# Distribution Notice -
#	Re-distribution of this software is restricted, as described in
#       your "Statement of Terms and Conditions for the Release of
#       The BRL-CAD Package" agreement.
#
# Copyright Notice -
#       This software is Copyright (C) 1998 by the United States Army
#       in all countries except the USA.  All rights reserved.
#
# Description -
#	Routines for implementing Tcl/Tk edit menus.
#

## - init_solid_edit_menus
#
# Routine that build solid edit menus.
#
proc init_solid_edit_menus { stype menu } {
    global mged_players
    global mged_gui
    global edit_type

    if ![info exists mged_players] {
	return
    }

    set stype [cook_solid_type $stype $menu]
    init_solid_edit_menu_hoc $stype

    set edit_type "none of above"
    foreach id $mged_players {
	.$id.menubar.settings.transform entryconfigure 2 -state normal
	set mged_gui($id,transform) "e"
	set_transform $id

	.$id.menubar.settings.coord entryconfigure 2 -state normal
	set mged_gui($id,coords) "o"
	mged_apply $id "set coords $mged_gui($id,coords)"

	.$id.menubar.settings.origin entryconfigure 3 -state normal
	set mged_gui($id,rotate_about) "k"
	mged_apply $id "set rotate_about $mged_gui($id,rotate_about)"

	.$id.menubar.edit entryconfigure 0 -state disabled
	.$id.menubar.edit entryconfigure 1 -state disabled

	set cmds "set mged_gui($id,transform) e; \
		  set_transform $id
	          press \"edit menu\"; "
	set i [build_solid_edit_menus .$id.menubar.edit $id 0 $cmds $menu]

	.$id.menubar.edit insert $i separator
	incr i
	.$id.menubar.edit insert $i radiobutton -variable edit_type \
		-label "Rotate" -underline 0 -command "press srot; \
		set mged_gui($id,transform) e; set_transform $id"
	incr i
	.$id.menubar.edit insert $i radiobutton -variable edit_type \
		-label "Translate" -underline 0 -command "press sxy; \
		set mged_gui($id,transform) e; set_transform $id"
	incr i
	.$id.menubar.edit insert $i radiobutton -variable edit_type \
		-label "Scale" -underline 0 -command "press sscale; \
		set mged_gui($id,transform) e; set_transform $id"
	incr i
	.$id.menubar.edit insert $i radiobutton -variable edit_type \
		-label "none of above" -command "set edit_solid_flag 0; \
		set mged_gui($id,transform) e; set_transform $id"
	incr i
	.$id.menubar.edit insert $i separator
	incr i
	.$id.menubar.edit insert $i command -label "Reject" -underline 0 \
		-command "press reject"
	incr i
	.$id.menubar.edit insert $i command -label "Accept" -underline 0 \
		-command "press accept"
	incr i
	.$id.menubar.edit insert $i command -label "Reset" -underline 0 \
		-command "reset_edit_solid"
	incr i
	.$id.menubar.edit insert $i separator
    }
}

## - build_solid_edit_menus
#
# Routine that recursively builds solid edit menus.
#
proc build_solid_edit_menus { w id pos cmds menu } {
    global mged_gui
    global mged_default
    global edit_type

    # skip menu title
    foreach item_list [lrange $menu 1 end] {
	set item [lindex $item_list 0]
	set submenu [lindex $item_list 1]
	if {$item != "RETURN"} {
	    if {$submenu == {}} {
		$w insert $pos radiobutton \
			-label $item \
			-variable edit_type \
			-command "$cmds; press \"$item\""
	    } else {
		set title [lindex [lindex $submenu 0] 0]
		$w insert $pos cascade \
			-label $item \
			-menu $w.menu$pos
		menu $w.menu$pos -title $title \
			-tearoff $mged_default(tearoff_menus)

		build_solid_edit_menus $w.menu$pos $id 0 \
			"$cmds; press \"$item\"" $submenu
	    }

	    incr pos
	}
    }

    return $pos
}

## - init_object_edit_menus
#
# Routine that build object edit menus.
#
proc init_object_edit_menus {} {
    global mged_players
    global mged_gui
    global edit_type

    if ![info exists mged_players] {
	return
    }

    init_object_edit_menu_hoc

    set edit_type "none of above"
    foreach id $mged_players {
	.$id.menubar.settings.transform entryconfigure 2 -state normal
	set mged_gui($id,transform) "e"
	set_transform $id

	.$id.menubar.settings.coord entryconfigure 2 -state normal
	set mged_gui($id,coords) "o"
	mged_apply $id "set coords $mged_gui($id,coords)"

	.$id.menubar.settings.origin entryconfigure 3 -state normal
	set mged_gui($id,rotate_about) "k"
	mged_apply $id "set rotate_about $mged_gui($id,rotate_about)"

	.$id.menubar.edit entryconfigure 0 -state disabled
	.$id.menubar.edit entryconfigure 1 -state disabled

	set reset_cmd "reset_edit_matrix"

	set i 0
	.$id.menubar.edit insert $i radiobutton -variable edit_type \
		-label "Scale" -command "press \"Scale\"; \
		set mged_gui($id,transform) e; set_transform $id"
	incr i
	.$id.menubar.edit insert $i radiobutton -variable edit_type \
		-label "X move" -command "press \"X move\"; \
		set mged_gui($id,transform) e; set_transform $id"
	incr i
	.$id.menubar.edit insert $i radiobutton -variable edit_type \
		-label "Y move" -command "press \"Y move\"; \
		set mged_gui($id,transform) e; set_transform $id"
	incr i
	.$id.menubar.edit insert $i radiobutton -variable edit_type \
		-label "XY move" -command "press \"XY move\"; \
		set mged_gui($id,transform) e; set_transform $id"
	incr i
	.$id.menubar.edit insert $i radiobutton -variable edit_type \
		-label "Rotate" -command "press \"Rotate\"; \
		set mged_gui($id,transform) e; set_transform $id"
	incr i
	.$id.menubar.edit insert $i radiobutton -variable edit_type \
		-label "Scale X" -command "press \"Scale X\"; \
		set mged_gui($id,transform) e; set_transform $id"
	incr i
	.$id.menubar.edit insert $i radiobutton -variable edit_type \
		-label "Scale Y" -command "press \"Scale Y\"; \
		set mged_gui($id,transform) e; set_transform $id"
	incr i
	.$id.menubar.edit insert $i radiobutton -variable edit_type \
		-label "Scale Z" -command "press \"Scale Z\"; \
		set mged_gui($id,transform) e; set_transform $id"
	incr i
	.$id.menubar.edit insert $i radiobutton -variable edit_type \
		-label "none of above" -command "set edit_object_flag 0; \
		set mged_gui($id,transform) e; set_transform $id"
	incr i
	.$id.menubar.edit insert $i separator
	incr i

	.$id.menubar.edit insert $i command -label "Reject" -underline 0 \
		-command "press reject"
	incr i
	.$id.menubar.edit insert $i command -label "Accept" -underline 0 \
		-command "press accept"
	incr i
	.$id.menubar.edit insert $i command -label "Reset" -underline 0 \
		-command $reset_cmd

	incr i
	.$id.menubar.edit insert $i separator
    }
}

## - undo_edit_menus
#
# Routine that reconfigures the edit menu to its non-edit form
#
proc undo_edit_menus {} {
    global mged_players
    global mged_gui

    if ![info exists mged_players] {
	return
    }

    foreach id $mged_players {
	destroy_edit_info $id

	while {1} {
	    if {[.$id.menubar.edit type 0] == "separator"} {
		.$id.menubar.edit delete 0
		continue
	    }

	    if {[.$id.menubar.edit entrycget 0 -label] != \
		    "Solid Selection..."} {
		.$id.menubar.edit delete 0
	    } else {
		break
	    }
	}

	set submenus [winfo children .$id.menubar.edit]
	foreach submenu $submenus {
	    destroy $submenu
	}

	.$id.menubar.edit entryconfigure 0 -state normal
	.$id.menubar.edit entryconfigure 1 -state normal

	.$id.menubar.settings.transform entryconfigure 2 -state disabled
	if {$mged_gui($id,transform) == "e"} {
	    set mged_gui($id,transform) "v"
	    set_transform $id
	}

	.$id.menubar.settings.coord entryconfigure 2 -state disabled
	if {$mged_gui($id,coords) == "o"} {
	    set mged_gui($id,coords) "v"
	    mged_apply $id "set coords $mged_gui($id,coords)"
	}

	.$id.menubar.settings.origin entryconfigure 3 -state disabled
	if {$mged_gui($id,rotate_about) == "k"} {
	    set mged_gui($id,rotate_about) "v"
	    mged_apply $id "set rotate_about $mged_gui($id,rotate_about)"
	}
    }
}

## - init_object_edit_menu_hoc
#
# Routine that initializes help on context
# for object edit menu entries.
#
proc init_object_edit_menu_hoc {} {
    hoc_register_menu_data "Edit" "Scale" \
	    "Object Edit - Scale"\
	    {{summary "Scale"}}
    hoc_register_menu_data "Edit" "X move" \
	    "Object Edit - X move"\
	    {{summary "X move"}}
    hoc_register_menu_data "Edit" "Y move" \
	    "Object Edit - Y move"\
	    {{summary "Y move"}}
    hoc_register_menu_data "Edit" "XY move" \
	    "Object Edit - XY move"\
	    {{summary "XY move"}}
    hoc_register_menu_data "Edit" "Rotate" \
	    "Object Edit - Rotate"\
	    {{summary "Rotate"}}
    hoc_register_menu_data "Edit" "Scale X" \
	    "Object Edit - Scale X"\
	    {{summary "Scale X"}}
    hoc_register_menu_data "Edit" "Scale Y" \
	    "Object Edit - Scale Y"\
	    {{summary "Scale Y"}}
    hoc_register_menu_data "Edit" "Scale Z" \
	    "Object Edit - Scale Z"\
	    {{summary "Scale Z"}}
    hoc_register_menu_data "Edit" "none of above" \
	    "Object Edit - none of above"\
	    {{summary "Reject"}}

    hoc_register_menu_data "Edit" "Reject" \
	    "Object Edit - Reject"\
	    {{summary "Reject"}}
    hoc_register_menu_data "Edit" "Accept" \
	    "Object Edit - Accept"\
	    {{summary "Accept"}}
    hoc_register_menu_data "Edit" "Reset" \
	    "Object Edit - Reset"\
	    {{summary "Reset"}}
}

## - init_solid_edit_menu_hoc
#
# Routine that initializes help on context
# for solid edit menu entries.
#
proc init_solid_edit_menu_hoc { stype } {
    # Generic solid edit operations
    hoc_register_menu_data "Edit" "Rotate" \
	    "Solid Edit - Rotate" \
	    {{summary "Rotate"}}
    hoc_register_menu_data "Edit" "Translate" \
	    "Solid Edit - Translate" \
	    {{summary "Translate"}}
    hoc_register_menu_data "Edit" "Scale" \
	    "Solid Edit - Scale" \
	    {{summary "Scale"}}
    hoc_register_menu_data "Edit" "none of above" \
	    "Solid Edit - none of above" \
	    {{summary "none of above"}}
    hoc_register_menu_data "Edit" "Reject" \
	    "Solid Edit - Reject" \
	    {{summary "Reject"}}
    hoc_register_menu_data "Edit" "Accept" \
	    "Solid Edit - Accept" \
	    {{summary "Accept"}}
    hoc_register_menu_data "Edit" "Reset" \
	    "Solid Edit - Reset" \
	    {{summary "Reset"}}

    # Solid specific edit operations
    switch $stype {
	ARB8 {
	    # ARB8 EDGES
	    hoc_register_menu_data "ARB8 EDGES" "move edge 12" \
		    "Solid Edit - move edge 12" \
		    {{summary "move edge 12"}}
	    hoc_register_menu_data "ARB8 EDGES" "move edge 23" \
		    "Solid Edit - move edge 23" \
		    {{summary "move edge 23"}}
	    hoc_register_menu_data "ARB8 EDGES" "move edge 34" \
		    "Solid Edit - move edge 34" \
		    {{summary "move edge 34"}}
	    hoc_register_menu_data "ARB8 EDGES" "move edge 14" \
		    "Solid Edit - move edge 14" \
		    {{summary "move edge 14"}}
	    hoc_register_menu_data "ARB8 EDGES" "move edge 15" \
		    "Solid Edit - move edge 15" \
		    {{summary "move edge 15"}}
	    hoc_register_menu_data "ARB8 EDGES" "move edge 26" \
		    "Solid Edit - move edge 26" \
		    {{summary "move edge 26"}}
	    hoc_register_menu_data "ARB8 EDGES" "move edge 56" \
		    "Solid Edit - move edge 56" \
		    {{summary "move edge 56"}}
	    hoc_register_menu_data "ARB8 EDGES" "move edge 67" \
		    "Solid Edit - move edge 67" \
		    {{summary "move edge 67"}}
	    hoc_register_menu_data "ARB8 EDGES" "move edge 78" \
		    "Solid Edit - move edge 78" \
		    {{summary "move edge 78"}}
	    hoc_register_menu_data "ARB8 EDGES" "move edge 58" \
		    "Solid Edit - move edge 58" \
		    {{summary "move edge 58"}}
	    hoc_register_menu_data "ARB8 EDGES" "move edge 37" \
		    "Solid Edit - move edge 37" \
		    {{summary "move edge 37"}}
	    hoc_register_menu_data "ARB8 EDGES" "move edge 48" \
		    "Solid Edit - move edge 48" \
		    {{summary "move edge 48"}}

	    # ARB8 FACES - MOVE
	    hoc_register_menu_data "ARB8 FACES" "move face 1234" \
		    "Solid Edit - move face 1234" \
		    {{summary "move face 1234"}}
	    hoc_register_menu_data "ARB8 FACES" "move face 5678" \
		    "Solid Edit - move face 5678" \
		    {{summary "move face 5678"}}
	    hoc_register_menu_data "ARB8 FACES" "move face 1584" \
		    "Solid Edit - move face 1584" \
		    {{summary "move face 1584"}}
	    hoc_register_menu_data "ARB8 FACES" "move face 2376" \
		    "Solid Edit - move face 2376" \
		    {{summary "move face 2376"}}
	    hoc_register_menu_data "ARB8 FACES" "move face 1265" \
		    "Solid Edit - move face 1265" \
		    {{summary "move face 1265"}}
	    hoc_register_menu_data "ARB8 FACES" "move face 4378" \
		    "Solid Edit - move face 4378" \
		    {{summary "move face 4378"}}

	    # ARB8 FACES - ROTATE
	    hoc_register_menu_data "ARB8 FACES" "rotate face 1234" \
		    "Solid Edit - rotate face 1234" \
		    {{summary "rotate face 1234"}}
	    hoc_register_menu_data "ARB8 FACES" "rotate face 5678" \
		    "Solid Edit - rotate face 5678" \
		    {{summary "rotate face 5678"}}
	    hoc_register_menu_data "ARB8 FACES" "rotate face 1584" \
		    "Solid Edit - rotate face 1584" \
		    {{summary "rotate face 1584"}}
	    hoc_register_menu_data "ARB8 FACES" "rotate face 2376" \
		    "Solid Edit - rotate face 2376" \
		    {{summary "rotate face 2376"}}
	    hoc_register_menu_data "ARB8 FACES" "rotate face 1265" \
		    "Solid Edit - rotate face 1265" \
		    {{summary "rotate face 1265"}}
	    hoc_register_menu_data "ARB8 FACES" "rotate face 4378" \
		    "Solid Edit - rotate face 4378" \
		    {{summary "rotate face 4378"}}
	}
	ARB7 {
	    # ARB7 EDGES
	    hoc_register_menu_data "ARB7 EDGES" "move edge 12" \
		    "Solid Edit - move edge 12" \
		    {{summary "move edge 12"}}
	    hoc_register_menu_data "ARB7 EDGES" "move edge 23" \
		    "Solid Edit - move edge 23" \
		    {{summary "move edge 23"}}
	    hoc_register_menu_data "ARB7 EDGES" "move edge 34" \
		    "Solid Edit - move edge 34" \
		    {{summary "move edge 34"}}
	    hoc_register_menu_data "ARB7 EDGES" "move edge 14" \
		    "Solid Edit - move edge 14" \
		    {{summary "move edge 14"}}
	    hoc_register_menu_data "ARB7 EDGES" "move edge 15" \
		    "Solid Edit - move edge 15" \
		    {{summary "move edge 15"}}
	    hoc_register_menu_data "ARB7 EDGES" "move edge 26" \
		    "Solid Edit - move edge 26" \
		    {{summary "move edge 26"}}
	    hoc_register_menu_data "ARB7 EDGES" "move edge 56" \
		    "Solid Edit - move edge 56" \
		    {{summary "move edge 56"}}
	    hoc_register_menu_data "ARB7 EDGES" "move edge 67" \
		    "Solid Edit - move edge 67" \
		    {{summary "move edge 67"}}
	    hoc_register_menu_data "ARB7 EDGES" "move edge 37" \
		    "Solid Edit - move edge 37" \
		    {{summary "move edge 37"}}
	    hoc_register_menu_data "ARB7 EDGES" "move edge 57" \
		    "Solid Edit - move edget 57" \
		    {{summary "move edge 57"}}
	    hoc_register_menu_data "ARB7 EDGES" "move edge 45" \
		    "Solid Edit - move edge 45" \
		    {{summary "move edge 45"}}
	    hoc_register_menu_data "ARB7 EDGES" "move point 5" \
		    "Solid Edit - move point 5" \
		    {{summary "move point 5"}}

	    # ARB7 FACES - MOVE
	    hoc_register_menu_data "ARB7 FACES" "move face 1234" \
		    "Solid Edit - move face 1234" \
		    {{summary "move face 1234"}}
	    hoc_register_menu_data "ARB7 FACES" "move face 2376" \
		    "Solid Edit - move face 2376" \
		    {{summary "move face 2376"}}

	    # ARB7 FACES - ROTATE
	    hoc_register_menu_data "ARB7 FACES" "rotate face 1234" \
		    "Solid Edit - rotate face 1234" \
		    {{summary "rotate face 1234"}}
	    hoc_register_menu_data "ARB7 FACES" "rotate face 2376" \
		    "Solid Edit - rotate face 2376" \
		    {{summary "rotate face 2376"}}
	}
	ARB6 {
	    # ARB6 EDGES
	    hoc_register_menu_data "ARB6 EDGES" "move edge 12" \
		    "Solid Edit - move edge 12" \
		    {{summary "move edge 12"}}
	    hoc_register_menu_data "ARB6 EDGES" "move edge 23" \
		    "Solid Edit - move edge 23" \
		    {{summary "move edge 23"}}
	    hoc_register_menu_data "ARB6 EDGES" "move edge 34" \
		    "Solid Edit - move edge 34" \
		    {{summary "move edge 34"}}
	    hoc_register_menu_data "ARB6 EDGES" "move edge 14" \
		    "Solid Edit - move edge 14" \
		    {{summary "move edge 14"}}
	    hoc_register_menu_data "ARB6 EDGES" "move edge 15" \
		    "Solid Edit - move edge 15" \
		    {{summary "move edge 15"}}
	    hoc_register_menu_data "ARB6 EDGES" "move edge 25" \
		    "Solid Edit - move edge 25" \
		    {{summary "move edge 25"}}
	    hoc_register_menu_data "ARB6 EDGES" "move edge 36" \
		    "Solid Edit - move edge 36" \
		    {{summary "move edge 36"}}
	    hoc_register_menu_data "ARB6 EDGES" "move edge 46" \
		    "Solid Edit - move edge 46" \
		    {{summary "move edge 46"}}
	    hoc_register_menu_data "ARB6 EDGES" "move point 5" \
		    "Solid Edit - move point 5" \
		    {{summary "move point 5"}}
	    hoc_register_menu_data "ARB6 EDGES" "move point 6" \
		    "Solid Edit - move point 6" \
		    {{summary "move point 6"}}

	    # ARB6 FACES - MOVE
	    hoc_register_menu_data "ARB6 FACES" "move face 1234" \
		    "Solid Edit - move face 1234" \
		    {{summary "move face 1234"}}
	    hoc_register_menu_data "ARB6 FACES" "move face 2365" \
		    "Solid Edit - move face 2365" \
		    {{summary "move face 2365"}}
	    hoc_register_menu_data "ARB6 FACES" "move face 1564" \
		    "Solid Edit - move face 1564" \
		    {{summary "move face 1564"}}
	    hoc_register_menu_data "ARB6 FACES" "move face 125" \
		    "Solid Edit - move face 125" \
		    {{summary "move face 125"}}
	    hoc_register_menu_data "ARB6 FACES" "move face 346" \
		    "Solid Edit - move face 346" \
		    {{summary "move face 346"}}

	    # ARB6 FACES - ROTATE
	    hoc_register_menu_data "ARB6 FACES" "rotate face 1234" \
		    "Solid Edit - rotate face 1234" \
		    {{summary "rotate face 1234"}}
	    hoc_register_menu_data "ARB6 FACES" "rotate face 2365" \
		    "Solid Edit - rotate face 2365" \
		    {{summary "rotate face 2365"}}
	    hoc_register_menu_data "ARB6 FACES" "rotate face 1564" \
		    "Solid Edit - rotate face 1564" \
		    {{summary "rotate face 1564"}}
	    hoc_register_menu_data "ARB6 FACES" "rotate face 125" \
		    "Solid Edit - rotate face 125" \
		    {{summary "rotate face 125"}}
	    hoc_register_menu_data "ARB6 FACES" "rotate face 346" \
		    "Solid Edit - rotate face 346" \
		    {{summary "rotate face 346"}}
	}
	ARB5 {
	    # ARB5 EDGES
	    hoc_register_menu_data "ARB5 EDGES" "move edge 12" \
		    "Solid Edit - move edge 12" \
		    {{summary "move edge 12"}}
	    hoc_register_menu_data "ARB5 EDGES" "move edge 23" \
		    "Solid Edit - move edge 23" \
		    {{summary "move edge 23"}}
	    hoc_register_menu_data "ARB5 EDGES" "move edge 34" \
		    "Solid Edit - move edge 34" \
		    {{summary "move edge 34"}}
	    hoc_register_menu_data "ARB5 EDGES" "move edge 14" \
		    "Solid Edit - move edge 14" \
		    {{summary "move edge 14"}}
	    hoc_register_menu_data "ARB5 EDGES" "move edge 15" \
		    "Solid Edit - move edge 15" \
		    {{summary "move edge 15"}}
	    hoc_register_menu_data "ARB5 EDGES" "move edge 25" \
		    "Solid Edit - move edge 25" \
		    {{summary "move edge 25"}}
	    hoc_register_menu_data "ARB5 EDGES" "move edge 35" \
		    "Solid Edit - move edge 35" \
		    {{summary "move edge 35"}}
	    hoc_register_menu_data "ARB5 EDGES" "move edge 45" \
		    "Solid Edit - move edge 45" \
		    {{summary "move edge 45"}}
	    hoc_register_menu_data "ARB5 EDGES" "move point 5" \
		    "Solid Edit - move point 5" \
		    {{summary "move point 5"}}

	    # ARB5 FACES - MOVE
	    hoc_register_menu_data "ARB5 FACES" "move face 1234" \
		    "Solid Edit - move face 1234" \
		    {{summary "move face 1234"}}
	    hoc_register_menu_data "ARB5 FACES" "move face 125" \
		    "Solid Edit - move face 125" \
		    {{summary "move face 125"}}
	    hoc_register_menu_data "ARB5 FACES" "move face 235" \
		    "Solid Edit - move face 235" \
		    {{summary "move face 235"}}
	    hoc_register_menu_data "ARB5 FACES" "move face 345" \
		    "Solid Edit - move face 345" \
		    {{summary "move face 345"}}
	    hoc_register_menu_data "ARB5 FACES" "move face 145" \
		    "Solid Edit - move face 145" \
		    {{summary "move face 145"}}

	    # ARB5 FACES - ROTATE
	    hoc_register_menu_data "ARB5 FACES" "rotate face 1234" \
		    "Solid Edit - rotate face 1234" \
		    {{summary "rotate face 1234"}}
	    hoc_register_menu_data "ARB5 FACES" "rotate face 125" \
		    "Solid Edit - rotate face 125" \
		    {{summary "rotate face 125"}}
	    hoc_register_menu_data "ARB5 FACES" "rotate face 235" \
		    "Solid Edit - rotate face 235" \
		    {{summary "rotate face 235"}}
	    hoc_register_menu_data "ARB5 FACES" "rotate face 345" \
		    "Solid Edit - rotate face 345" \
		    {{summary "rotate face 345"}}
	    hoc_register_menu_data "ARB5 FACES" "rotate face 145" \
		    "Solid Edit - rotate face 145" \
		    {{summary "rotate face 145"}}
	}
	ARB4 {
	    # ARB4 POINTS
	    hoc_register_menu_data "ARB4 POINTS" "move point 1" \
		    "Solid Edit - move point 1" \
		    {{summary "move point 1"}}
	    hoc_register_menu_data "ARB4 POINTS" "move point 2" \
		    "Solid Edit - move point 2" \
		    {{summary "move point 2"}}
	    hoc_register_menu_data "ARB4 POINTS" "move point 3" \
		    "Solid Edit - move point 3" \
		    {{summary "move point 3"}}
	    hoc_register_menu_data "ARB4 POINTS" "move point 4" \
		    "Solid Edit - move point 4" \
		    {{summary "move point 4"}}

	    # ARB4 FACES - MOVE
	    hoc_register_menu_data "ARB4 FACES" "move face 123" \
		    "Solid Edit - move face 123" \
		    {{summary "move face 123"}}
	    hoc_register_menu_data "ARB4 FACES" "move face 124" \
		    "Solid Edit - move face 124" \
		    {{summary "move face 124"}}
	    hoc_register_menu_data "ARB4 FACES" "move face 234" \
		    "Solid Edit - move face 234" \
		    {{summary "move face 234"}}
	    hoc_register_menu_data "ARB4 FACES" "move face 134" \
		    "Solid Edit - move face 134" \
		    {{summary "move face 134"}}

	    # ARB4 FACES - ROTATE
	    hoc_register_menu_data "ARB4 FACES" "rotate face 123" \
		    "Solid Edit - rotate face 123" \
		    {{summary "rotate face 123"}}
	    hoc_register_menu_data "ARB4 FACES" "rotate face 124" \
		    "Solid Edit - rotate face 124" \
		    {{summary "rotate face 124"}}
	    hoc_register_menu_data "ARB4 FACES" "rotate face 234" \
		    "Solid Edit - rotate face 234" \
		    {{summary "rotate face 234"}}
	    hoc_register_menu_data "ARB4 FACES" "rotate face 134" \
		    "Solid Edit - rotate face 134" \
		    {{summary "rotate face 134"}}
	}
	ars {
	    # ARS
	    hoc_register_menu_data "Edit" "pick vertex" \
		    "Solid Edit - pick vertex" \
		    {{summary "pick vertex"}}
	    hoc_register_menu_data "Edit" "move point" \
		    "Solid Edit - move point" \
		    {{summary "move point"}}
	    hoc_register_menu_data "Edit" "delete curve" \
		    "Solid Edit - delete curve" \
		    {{summary "delete curve"}}
	    hoc_register_menu_data "Edit" "delete column" \
		    "Solid Edit - delete column" \
		    {{summary "delete column"}}
	    hoc_register_menu_data "Edit" "dup curve" \
		    "Solid Edit - dup curve" \
		    {{summary "dup curve"}}
	    hoc_register_menu_data "Edit" "dup column" \
		    "Solid Edit - dup column" \
		    {{summary "dup column"}}
	    hoc_register_menu_data "Edit" "move curve" \
		    "Solid Edit - move curve" \
		    {{summary "move curve"}}
	    hoc_register_menu_data "Edit" "move column" \
		    "Solid Edit - move column" \
		    {{summary "move column"}}

	    # ARS PICK
	    hoc_register_menu_data "ARS PICK MENU" "pick vertex" \
		    "Solid Edit - pick vertex" \
		    {{summary "pick vertex"}}
	    hoc_register_menu_data "ARS PICK MENU" "next vertex" \
		    "Solid Edit - next vertex" \
		    {{summary "next vertex"}}
	    hoc_register_menu_data "ARS PICK MENU" "prev vertex" \
		    "Solid Edit - prev vertex" \
		    {{summary "prev vertex"}}
	    hoc_register_menu_data "ARS PICK MENU" "next curve" \
		    "Solid Edit - next curve" \
		    {{summary "next curve"}}
	    hoc_register_menu_data "ARS PICK MENU" "prev curve" \
		    "Solid Edit - prev curve" \
		    {{summary "prev curve"}}
	}
	tor {
	    # TOR
	    hoc_register_menu_data "Edit" "scale radius 1" \
		    "Solid Edit - scale radius 1" \
		    {{summary "scale radius 1"}}
	    hoc_register_menu_data "Edit" "scale radius 2" \
		    "Solid Edit - scale radius 2" \
		    {{summary "scale radius 2"}}
	}
	eto {
	    # ETO
	    hoc_register_menu_data "Edit" "scale r" \
		    "Solid Edit - scale r" \
		    {{summary "scale r"}}
	    hoc_register_menu_data "Edit" "scale D" \
		    "Solid Edit - scale D" \
		    {{summary "scale D"}}
	    hoc_register_menu_data "Edit" "scale C" \
		    "Solid Edit - scale C" \
		    {{summary "scale C"}}
	    hoc_register_menu_data "Edit" "rotate C" \
		    "Solid Edit - rotate C" \
		    {{summary "rotate C"}}
	}
	ell {
	    # ELL
	    hoc_register_menu_data "Edit" "scale A" \
		    "Solid Edit - scale A" \
		    {{summary "scale A"}}
	    hoc_register_menu_data "Edit" "scale B" \
		    "Solid Edit - scale B" \
		    {{summary "scale B"}}
	    hoc_register_menu_data "Edit" "scale C" \
		    "Solid Edit - scale C" \
		    {{summary "scale C"}}
	    hoc_register_menu_data "Edit" "scale A,B,C" \
		    "Solid Edit - scale A,B,C" \
		    {{summary "scale A,B,C"}}
	}
	spl {
	    # SPLINE
	    hoc_register_menu_data "Edit" "pick vertex" \
		    "Solid Edit - pick vertex" \
		    {{summary "pick vertex"}}
	    hoc_register_menu_data "Edit" "move vertex" \
		    "Solid Edit - move vertex" \
		    {{summary "move vertex"}}
	}
	nmg {
	    # NMG
	    hoc_register_menu_data "Edit" "pick edge" \
		    "Solid Edit - pick edge" \
		    {{summary "pick edge"}}
	    hoc_register_menu_data "Edit" "move edge" \
		    "Solid Edit - move edge" \
		    {{summary "move edge"}}
	    hoc_register_menu_data "Edit" "split edge" \
		    "Solid Edit - split edge" \
		    {{summary "split edge"}}
	    hoc_register_menu_data "Edit" "delete edge" \
		    "Solid Edit - delete edge" \
		    {{summary "delete edge"}}
	    hoc_register_menu_data "Edit" "next eu" \
		    "Solid Edit - next eu" \
		    {{summary "next eu"}}
	    hoc_register_menu_data "Edit" "prev eu" \
		    "Solid Edit - prev eu" \
		    {{summary "prev eu"}}
	    hoc_register_menu_data "Edit" "radial eu" \
		    "Solid Edit - radial eu" \
		    {{summary "radial eu"}}
	    hoc_register_menu_data "Edit" "extrude loop" \
		    "Solid Edit - extrude loop" \
		    {{summary "extrude loop"}}
	    hoc_register_menu_data "Edit" "debug edge" \
		    "Solid Edit - debug edge" \
		    {{summary "debug edge"}}
	}
	part {
	    # PARTICLE
	    hoc_register_menu_data "Edit" "scale H" \
		    "Solid Edit - scale H" \
		    {{summary "scale H"}}
	    hoc_register_menu_data "Edit" "scale v" \
		    "Solid Edit - scale v" \
		    {{summary "scale v"}}
	    hoc_register_menu_data "Edit" "scale h" \
		    "Solid Edit - scale h" \
		    {{summary "scale h"}}
	}
	rpc {
	    # RPC
	    hoc_register_menu_data "Edit" "scale B" \
		    "Solid Edit - scale B" \
		    {{summary "scale B"}}
	    hoc_register_menu_data "Edit" "scale H" \
		    "Solid Edit - scale H" \
		    {{summary "scale H"}}
	    hoc_register_menu_data "Edit" "scale r" \
		    "Solid Edit - scale r" \
		    {{summary "scale r"}}
	}
	rhc {
	    # RHC
	    hoc_register_menu_data "Edit" "scale B" \
		    "Solid Edit - scale B" \
		    {{summary "scale B"}}
	    hoc_register_menu_data "Edit" "scale H" \
		    "Solid Edit - scale H" \
		    {{summary "scale H"}}
	    hoc_register_menu_data "Edit" "scale r" \
		    "Solid Edit - scale r" \
		    {{summary "scale r"}}
	    hoc_register_menu_data "Edit" "scale c" \
		    "Solid Edit - scale c" \
		    {{summary "scale c"}}
	}
	epa {
	    # EPA
	    hoc_register_menu_data "Edit" "scale H" \
		    "Solid Edit - scale H" \
		    {{summary "scale H"}}
	    hoc_register_menu_data "Edit" "scale A" \
		    "Solid Edit - scale A" \
		    {{summary "scale A"}}
	    hoc_register_menu_data "Edit" "scale B" \
		    "Solid Edit - scale B" \
		    {{summary "scale B"}}
	}
	ehy {
	    # EHY
	    hoc_register_menu_data "Edit" "scale H" \
		    "Solid Edit - scale H" \
		    {{summary "scale H"}}
	    hoc_register_menu_data "Edit" "scale A" \
		    "Solid Edit - scale A" \
		    {{summary "scale A"}}
	    hoc_register_menu_data "Edit" "scale B" \
		    "Solid Edit - scale B" \
		    {{summary "scale B"}}
	    hoc_register_menu_data "Edit" "scale c" \
		    "Solid Edit - scale c" \
		    {{summary "scale c"}}
	}
	pipe {
	    # PIPE
	    hoc_register_menu_data "Edit" "select point" \
		    "Solid Edit - select point" \
		    {{summary "select point"}}
	    hoc_register_menu_data "Edit" "next point" \
		    "Solid Edit - next point" \
		    {{summary "next point"}}
	    hoc_register_menu_data "Edit" "previous point" \
		    "Solid Edit - previous point" \
		    {{summary "previous point"}}
	    hoc_register_menu_data "Edit" "move point" \
		    "Solid Edit - move point" \
		    {{summary "move point"}}
	    hoc_register_menu_data "Edit" "delete point" \
		    "Solid Edit - delete point" \
		    {{summary "delete point"}}
	    hoc_register_menu_data "Edit" "append point" \
		    "Solid Edit - append point" \
		    {{summary "append point"}}
	    hoc_register_menu_data "Edit" "prepend point" \
		    "Solid Edit - prepend point" \
		    {{summary "prepend point"}}
	    hoc_register_menu_data "Edit" "scale point OD" \
		    "Solid Edit - scale point OD" \
		    {{summary "scale point OD"}}
	    hoc_register_menu_data "Edit" "scale point ID;" \
		    "Solid Edit - scale point ID" \
		    {{summary "scale point ID"}}
	    hoc_register_menu_data "Edit" "scale point bend" \
		    "Solid Edit - scale point bend" \
		    {{summary "scale point bend"}}
	    hoc_register_menu_data "Edit" "scale pipe OD" \
		    "Solid Edit - scale pipe OD" \
		    {{summary "scale pipe OD;"}}
	    hoc_register_menu_data "Edit" "scale pipe ID" \
		    "Solid Edit - scale pipe ID" \
		    {{summary "scale pipe ID"}}
	    hoc_register_menu_data "Edit" "scale pipe bend" \
		    "Solid Edit - scale pipe bend" \
		    {{summary "scale pipe bend"}}
	}
	vol {
	    # VOL
	    hoc_register_menu_data "Edit" "file name" \
		    "Solid Edit - file name" \
		    {{summary "file name"}}
	    hoc_register_menu_data "Edit" "file size (X Y Z)" \
		    "Solid Edit - file size (X Y Z)" \
		    {{summary "file size (X Y Z)"}}
	    hoc_register_menu_data "Edit" "voxel size (X Y Z)" \
		    "Solid Edit - voxel size (X Y Z)" \
		    {{summary "voxel size (X Y Z)"}}
	    hoc_register_menu_data "Edit" "threshold (low)" \
		    "Solid Edit - threshold (low)" \
		    {{summary "threshold (low)"}}
	    hoc_register_menu_data "Edit" "threshold (hi)" \
		    "Solid Edit - threshold (hi)" \
		    {{summary "threshold (hi)"}}
	}
	ebm {
	    # EBM
	    hoc_register_menu_data "Edit" "file name" \
		    "Solid Edit - file name" \
		    {{summary "file name"}}
	    hoc_register_menu_data "Edit" "file size (W N)" \
		    "Solid Edit - file size (W N)" \
		    {{summary "file size (W N)"}}
	    hoc_register_menu_data "Edit" "extrude depth" \
		    "Solid Edit - extrude depth" \
		    {{summary "extrude depth"}}
	}
	dsp {
	    # DSP
	    hoc_register_menu_data "Edit" "file name" \
		    "Solid Edit - file name" \
		    {{summary "file name"}}
	    hoc_register_menu_data "Edit" "Scale X" \
		    "Solid Edit - Scale X" \
		    {{summary "Scale X"}}
	    hoc_register_menu_data "Edit" "Scale Y" \
		    "Solid Edit - Scale Y" \
		    {{summary "Scale Y"}}
	    hoc_register_menu_data "Edit" "Scale ALT" \
		    "Solid Edit - Scale ALT" \
		    {{summary "Scale ALT"}}
	}
    }
}

## - cook_solid_type
#
# Routine that looks for an incoming solid type of
# type "arb8". If found, look in the edit_menus to
# determine the real arb type. The cooked solid type
# for arbs will be one of the following:
#     ARB4 ARB5 ARB6 ARB7 ARB8
#
# All other types will return the raw solid type as
# the cooked type.
#
proc cook_solid_type { raw_stype edit_menus } {
    switch $raw_stype {
	arb8 {
	    return [lindex [lindex [lindex [lindex [lindex $edit_menus 1] 1] 0] 0] 0]
	}
	default {
	    return $raw_stype
	}
    }
}
