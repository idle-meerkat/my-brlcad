/*
 * tstatic.cc
 *
 * Ian Soboroff, NIST
 * June, 1994
 *
 * This tests static allocation of STEP entities, that is, not using the
 * Instance Manager.  We're just going to define some entities, give them
 * some data, and print them out.
 * Just for fun, this includes a small iterator for STEPentity's.  See the
 * file ../SEarritr.h for details.
 *
 */

#include "tests.h"

main()
{
    // This has to be done before anything else.  This initializes
    // all of the registry information for the schema you are using.
    // The SchemaInit() function is generated by fedex_plus... see
    // extern statement above.

    Registry *registry = new Registry(SchemaInit);
    
    // This test creates a bunch of different entities, puts values in them,
    // prints the values out, links them to an array of entities, and then
    // outputs all the enitities in STEP exchange format.

    // For specifics on the structure of the entity classes, see
    // the SdaiEXAMPLE_SCHEMA.h header file.

    const STEPentity* entArr[4];   // our array of entity pointers

    cout << "Creating an SdaiRectangle..." << endl;
    SdaiRectangle rect;
    rect.item_name_("MyRect");
    rect.item_color_(Color__orange);
    rect.number_of_sides_(4);
    rect.height_(5);
    rect.width_(10);
    cout << "Rectangle: (" << rect.opcode() << ") " << endl;
    cout << "  Name: " << rect.item_name_() << endl;
    cout << "  Color: " << rect.item_color_() << endl;
    cout << "  Sides: " << rect.number_of_sides_() << endl;
    cout << "  Height: " << rect.height_() << endl;
    cout << "  Width:  " << rect.width_() << endl;
    cout << endl;
    entArr[0] = &rect;

    cout << "Creating an SdaiSquare..." << endl;
    SdaiSquare square;
    square.item_name_("MySquare");
    square.item_color_(Color__green);
    square.number_of_sides_(4);
    square.height_(3);
    square.width_(3);
    cout << "Square: (" << square.opcode() << ") " << endl;
    cout << "  Name: " << square.item_name_() << endl;
    cout << "  Color: " << square.item_color_() << endl;
    cout << "  Sides: " << square.number_of_sides_() << endl;
    cout << "  Height: " << square.height_() << endl;
    cout << "  Width:  " << square.width_() << endl;
    cout << endl;
    entArr[1] = &square;

    cout << "Creating an SdaiTriangle..." << endl;
    SdaiTriangle tri;
    tri.item_name_("MyTri");
    tri.item_color_(Color__blue);
    tri.number_of_sides_(3);
    tri.side1_length_(3);
    tri.side2_length_(4);
    tri.side3_length_(5);
    cout << "Triangle: (" << tri.opcode() << ") " << endl;
    cout << "  Name: " << tri.item_name_() << endl;
    cout << "  Color: " << tri.item_color_() << endl;
    cout << "  Sides: " << tri.number_of_sides_() << endl;
    cout << "  Side 1: " << tri.side1_length_() << endl;
    cout << "  Side 2: " << tri.side2_length_() << endl;
    cout << "  Side 3: " << tri.side3_length_() << endl;
    cout << endl;
    entArr[2] = &tri;

    cout << "Creating an SdaiCircle..." << endl;
    SdaiCircle circ;
    circ.item_name_("MyCirc");
    circ.item_color_(Color__red);
    circ.number_of_sides_(1);
    circ.radius_(15);
    cout << "Circle: (" << circ.opcode() << ") " << endl;
    cout << "  Name: " << circ.item_name_() << endl;
    cout << "  Color: " << circ.item_color_() << endl;
    cout << "  Sides: " << circ.number_of_sides_() << endl;
    cout << "  Radius: " << circ.radius_() << endl;
    cout << endl;
    entArr[3] = &circ;
    
    cout << "And now, all entities in STEP Exchange Format!" << endl << endl;
    SEarrIterator SEitr(entArr, 4);
    for (SEitr=0; !SEitr; ++SEitr) 
    {
	SEitr()->STEPwrite(cout);
    }
}



