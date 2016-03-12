#ifndef SYNTAXIC_CORE_TEXT_FILE_HPP
#define SYNTAXIC_CORE_TEXT_FILE_HPP

#include "core/common.hpp"
#include "core/line.hpp"
#include "core/text_buffer.hpp"
#include "core/undo_manager.hpp"

#include <stdexcept>
#include <string>
#include <vector>

class IOProvider;
class Line;
class SimpleTextEdit;
struct Character;

class TextFileError : public std::runtime_error {
public:
  TextFileError(const std::string& s) : std::runtime_error(s) {}
};

/** Any temporary accesses to the Line objects must be temporary. Line objects may not be kept. */
class TextFile : public TextBuffer {
  friend class SimpleTextEdit;
  friend class UndoManager;

private:
  std::string absolute_path;
  LineEndings line_endings;
  bool unsaved_edits;
  UndoManager undo_manager;
  std::string encoding;
  IOProvider* io_provider;

public:
  /** Create a new text file with no path specified for now. */
  TextFile(IOProvider* iop);
  TextFile(const TextFile&) = delete;
  TextFile& operator=(const TextFile&) = delete;

  // File metadata

  inline bool has_unsaved_edits() { return unsaved_edits; }
  inline void set_unsaved(bool b) { unsaved_edits = b; }
  /** Is file untitled? */
  bool is_new();
  /** Used with "save as.." */
  void change_path(const std::string& new_path);
  /** Last component of the filename. */
  std::string get_file_name();
  /** Extension including the dot if any. */
  std::string get_extension();
  inline std::string get_absolute_path() { return absolute_path; }

  // Encoding
  inline std::string get_encoding() { return encoding; }
  inline void set_encoding(const std::string& enc) { if (enc != encoding) unsaved_edits = true; encoding = enc; }

  // Input/output
  void save(bool trim_trailing_whitespace=false, bool ignore_unencodable_chars=false);
  /** Load trims the former contents of the text buffer.  */
  void load();

  // Undo manager:
  inline UndoManager& get_undo_manager() { return undo_manager; }
};

#endif