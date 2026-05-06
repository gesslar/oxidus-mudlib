// @lpc-nocheck
/**
 * @file /tests/adm/simul_efun/array.misc.test.c
 *
 * Tests for the leftover one-off array simul_efuns:
 * array_item(), array_columns(), and array_sum().
 */

#include <test.h>

inherit STD_TEST;

void setup() {
  describe("array_item", ({
    test("returns an element drawn from the input array", function() {
      mixed *src = ({ "apple", "banana", "cherry", "date" });
      mixed picked = array_item(src);
      ASSERT_EQ(1, in_array(picked, src));
    }),
    test("returns the only element of a single-element array", function() {
      ASSERT_EQ("only", array_item(({ "only" })));
    }),
    test("works with integer arrays", function() {
      int *src = ({ 1, 2, 3, 4, 5 });
      mixed picked = array_item(src);
      ASSERT_EQ(1, in_array(picked, src));
    }),
    test("non-array input errors", function() {
      string err = catch(array_item("not an array"));
      ASSERT_NE(0, err);
    }),
    test("integer input errors", function() {
      string err = catch(array_item(42));
      ASSERT_NE(0, err);
    }),
  }));

  describe("array_sum", ({
    test("sums a populated int array", function() {
      ASSERT_EQ(15, array_sum(({ 1, 2, 3, 4, 5 })));
    }),
    test("empty array returns 0", function() {
      ASSERT_EQ(0, array_sum(({})));
    }),
    test("single element returns that element", function() {
      ASSERT_EQ(7, array_sum(({ 7 })));
    }),
    test("handles negative integers", function() {
      ASSERT_EQ(-5, array_sum(({ 5, -10 })));
    }),
    test("handles a zero element", function() {
      ASSERT_EQ(3, array_sum(({ 0, 1, 2 })));
    }),
    test("non-array input errors", function() {
      string err = catch(array_sum("not an array"));
      ASSERT_NE(0, err);
    }),
    test("non-int-uniform array errors", function() {
      string err = catch(array_sum(({ 1, "two", 3 })));
      ASSERT_NE(0, err);
    }),
  }));

  describe("array_columns", ({
    test("empty input returns empty string", function() {
      ASSERT_EQ("", array_columns(({})));
    }),
    test("default cols=2 width=79 lays out two items per row", function() {
      // colwidth = 79 / 2 = 39. Row 1: "apple" padded to 39, then
      // "banana" padded to 39 — trim() strips the trailing pad on the
      // last column.
      string expected =
        "apple" + sprintf("%-*s", 39 - 5, "") + "banana" + "\n";
      ASSERT_EQ(expected, array_columns(({ "apple", "banana" })));
    }),
    test("uneven items produce an extra row with fewer columns", function() {
      // 5 items into 2 cols => 3 rows; last row has 1 item.
      // colwidth = 79 / 2 = 39.
      string row1 = "a" + sprintf("%-*s", 39 - 1, "") + "b" + "\n";
      string row2 = "c" + sprintf("%-*s", 39 - 1, "") + "d" + "\n";
      // Final row's lone item gets trimmed of trailing padding.
      string row3 = "e\n";
      ASSERT_EQ(row1 + row2 + row3,
        array_columns(({ "a", "b", "c", "d", "e" })));
    }),
    test("custom column count places items into requested columns", function() {
      // 3 cols, default width 79 => colwidth = 26. Single row of 3.
      string expected =
        "x" + sprintf("%-*s", 26 - 1, "") +
        "y" + sprintf("%-*s", 26 - 1, "") +
        "z" + "\n";
      ASSERT_EQ(expected, array_columns(({ "x", "y", "z" }), 3));
    }),
    test("custom width controls column width", function() {
      // 2 cols, width 40 => colwidth = 20.
      string expected =
        "foo" + sprintf("%-*s", 20 - 3, "") + "bar" + "\n";
      ASSERT_EQ(expected, array_columns(({ "foo", "bar" }), 2, 40));
    }),
    test("col<1 falls back to default of 2", function() {
      // col=0 => col=2; width default 79 => colwidth = 39.
      string expected =
        "a" + sprintf("%-*s", 39 - 1, "") + "b" + "\n";
      ASSERT_EQ(expected, array_columns(({ "a", "b" }), 0));
    }),
    test("width<col falls back to default width of 79", function() {
      // col=2, width=1 (< col) => width reset to 79; colwidth = 39.
      string expected =
        "a" + sprintf("%-*s", 39 - 1, "") + "b" + "\n";
      ASSERT_EQ(expected, array_columns(({ "a", "b" }), 2, 1));
    }),
    test("single item across one row trims to just the item", function() {
      // 1 item, default 2 cols, width 79 => 1 row, last column trimmed.
      ASSERT_EQ("solo\n", array_columns(({ "solo" })));
    }),
    test("non-array input errors", function() {
      string err = catch(array_columns("not an array"));
      ASSERT_NE(0, err);
    }),
  }));
}
