/* workroom.c

Tacitus @ LPUniversity
23-OCT-05
Generic Workroom

This is file demonstates the mudlib's room inheritable
basic capabilities and mudlib coding standards. If you
do not know how to edit this file to produce your own
personalized workroom, then please refer to the mudlib,
lpc, and education documentation or contact a more senior
developer.

*/

/* Last edited on July 11th, 2006 by Tacitus */

// 2024/02/01: Gesslar - some formatting updates

#include <rooms.h>

inherit STD_ROOM;

void setup() {
  set_short("A workroom");
  set_long(
"This is a bare and generic workroom for developers who don't "
"have one of their own. To see the source code for this workroom, "
"you can type 'more here'. There is a sign on the wall."
  );
  set_exits(([
    "square": ROOM_START,
    "void"  : ROOM_VOID
  ]));

  set_items(([
    "sign" : "The sign reads:\n"
    "'You are never given a wish without also being given the "
    "power to make it come true. You may, however, have to work for "
    "it.'",
  ]));

  set_light(1);
}
