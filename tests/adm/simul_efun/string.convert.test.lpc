// @lpc-nocheck
/**
 * @file /tests/adm/simul_efun/string.convert.test.c
 *
 * Tests for from_string() from /adm/simul_efun/string.c, which parses an LPC
 * value out of its string representation.
 */

#include <test.h>

inherit STD_TEST;

void setup() {
  describe("from_string scalars", ({
    test("parses a positive integer", function() {
      ASSERT_EQ(123, from_string("123"));
    }),
    test("parses a negative integer", function() {
      ASSERT_EQ(-5, from_string("-5"));
    }),
    test("parses a hexadecimal integer", function() {
      ASSERT_EQ(32, from_string("0x20"));
    }),
    test("parses a float", function() {
      ASSERT_EQ(3.14, from_string("3.14"));
    }),
    test("parses a quoted string", function() {
      ASSERT_EQ("hello", from_string("\"hello\""));
    }),
    // The hex scan loop only accepts digits 0-9, never a-f, so any hex literal
    // containing a letter digit stops early: from_string("0xFF") yields 0.
    pending("parses a hexadecimal integer with letter digits",
      "hex loop excludes a-f, see string.c#L316"),
    test("an empty string parses to 0", function() {
      ASSERT_EQ(0, from_string(""));
    }),
    test("a whitespace-only string parses to 0", function() {
      ASSERT_EQ(0, from_string("   "));
    }),
  }));

  describe("from_string collections", ({
    test("parses an array of integers", function() {
      ASSERT_EQ(1, same(({ 1, 2, 3 }), from_string("({1,2,3,})")));
    }),
    test("parses an array of strings", function() {
      ASSERT_EQ(1, same(({ "a", "b" }), from_string("({\"a\",\"b\",})")));
    }),
    test("parses an empty array", function() {
      ASSERT_EQ(1, same(({}), from_string("({})")));
    }),
    test("parses a mapping", function() {
      ASSERT_EQ(1, same(([ "a": 1 ]), from_string("([\"a\":1,])")));
    }),
  }));

  describe("from_string with flag", ({
    test("returns value and remaining string for an integer", function() {
      ASSERT_EQ(1, same(({ 123, "rest" }), from_string("123 rest", 1)));
    }),
  }));
}
