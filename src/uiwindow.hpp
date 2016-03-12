#ifndef SYNTAXIC_UIWINDOW_HPP
#define SYNTAXIC_UIWINDOW_HPP

#include <memory>
#include <string>

// User input type flags and results too (first 3)
#define UI_YES     1
#define UI_NO      2
#define UI_CANCEL  4
#define UI_WARNING 8
#define UI_OK      16

// User file type flags.
#define UF_OPEN             1
#define UF_MUST_EXIST       2
#define UF_CHANGE_DIR       4
#define UF_SAVE             8
#define UF_OVERWRITE_PROMPT 16

class Doc;
class DocCollection;
class STree;

class UIWindow {
public:
  virtual int get_num_collections() = 0;
  virtual DocCollection* get_collection(int index) = 0;

  virtual void add_document(Doc* doc, bool switch_to_doc) = 0;
  virtual void remove_document(Doc* doc) = 0;
  virtual void go_to_document(Doc* doc) = 0;

  virtual void add_file_provider(STree* fp, bool expand) = 0;
  virtual void remove_file_provider(STree* fp) = 0;

  virtual int get_user_input(const std::string& title, const std::string& text, int type) = 0;
  /** Returns UI_OK if entered, else UI_CANCEL. */
  virtual int get_user_text_input(const std::string& title, const std::string& text, const std::string& entered_text, int type, std::string& result) = 0;
  virtual int get_user_file(std::string& output_file, const std::string& text, const std::string& wildcard, int type) = 0;
  /** Feedback a popup message to the user. */
  virtual void feedback(const std::string& title, const std::string& text) = 0;

  virtual ~UIWindow() {};
};

std::unique_ptr<UIWindow> make_window();

#endif