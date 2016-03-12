#include "console.hpp"
#include "document.hpp"
#include "core/encoding.hpp"
#include "js_console.hpp"
#include "master.hpp"
#include "master_io_provider.hpp"
#include "process.hpp"
#include "recents.hpp"
#include "qtgui/main_window.hpp"
#include "uiwindow.hpp"
#include "core/util.hpp"
#include "core/utf8_util.hpp"
#include "core/util_path.hpp"

#include "utf8.h"

#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QStringList>

// TODO: Change to a pointer.
Master master;

Master::Master() : main_window(nullptr), markovian(0), markovian_avalanche(0) {}

Master::~Master() {}

void Master::debug() {
  //printf("UIWindows:\n");
  //for (auto& p:
  printf("Documents:\n");
  for (auto& p: documents) {
    printf("  %p\n", (void*) (p.get()));
  }
}

void Master::init_master() {
  recent_projects = std::unique_ptr<Recents>(new Recents("projects", 10));
  recent_files = std::unique_ptr<Recents>(new Recents("files", 10));

  try {
    // Load meta data
    std::vector<char> file_contents;
    read_file(file_contents, under_root("syntaxic_meta.json"));

    // Load themes:
    theme_engine.init_theme_engine(file_contents.data(), file_contents.size());

    // Load statlang meta.
    stat_lang.init(file_contents.data(), file_contents.size());
  } catch (std::exception& e) {
    std::string text = "Error while starting Syntaxic:\n\n";
    text += e.what();
    text += "\n\nPlease reinstall or report a bug to support@kpartite.com.";
    text += "\n\nProgram startup will continue but functionality may be limited.";
    QMessageBox::critical(nullptr, "Failed to initialize program",
        QString::fromStdString(text));
  }

  pref_manager.init();
  reload_settings();

  // TODO: This is temporary
  // Create temporary document
  //temp_doc = std::unique_ptr<Document>(new Document);
}

void Master::add_file_provider(std::unique_ptr<STree> fp, bool expand) {
  fp->set_ui_window(main_window.get());
  main_window->add_file_provider(fp.get(), expand);
  file_providers.push_back(std::move(fp));
  save_file_providers();
}

void Master::remove_file_provider(STree* provider) {
  main_window->remove_file_provider(provider);
  for (unsigned int i = 0; i < file_providers.size(); i++) {
    if (file_providers[i].get() == provider) {
      file_providers.erase(file_providers.begin() + i);
      save_file_providers();
      return;
    }
  }
}

void Master::save_file_providers() {
  std::vector<STreeSave> fpss;
  for (unsigned int i = 0; i < file_providers.size(); i++) {
    fpss.push_back(file_providers[i]->get_save_info());
  }
  settings.set_sidebar_state(fpss);
}

void Master::set_markovian(int m) {
  if (markovian == m) markovian_avalanche++;
  else markovian_avalanche = 1;
  markovian = m;
}

void Master::create_main_window() { main_window = make_window(); }
void Master::add_uiwindow(std::unique_ptr<UIWindow> w) { uiwindows.push_back(std::move(w)); }

void Master::new_document(UIWindow* pref_window) {
  std::unique_ptr<TextFile> tf(new TextFile(master_io_provider));
  std::unique_ptr<Document> doc(new Document(std::move(tf)));
  documents.push_back(std::move(doc));

  // Find pref window
  if (pref_window == nullptr) {
    pref_window = main_window.get();
  }

  pref_window->add_document(documents.back().get(), true);
}

void Master::open_document(const char* p, UIWindow* pref_window, int row, int col) {
  std::string path = UtilPath::to_absolute(p);
  recent_files->add(path);

  if (UtilPath::get_lowercase_extension(path) == ".synproj") {
    open_project(std::string(p));
    return;
  }

  // Check that path is not already open?
  for (auto& doc : documents) {
    Document* document = dynamic_cast<Document*>(doc.get());
    if (document == nullptr) continue;
    std::string doc_path = document->get_text_file()->get_absolute_path();
    if (doc_path == path) {
      doc->get_window()->go_to_document(doc.get());
      if (row > -1 && col > -1) {
        doc->handle_jump_to(row, col);
      }
      return;
    }
  }

  // Find pref_window
  if (pref_window == nullptr) {
    pref_window = main_window.get();
  }

  // Check it's not too big
  bool is_big_file = false;
  try {
    long int size = master_io_provider->get_file_size(path);
    if (size > long(5*1024*1024)) {
      float mbs = size / (1024.0*1024.0);
      int answer = pref_window->get_user_input("Big file", "File '" + path + "' is big (" + std::to_string(mbs) + " MB).\n\nSyntaxic is not currently optimized for opening big files.  Performance may be bad and memory usage may be excessive.\n\nAre you sure you wish to continue opening this file?",
          UI_YES | UI_NO | UI_WARNING);
      if (answer == UI_NO) return;
    }
    if (size > long(pref_manager.get_int("loading.big_file_limit")*1024)) {
      is_big_file = true;
    }
  } catch (IOProviderError& e) {
    pref_window->get_user_input("Error getting file size", e.what(), UI_OK | UI_WARNING);
    return;
  }

  // Look at the last document in pref_window, is it a new document? If so, close it.
  if (documents.size() > 0) {
    Document* last_doc = dynamic_cast<Document*>(documents.back().get());
    if (last_doc != nullptr && !last_doc->is_temporary()) {
      TextFile* last_tf = last_doc->get_text_file();
      if (last_tf->is_new() && !last_tf->has_unsaved_edits()) {
        close_document(last_doc);
      }
    }
  }

  std::unique_ptr<TextFile> tf(new TextFile(master_io_provider));
  tf->change_path(path);
  for (;;) {
    try {
      tf->load();
    } catch (IOProviderError& e) {
      pref_window->get_user_input("Error opening " + path, e.what(), UI_OK | UI_WARNING);
      return;
    } catch (EncodingError& e) {
      MainWindow* main_window = dynamic_cast<MainWindow*>(pref_window);
      if (main_window == nullptr) return;

      QStringList choices;
      for (int i = 0; i < NUM_ENCODINGS; i++) {
        choices << ENCODINGS[i].extended;
      }
      bool ok;

      QString choice = QInputDialog::getItem(main_window, "Encoding error", QString::fromStdString("Error while trying to open '" + path + "':\n\n" + std::string(e.what()) + "\n\nPlease choose another encoding:"), choices, 0, false, &ok);
      if (ok) {
        tf->set_encoding(codec_extended_to_name(choice.toStdString().c_str()));
        // The above triggers unsaved state, so put it back:
        tf->set_unsaved(false);
        continue;
      } else {
        return;
      }
    }
    break;
  }

  std::unique_ptr<Document> doc(new Document(std::move(tf), is_big_file));
  documents.push_back(std::move(doc));
  pref_window->add_document(documents.back().get(), true);
  if (col < 0) col = 0;
  if (row > -1 && col > -1) {
    documents.back()->handle_jump_to(row, col);
  }
}

void Master::open_temp_read_only_document(const std::string& title, const std::string& text) {
  std::unique_ptr<Document> doc = make_temp_read_only_document(title, text);
  documents.push_back(std::move(doc));
  main_window->add_document(documents.back().get(), true);
}

void Master::open_temp_read_only_document(const std::string& title, const TextBuffer& tb) {
  std::unique_ptr<Document> doc = make_temp_read_only_document(title, "");
  // TODO: Ugly: change Doc interface
  TextBuffer* mytb = const_cast<TextBuffer*> (doc->get_text_buffer());
  mytb->from_buffer(tb);
  documents.push_back(std::move(doc));
  main_window->add_document(documents.back().get(), true);
}

void Master::new_shell() {
  std::unique_ptr<Process> process(new Process());
  process->start_shell(".");
  documents.push_back(std::move(process));

  main_window->add_document(documents.back().get(), true);
}

void Master::new_js_console() {
  std::unique_ptr<JSConsole> console(new JSConsole());
  documents.push_back(std::move(console));
  main_window->add_document(documents.back().get(), true);
}

void Master::open_temp_document(UIWindow* pref_window) {
  // Find pref_window
  if (pref_window == nullptr) {
    pref_window = main_window.get();
  }

  if (temp_doc->get_window() != nullptr) {
    if (temp_doc->get_window() != pref_window) {
      // TODO: Close it here
    }
    pref_window->go_to_document(temp_doc.get());
  } else {
    pref_window->add_document(temp_doc.get(), true);
  }

  CursorLocation cl = temp_doc->get_text_view()->get_cursor();
  temp_doc->handle_jump_to(cl.row, cl.col);
}

void Master::save_temp_bookmark(const std::string& abs_path, int row, int col) {
  temp_bookmark.abs_path = abs_path;
  temp_bookmark.row = row;
  temp_bookmark.col = col;
}

void Master::jump_temp_bookmark() {
  if (temp_bookmark.abs_path.empty()) {
    return;
  }
  open_document(temp_bookmark.abs_path.c_str(), nullptr, temp_bookmark.row, temp_bookmark.col);
}

bool Master::save_document(Document* document) {
  try {
    if (document->get_text_file()->is_new()) {
      return save_document_as(document);
    } else {
      document->handle_save();
      return true;
    }
  } catch(TextFileError tfe) {
    std::string what = "Error occured while trying to save: ";
    what += tfe.what();
    document->get_window()->get_user_input(
      "Error saving document", what, UI_OK | UI_WARNING);
    return false;
  } catch(...) {
    document->get_window()->get_user_input(
      "Error saving document", "Uknown error occured while trying to save document.",
      UI_OK | UI_WARNING);
      return false;
  }
}

bool Master::save_document_as(Document* document) {
  UIWindow* window = document->get_window();
  std::string new_path;
  int dialog_result = window->get_user_file(new_path, "Save as...", "*",
      UF_SAVE | UF_OVERWRITE_PROMPT);
  if (dialog_result == UI_CANCEL) return false;
  try {
    document->handle_save_as(new_path);
  } catch(TextFileError tfe) {
    std::string what = "Error occured while trying to save: ";
    what += tfe.what();
    document->get_window()->get_user_input(
      "Error saving document", what, UI_OK | UI_WARNING);
    return false;
  } catch(...) {
    document->get_window()->get_user_input(
      "Error saving document", "Uknown error occured while trying to save document.",
      UI_OK | UI_WARNING);
      return false;
  }
  return true;
}

bool Master::close_document(Doc* doc) {
  UIWindow* window = doc->get_window();

  if (temp_doc.get() == doc) {
    if (window != nullptr) {
      window->remove_document(doc);
      temp_doc->set_window(nullptr);
    }
    return true;
  }

  // TODO: This is legacy, should switch to using handle_about_to_close().
  Document* document = dynamic_cast<Document*>(doc);
  if (document != nullptr) {
    if (document->get_text_file()->has_unsaved_edits()) {
      std::string text = "File '";
      text += document->get_text_file()->get_file_name();
      text += "' has unsaved changes. Do you want to save the file before closing?";
      int dialog_result = window->get_user_input("Save file before closing?", text, UI_YES | UI_NO | UI_CANCEL | UI_WARNING);
      if (dialog_result == UI_YES) {
        if (!save_document(document)) {
          // Saving didn't succeed, don't close the document.
          return false;
        }
      } else if (dialog_result == UI_NO) {
        /* Ignore. */
      } else {
        // Cancel: don't close the document.
        return false;
      }
    }
  } else {
    if (!doc->handle_about_to_close()) return false;
  }

  for (unsigned int i = 0; i < documents.size(); i++) {
    if (documents[i].get() == doc) {
      window->remove_document(doc);
      doc->set_window(nullptr);
      documents.erase(documents.begin() + i);
      break;
    }
  }
  return true;
}

void Master::new_project(ProjectSpec spec) {
  try {
    spec.project_json_path = UtilPath::join_components(spec.root_path, spec.project_name + ".synproj");
    save_project(spec);
    std::unique_ptr<STree> fp(new Project(spec.root_path, spec));
    master.add_file_provider(std::move(fp));
  } catch (std::exception& e) {
    main_window->get_user_input(
      "Error creating project", e.what(), UI_OK | UI_WARNING);
  }
}

void Master::open_project(const std::string& path) {
  try {
    ProjectSpec spec = ::open_project(path);
    recent_projects->add(path);
    std::unique_ptr<STree> fp(new Project(spec.root_path, spec));
    master.add_file_provider(std::move(fp));
  } catch (std::exception& e) {
    main_window->get_user_input(
      "Error opening project", e.what(), UI_OK | UI_WARNING);
  }
}

bool Master::quit() {
  if (tools.size() > 0) {
    feedback("Tools still running", "Terminate tools (by pressing escape).");
    return false;
  }

  for (int i = documents.size()-1; i >= 0; i--) {
    if (!close_document(documents[i].get())) return false;
  }

  settings.save_settings();

  return true;
}

void Master::open_about() {
  open_document(under_root("SYNTAXIC_LICENSE").c_str(), nullptr);
}

void Master::open_welcome() {
  open_document(under_root("SYNTAXIC_WELCOME").c_str(), nullptr);
}

void Master::open_help() {
  open_document(under_root("SYNTAXIC_HELP").c_str(), nullptr);
}

void Master::delete_windows() {
  uiwindows.clear();
  main_window = nullptr;
}

void Master::add_bookmark(const std::string& abs_path, int row, int col) {
  Bookmark bm;
  bm.abs_path = abs_path;
  bm.row = row;
  bm.col = col;
  if (bookmarks.size() > 0) {
    if (bookmarks.front().abs_path == abs_path && bookmarks.front().row == row) {
      bookmarks.front().col = col;
      feedback("Bookmark updated", "Bookmark 1 updated.");
      return;
    }
  }

  bookmarks.insert(bookmarks.begin(), bm);
  if (bookmarks.size() > 10) bookmarks.pop_back();

  std::string text = "Bookmark 1 of " + std::to_string(bookmarks.size()) + " set.";
  feedback("Bookmark set", text);
}

void Master::jump_to_bookmark() {
  if (bookmarks.empty()) return;

  int offset = 0;
  if (markovian == MARKOVIAN_BOOKMARK) offset = markovian_avalanche - 1;
  offset = offset % bookmarks.size();

  const Bookmark& bm = bookmarks[offset];
  open_document(bm.abs_path.c_str(), nullptr, bm.row, bm.col);

  std::string text = "Bookmark " + std::to_string(offset+1) + " of "
      + std::to_string(bookmarks.size()) + ".";
  feedback("Jumped to bookmark", text);
}

void Master::feedback(const std::string& title, const std::string& text) {
  main_window->feedback(title, text);
  for (auto& uiw: uiwindows) {
    uiw->feedback(title, text);
  }
}

void Master::get_known_documents(std::vector<KnownDocument>& known_docs) {
  KnownDocuments kd;
  for (auto& doc: documents) {
    Document* document = dynamic_cast<Document*>(doc.get());
    if (document == nullptr) continue;

    if (document->get_text_file()->is_new()) continue;
    kd.add_document(document->get_text_file()->get_absolute_path());
  }

  for (auto& fp: file_providers) {
    fp->populate_known_documents(&kd);
  }
  known_docs = kd.get();
}

void Master::reload_settings() {
  int tabdef = master.pref_manager.get_tabdef();
  for (auto& d: documents) {
    d->get_appendage().tabdef = tabdef;
  }

  // Reload the keymappers.
  key_mapper_main.clear();

  std::string error_output;
  key_mapper_main.add_key("Left", DocAction::MOVE_LEFT);
  key_mapper_main.add_key("Right", DocAction::MOVE_RIGHT);
  key_mapper_main.add_key("Up", DocAction::MOVE_UP);
  key_mapper_main.add_key("Down", DocAction::MOVE_DOWN);
  error_output += key_mapper_main.add_keys_safe(pref_manager.get_key("keys.skip-left"), DocAction::SKIP_LEFT);
  error_output += key_mapper_main.add_keys_safe(pref_manager.get_key("keys.skip-right"), DocAction::SKIP_RIGHT);
  key_mapper_main.add_key("Ctrl-Up", DocAction::SKIP_UP);
  key_mapper_main.add_key("Ctrl-Down", DocAction::SKIP_DOWN);
  error_output += key_mapper_main.add_keys_safe(pref_manager.get_key("keys.home"), DocAction::MOVE_HOME);
  error_output += key_mapper_main.add_keys_safe(pref_manager.get_key("keys.end"), DocAction::MOVE_END);
  error_output += key_mapper_main.add_keys_safe(pref_manager.get_key("keys.delete"), DocAction::DELETE_FORWARD);
  error_output += key_mapper_main.add_keys_safe(pref_manager.get_key("keys.delete-word"), DocAction::DELETE_WORD);
  error_output += key_mapper_main.add_keys_safe(pref_manager.get_key("keys.home-file"), DocAction::MOVE_HOME_FILE);
  error_output += key_mapper_main.add_keys_safe(pref_manager.get_key("keys.end-file"), DocAction::MOVE_END_FILE);
  key_mapper_main.add_key("PageUp", DocAction::MOVE_PAGE_UP);
  key_mapper_main.add_key("PageDown", DocAction::MOVE_PAGE_DOWN);

  key_mapper_navigation.clear();
  error_output += key_mapper_navigation.add_keys_safe(pref_manager.get_key("keys.navigation.left"), DocAction::MOVE_LEFT);
  error_output += key_mapper_navigation.add_keys_safe(pref_manager.get_key("keys.navigation.right"), DocAction::MOVE_LEFT);
  error_output += key_mapper_navigation.add_keys_safe(pref_manager.get_key("keys.navigation.down"), DocAction::MOVE_LEFT);
  error_output += key_mapper_navigation.add_keys_safe(pref_manager.get_key("keys.navigation.up"), DocAction::MOVE_LEFT);
  error_output += key_mapper_navigation.add_keys_safe(pref_manager.get_key("keys.navigation.page_up"), DocAction::MOVE_LEFT);
  error_output += key_mapper_navigation.add_keys_safe(pref_manager.get_key("keys.navigation.page_down"), DocAction::MOVE_LEFT);
  error_output += key_mapper_navigation.add_keys_safe(pref_manager.get_key("keys.navigation.skip_left"), DocAction::MOVE_LEFT);
  error_output += key_mapper_navigation.add_keys_safe(pref_manager.get_key("keys.navigation.skip_right"), DocAction::MOVE_LEFT);
  key_mapper_navigation.add_key("H", DocAction::MOVE_LEFT);
  key_mapper_navigation.add_key("L", DocAction::MOVE_RIGHT);
  key_mapper_navigation.add_key("J", DocAction::MOVE_DOWN);
  key_mapper_navigation.add_key("K", DocAction::MOVE_UP);
  key_mapper_navigation.add_key("U", DocAction::MOVE_PAGE_UP);
  key_mapper_navigation.add_key("M", DocAction::MOVE_PAGE_DOWN);
  key_mapper_navigation.add_key("G", DocAction::SKIP_LEFT);
  key_mapper_navigation.add_key(";", DocAction::SKIP_RIGHT);
}

void Master::get_past_invokes(std::vector<std::string>& commands) {
  settings.get_invokes(commands);
}

void Master::add_invoke(const std::string& cmd) {
  settings.add_invoke(cmd);
}

void Master::run_tool(const std::string& cmd, bool wants_temp_buffer, const std::string& cwd) {
  if (wants_temp_buffer) {
    for (auto& tool : tools) {
      if (tool->uses_temp_buffer()) {
        feedback("Already running", "There is a tool already running in the temporary buffer.");
        return;
      }
    }
  }

  // Show temp document if it is not shown.
  if (temp_doc->get_window() == nullptr) {
    main_window->add_document(temp_doc.get(), false);
  }

  Document* doc = nullptr;
  if (wants_temp_buffer) doc = temp_doc.get();

  std::unique_ptr<SynTool> tool(new SynTool(cmd, wants_temp_buffer, doc, cwd));
  tools.push_back(std::move(tool));
  tools.back()->start();
}

void Master::tool_finished(SynTool* tool) {
  for (unsigned int i = 0; i < tools.size(); i++) {
    auto& t = tools[i];
    if (t.get() == tool) {
      tools.erase(tools.begin() + i);
      return;
    }
  }
}

void Master::go_to_navigable(const std::string& navigable) {
  if (navigable.empty()) return;

  std::vector<std::string> parts1;
  utf8_string_split(parts1, navigable, ':');

  int row = 0, col = 0;
  if (parts1.size() > 1) {
    if (!parts1[1].empty()) row = std::stoi(parts1[1]) - 1;
  }
  if (parts1.size() > 2) {
    if (!parts1[1].empty()) col = std::stoi(parts1[2]) - 1;
  }

  // See if you can open the file using parts1[0].
  {
    if (QFileInfo::exists(QString::fromStdString(parts1[0]))) {
      open_document(parts1[0].c_str(), nullptr, row, col);
      return;
    }
  }

  // Otherwise, get last component of parts1[0] and find a file that matches it among known files.
  {
    std::string last_comp = last_component(parts1[0]);

    std::vector<KnownDocument> known_docs;
    get_known_documents(known_docs);

    for (KnownDocument& kd: known_docs) {
      if (kd.file_name == last_comp) {
        open_document(kd.abs_path.c_str(), nullptr, row, col);
        return;
      }
    }
  }

  {
    std::string text = "Cannot parse '";
    text += navigable;
    text += "'.";
    feedback("Cannot jump", text);
  }
}

void Master::global_find(const std::string& term) {
  std::vector<KnownDocument> known_docs;
  get_known_documents(known_docs);
  QProgressDialog progress("Global find...", "Cancel", 0, known_docs.size(), dynamic_cast<MainWindow*> (get_main_window()));
  progress.setMinimumDuration(500);
  progress.setWindowModality(Qt::WindowModal);

  TextBuffer output;
  int i = 0;
  int num_errors = 0;
  int num_binary = 0;
  std::string error_text;
  for (KnownDocument& kd: known_docs) {
    progress.setValue(i); i++;
    if (progress.wasCanceled()) return;
    try {
      if (master_io_provider->get_file_size(kd.abs_path) < 1000000) {
        std::vector<char> contents = master_io_provider->read_file(kd.abs_path);
        if (is_binary_file(contents.data(), contents.size())) {
          num_binary++;
          continue;
        }
        TextBuffer tb;
        tb.from_utf8(utf8_convert_best(contents.data(), contents.size()));
        for (int i = 0; i < tb.get_num_lines(); i++) {
          std::string line = tb.get_line(i).to_string();
          size_t found = line.find(term);
          if (found != std::string::npos) {
            output.append(kd.abs_path, 7);
            output.append(std::string(":") + std::to_string(i+1) + ": ");
            output.append(line.substr(0, found), 0);
            output.append(line.substr(found, term.size()), 3);
            output.append(line.substr(found + term.size()) + "\n");
          }
        }
      }
    } catch (std::exception& e) {
      num_errors += 1;
      error_text += "Error while reading '" + kd.abs_path + "': " + e.what() + "\n";
    }
  }

  if (num_binary > 0) {
    output.append(std::to_string(num_binary) + " binary files ignored.\n", 2);
  }
  if (num_errors > 0) {
    output.append(std::to_string(num_errors) + " errors:\n", 0);
    output.append(error_text, 10);
  }
  progress.setValue(known_docs.size());
  open_temp_read_only_document("Search results", output);
}

int Master::js_get_current_doc() {
  MainWindow* mw = dynamic_cast<MainWindow*>(main_window.get());
  Doc* doc = mw->get_active_document();
  int i = 0;
  for (auto& d: documents) {
    if (d.get() == doc) {
      return i;
    }
    i++;
  }
  return -1;
}

int Master::js_get_shell_doc() {
  int i = 0;
  for (auto& d: documents) {
    if (d->get_display_style() & DocFlag::SHELL_THEME) {
      return i;
    }
    i++;
  }
  return -1;
}

Doc* Master::js_get_doc(int handle) {
  if (handle < 0) return nullptr;
  if (handle >= int(documents.size())) return nullptr;
  return documents[handle].get();
}

int Master::js_get_doc_handle(Doc* doc) {
  for (unsigned int i = 0; i < documents.size(); i++) {
    if (documents[i].get() == doc) return i;
  }
  return -1;
}