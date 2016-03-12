#include "core/encoding.hpp"
#include "core/line.hpp"
#include "core/text_file.hpp"
#include "core/util.hpp"
#include "core/utf8_util.hpp"
#include "io_provider.hpp"

#include <cassert>
#include <cstdio>
#include <ctime>
#include <utility>
#include <cctype>

#include <QFile>
#include <QFileInfo>
#include <QString>

#define READ_BUF_SIZE 512

TextFile::TextFile(IOProvider* iop) : line_endings(UNIX), unsaved_edits(false), undo_manager(*this), encoding("UTF-8"), io_provider(iop) {
#ifdef CMAKE_WINDOWS
  line_endings = WINDOWS;
#endif
  undo_manager.add_save_point();
}

std::string TextFile::get_file_name() {
  if (absolute_path.empty()) return "";

  QFileInfo q_file_info(QString::fromStdString(absolute_path));
  return q_file_info.fileName().toStdString();
}

std::string TextFile::get_extension() {
  return extract_extension(absolute_path);
}

bool TextFile::is_new() {
  return absolute_path.empty();
}

void TextFile::change_path(const std::string& path) {
  absolute_path = path;
}

void TextFile::load() {
  assert(!absolute_path.empty());
  assert(line_endings != UNKNOWN && line_endings != MIXED);

  std::string utf8_contents;
  {
    std::vector<char> contents = io_provider->read_file(absolute_path);
    if (encoding == "UTF-8") {
      utf8_contents = std::string(contents.data(), contents.size());
    } else {
      utf8_contents = encoding_to_utf8(encoding.c_str(), contents.data(), contents.size());
    }
    if (!utf8_check(utf8_contents.c_str(), utf8_contents.size())) {
      throw EncodingError("Invalid UTF-8 encoding.");
    }
  }

  from_utf8(utf8_contents);

  for (Line& l : lines) {
    l.optimize_size();
  }
}

void TextFile::save(bool trim_trailing_whitespace, bool ignore_unencodable_chars) {
  assert(line_endings != UNKNOWN && line_endings != MIXED);
  assert(!absolute_path.empty());

  // TODO: Where else should it go?
  if (trim_trailing_whitespace) {
    for (Line& l : lines) {
      l.trim_whitespace();
    }
  }

  std::string utf8_contents = to_utf8(line_endings);
  if (encoding == "UTF-8") {
    io_provider->write_file_safe(absolute_path, utf8_contents.data(), utf8_contents.size());
  } else {
    if (!encoding_possible(encoding.c_str(), utf8_contents) && !ignore_unencodable_chars) {
      throw EncodingError("Characters present in this file cannot be encoded in " + encoding + ".");
    }
    std::vector<char> contents = utf8_to_encoding(encoding.c_str(), utf8_contents);
    io_provider->write_file_safe(absolute_path, contents.data(), contents.size());
  }

  unsaved_edits = false;
  undo_manager.add_save_point();
}