#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include "core/flow_grid.hpp"
#include "core/hooks.hpp"
#include "core/line.hpp"
#include "core/mapper.hpp"
#include "core/text_edit.hpp"
#include "core/text_file.hpp"
#include "core/util.hpp"
#include "core/utf8_util.hpp"
#include "core/util_glob.hpp"
#include "duktape.h"
#include "lm.hpp"
#include "lmgen.hpp"
#include "master_io_provider.hpp"
#include "preferences.hpp"
#include "statlang/symboldb.hpp"
#include "uiwindow.hpp"
#include "myre2.hpp"

#include <QCoreApplication>
#include <QTimer>

TEST_CASE("Line", "[text]") {
  Line line("  foobar ");

  REQUIRE(line.size() == 9);
  REQUIRE(line.get_start() == 2);
  REQUIRE(line.get_end() == 8);
  REQUIRE(line.to_string() == "  foobar ");
  REQUIRE(line.to_string(1, 8) == " foobar");
  REQUIRE(line.is_whitespace() == false);

  SECTION("trimming") {
    line.trim_whitespace();
    REQUIRE(line.size() == 8);
    REQUIRE(line.to_string() == "  foobar");
  }

  SECTION("appending") {
    line.append("bobo");
    REQUIRE(line.size() == 13);
    REQUIRE(line.to_string() == "  foobar bobo");
  }

  SECTION("inserting") {
    line.insert(3, "ghf");
    REQUIRE(line.size() == 12);
    REQUIRE(line.to_string() == "  fghfoobar ");
  }

  SECTION("deleting") {
    line.remove(3, 5);
    REQUIRE(line.size() == 7);
    REQUIRE(line.to_string() == "  fbar ");
  }
}

TEST_CASE("File", "[text]") {
  MasterIOProvider miop;
  TextFile tf(master_io_provider);
  tf.change_path("test_files/small");
  tf.load();

  REQUIRE(tf.get_num_lines() == 7);

  SECTION("UTF8") {
    Line& l = tf.get_line(1);
    REQUIRE(l.size() == 15);
    char32_t c = l.get_char(5).c;
    REQUIRE(c == 0x0161);
    c = l.get_char(6).c;
    REQUIRE(c == 0x0111);
    std::string s = l.to_string();
    REQUIRE(s.size() == 25);
  }
}

TEST_CASE("Mini File", "[text]") {
  MasterIOProvider miop;
  TextFile tf(master_io_provider);
  tf.change_path("test_files/mini");
  tf.load();

  REQUIRE(tf.get_num_lines() == 2);
  REQUIRE(tf.to_string() == "abcd\nefgh");
  CursorLocation start_loc(0, 0);

  SECTION("Edit + Undo Basic Char") {
    {
      SimpleTextEdit ste(tf, CursorLocation(1, 1), &tf);
      ste.insert_char(CursorLocation(0, 2), 'X');
      REQUIRE(ste.get_end_location() == CursorLocation(0, 3));
    }
    REQUIRE(tf.to_string() == "abXcd\nefgh");
    {
      SimpleTextEdit ste(tf, CursorLocation(1, 1), &tf);
      ste.remove_char(CursorLocation(1, 0));
      REQUIRE(ste.get_end_location() == CursorLocation(1, 0));
    }
    REQUIRE(tf.to_string() == "abXcd\nfgh");

    bool rv = tf.get_undo_manager().undo(start_loc);
    REQUIRE(tf.to_string() == "abXcd\nefgh");
    REQUIRE(start_loc == CursorLocation(1,1));
    REQUIRE(rv == true);
    rv = tf.get_undo_manager().undo(start_loc);
    REQUIRE(tf.to_string() == "abcd\nefgh");
    REQUIRE(rv == true);
    rv = tf.get_undo_manager().undo(start_loc);
    REQUIRE(tf.to_string() == "abcd\nefgh");
    REQUIRE(rv == false);
    {
      SimpleTextEdit ste(tf, CursorLocation(1, 1), &tf);
      ste.remove_char(CursorLocation(0, 4));
    }
    REQUIRE(tf.to_string() == "abcdefgh");
    rv = tf.get_undo_manager().undo(start_loc);
    REQUIRE(tf.to_string() == "abcd\nefgh");
    REQUIRE(rv == true);
  }

  SECTION("Edit + Undo Newline") {
    {
      SimpleTextEdit ste(tf, CursorLocation(1, 1), &tf);
      ste.insert_char(CursorLocation(0, 2), '\n');
      REQUIRE(ste.get_end_location() == CursorLocation(1, 0));
    }
    REQUIRE(tf.to_string() == "ab\ncd\nefgh");
    bool rv = tf.get_undo_manager().undo(start_loc);
    REQUIRE(start_loc == CursorLocation(1,1));
    REQUIRE(tf.to_string() == "abcd\nefgh");
    REQUIRE(rv == true);
  }

  SECTION("Edit + Undo remove text") {
    {
      SimpleTextEdit ste(tf, CursorLocation(1, 1), &tf);
      ste.remove_text(CursorLocation(0, 2), CursorLocation(1, 3));
      REQUIRE(ste.get_end_location() == CursorLocation(0, 2));
    }
    REQUIRE(tf.to_string() == "abh");
    bool rv = tf.get_undo_manager().undo(start_loc);
    REQUIRE(start_loc == CursorLocation(1,1));
    REQUIRE(tf.to_string() == "abcd\nefgh");
    REQUIRE(rv == true);
  }

  SECTION("Edit + Undo insert text") {
    {
      SimpleTextEdit ste(tf, CursorLocation(1, 1), &tf);
      ste.insert_text(CursorLocation(0, 2), "POLITIKA");
      REQUIRE(ste.get_end_location() == CursorLocation(0, 10));
    }
    REQUIRE(tf.to_string() == "abPOLITIKAcd\nefgh");
    bool rv = tf.get_undo_manager().undo(start_loc);
    REQUIRE(start_loc == CursorLocation(1,1));
    REQUIRE(tf.to_string() == "abcd\nefgh");
    REQUIRE(rv == true);

    {
      SimpleTextEdit ste(tf, CursorLocation(1, 1), &tf);
      ste.insert_text(CursorLocation(0, 2), "PO\nLIT\nIKA");
      REQUIRE(ste.get_end_location() == CursorLocation(2, 3));
    }
    REQUIRE(tf.to_string() == "abPO\nLIT\nIKAcd\nefgh");
    rv = tf.get_undo_manager().undo(start_loc);
    REQUIRE(start_loc == CursorLocation(1,1));
    REQUIRE(tf.to_string() == "abcd\nefgh");
    REQUIRE(rv == true);
  }

  SECTION("Multi undo") {
    {
      SimpleTextEdit ste(tf, CursorLocation(1, 1), &tf);
      ste.remove_text(CursorLocation(0, 2), CursorLocation(1, 2));
      ste.insert_text(CursorLocation(0, 2), "foo\nbar\nbaz");
    }
    REQUIRE(tf.to_string() == "abfoo\nbar\nbazgh");
    bool rv = tf.get_undo_manager().undo(start_loc);
    REQUIRE(tf.to_string() == "abcd\nefgh");
    REQUIRE(start_loc == CursorLocation(1,1));
    REQUIRE(rv == true);
  }
}

TEST_CASE("Utils") {
  SECTION("UTF8-UTF32") {
    std::string foo = " \n\t abcd foo  ";
    std::vector<uint32_t> cps = utf8_string_to_vector(foo);
    std::string s = utf8_vector_to_string(cps);
    REQUIRE(s == foo);
  }

  SECTION("String split") {
    std::vector<std::string> vec;
    std::string s("foo bar baz");
    utf8_string_split(vec, s, ' ');
    REQUIRE(vec.size() == 3);
    REQUIRE(vec[0] == "foo");
    REQUIRE(vec[1] == "bar");
    REQUIRE(vec[2] == "baz");

    vec.clear();
    s = "   foo   bar \t   baz\n \t";
    utf8_string_split_whitespace(vec, s);
    REQUIRE(vec.size() == 3);
    REQUIRE(vec[0] == "foo");
    REQUIRE(vec[1] == "bar");
    REQUIRE(vec[2] == "baz");
  }

  SECTION("String lower") {
    std::string s("FOO BAR baz");
    s = utf8_string_lower(s);
    REQUIRE(s == "foo bar baz");
  }

  SECTION("Strip") {
    std::string s(" \n\t  ABCD \t ");
    s = utf8_strip(s);
    REQUIRE(s == "ABCD");
  }
}

TEST_CASE("Hooks") {
  SECTION("Hooks dissapear") {
    HookSource<int> hook_source;
    int f = 1;
    {
      Hook<int> hook = hook_source.add([&f](int i) { f += i; });
      REQUIRE(hook_source.num_hooks() == 1);
      hook_source.call(3);
      REQUIRE(f == 4);

      Hook<int> hook2 = hook_source.add([&f](int i) { f += i; });
      REQUIRE(hook_source.num_hooks() == 2);
      hook_source.call(3);
      REQUIRE(f == 10);
    }
    REQUIRE(hook_source.num_hooks() == 0);
  }

  SECTION("Hook source dissapears") {
    Hook<int> hook;
    REQUIRE(hook.valid() == false);
    {
      HookSource<int> hook_source;;
      hook = hook_source.add([](int i) { i += 1; });
      REQUIRE(hook.valid() == true);
      REQUIRE(hook_source.num_hooks() == 1);
    }
    REQUIRE(hook.valid() == false);
  }
}

#ifdef CMAKE_WINDOWS
#define u8
#endif

TEST_CASE("Mapper") {
  std::string str(u8"line1\nliščne2\nline3");
  Mapper mapper(str, 3);

  int row, col;
  mapper.map(2, &row, &col);
  REQUIRE(row == 0);
  REQUIRE(col == 5);
  mapper.map(6, &row, &col);
  REQUIRE(row == 1);
  REQUIRE(col == 0);
  mapper.map(12, &row, &col);
  REQUIRE(row == 1);
  REQUIRE(col == 4);
  mapper.map(17, &row, &col);
  REQUIRE(row == 2);
  REQUIRE(col == 1);
}

void test_lm(std::string addr) {
  printf("addr = %s\n", addr.c_str());
  LMKey key = lmgen(addr);
  REQUIRE(well_formed(key) == true);
  std::string skey = key_to_string(key);
  REQUIRE(well_formed(skey) == true);
  printf("key = %s\n", skey.c_str());
  LMKey lkey = string_to_key(skey);
  for (int i = 0; i < 20; i++) {
    REQUIRE(lkey.b[i] == key.b[i]);
  }
  REQUIRE(check(lkey, addr) == true);
}

TEST_CASE("LM") {
  test_lm("msipos@mailc.net");
  test_lm("info@kpartite.com");
  test_lm("maksimsipos@gmail.com");
  LMKey lkey = string_to_key("c1dr-wful-b0vk-430s-vvsh");
  REQUIRE(check(lkey, "msipos@mailc.net") == false);

  LMKey lkey2 = string_to_key("c1dr-wful-b0vk-430s-vvsa");
  REQUIRE(well_formed(lkey2) == false);

  LMKey lkey3 = string_to_key("c1dr-wful-b1vk-430s-vvsh");
  REQUIRE(well_formed(lkey3) == false);

  LMKey lkey4 = string_to_key("f408-yo4v-nfnu-wd3j-a5ks");
  REQUIRE(check(lkey4, "msipos@mailc.net") == true);
}

TEST_CASE("Search", "[text]") {
  MasterIOProvider miop;
  TextFile tf(master_io_provider);
  tf.change_path("test_files/small");
  tf.load();

  std::vector<SearchResult> results;
  tf.search("line", results, {false, false});

  REQUIRE(results.size() == 3);

  REQUIRE(results[0].row == 2);
  REQUIRE(results[0].col == 6);
  REQUIRE(results[0].size == 4);
  REQUIRE(results[1].row == 4);
  REQUIRE(results[1].col == 14);
  REQUIRE(results[1].size == 4);
  REQUIRE(results[2].row == 6);
  REQUIRE(results[2].col == 5);
  REQUIRE(results[2].size == 4);

  results.clear();

  tf.search(u8"žč", results, {false, false});

  REQUIRE(results.size() == 1);
  REQUIRE(results[0].row == 1);
  REQUIRE(results[0].col == 7);
  REQUIRE(results[0].size == 2);
}

TEST_CASE("Extensions") {
  REQUIRE(".exe" == extract_extension("bar/foo.exe"));
  REQUIRE(".EXE" == extract_extension("/bar/foo.EXE"));
  REQUIRE(".exe" == extract_lowercase_extension("C:/bar/foo.EXE"));
  REQUIRE(".exe" == extract_lowercase_extension("C:\\bar\\foo.EXE"));
  REQUIRE(".golang" == extract_lowercase_extension("C:\\bar\\foo.golang"));
  REQUIRE(".superduperlongextension" == extract_lowercase_extension("C:\\bar\\foo.SuperDuperLongExtension"));
  REQUIRE("" == extract_lowercase_extension("foobar/Makefile"));
  REQUIRE("" == extract_lowercase_extension("Makefile"));
  REQUIRE(".cpp" == extract_lowercase_extension("foobar/.cpp"));
}

TEST_CASE("Globbing") {
  REQUIRE(true == UtilGlob::matches("*.foo", "name.foo"));
  REQUIRE(true == UtilGlob::matches("CMakeLists.txt", "CMakeLists.txt"));
  REQUIRE(true == UtilGlob::matches("r*.cpp", "recents.cpp"));
  REQUIRE(false == UtilGlob::matches("*.foo", "name.bar"));
  REQUIRE(false == UtilGlob::matches("CMakeLists.txt", "CMakeCache.txt"));
  REQUIRE(false == UtilGlob::matches("r*.cpp", "advantage.cpp"));
}

TEST_CASE("Playing with Re2", "[text]") {
  RE2 pattern("[a-z]+");
  REQUIRE(RE2::FullMatch("foobar", pattern) == true);

  RE2 pattern2("$");
  REQUIRE(RE2::FullMatch("", pattern2) == true);
}

TEST_CASE("Statlang SymbolDB", "[statlang]") {
  SymbolDatabase sdb;
  sdb.start_adding();
  sdb.add_symbol("foo", 0, 0);
  sdb.add_symbol("foo", 0, 0);
  sdb.add_symbol("bar", 0, 0);
  sdb.add_symbol("baz", 0, 0);
  sdb.add_symbol("foobar", 0, 0);
  sdb.add_symbol("foobaa", 0, 0);
  sdb.add_symbol("barbaz", 0, 0);
  sdb.finish_adding();

  SECTION("f"){
    std::unordered_map<std::string, int> matches;
    sdb.query_by_prefix("f", matches);
    REQUIRE(matches.size() == 3);
    REQUIRE(matches["foo"] == 2);
    REQUIRE(matches["foobar"] == 1);
    REQUIRE(matches["foobaa"] == 1);
  }
}

TEST_CASE("ContFile", "[text]") {
  Character ch;
  REQUIRE(ch.is_eof() == false);
  ch.set_eof();
  REQUIRE(ch.is_eof() == true);
}

TEST_CASE("Preferences", "[program]") {
  PrefManager pm;
}

TEST_CASE("Duktape") {
  duk_context *ctx = duk_create_heap_default();
  duk_eval_string(ctx, "print('Hello world!');");
  duk_destroy_heap(ctx);
}

TEST_CASE("FlowGrid") {
  MasterIOProvider miop;
  TextFile tf(master_io_provider);
  tf.change_path("test_files/small");
  tf.load();

  FlowGrid fg;
  fg.text_buffer = &tf;
  fg.x_width = 10;
  fg.tab_width = 40;
  fg.line_height = 15;
  fg.folded_line_height = 5;
  fg.word_wrap_width = -1;
  fg.x_offset = 0;
  fg.reflow();

  REQUIRE(fg.map_to_x(0, 0) == 0);
  REQUIRE(fg.map_to_y(0, 0) == 0);
  REQUIRE(fg.map_to_x(1, 3) == 30);
  REQUIRE(fg.map_to_y(1, 3) == 15);
  REQUIRE(fg.output_height == 7*15);
  REQUIRE(fg.output_width == 26*10);
}