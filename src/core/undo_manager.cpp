#include "core/undo_manager.hpp"
#include "core/text_file.hpp"

UndoManager::UndoManager(TextFile& tf) : text_file(tf), counter(0) {}

int UndoManager::new_activity() {
  return ++counter;
}

void UndoManager::add_change_line(int id, int line_index, CursorLocation cl) {
  elements.push_back(UndoElement(UNDO_CHANGE_LINE, line_index,
                     text_file.get_line(line_index).to_string(), id, cl));
}

void UndoManager::add_insert_line(int id, int line_index, CursorLocation cl) {
  elements.push_back(UndoElement(UNDO_INSERT_LINE, line_index,
                     std::string(), id, cl));
}

void UndoManager::add_remove_line(int id, int line_index, CursorLocation cl) {
  elements.push_back(UndoElement(UNDO_REMOVE_LINE, line_index,
                     text_file.get_line(line_index).to_string(), id, cl));
}

void UndoManager::add_save_point() {
  for (int i = elements.size() - 1; i >= 0; i--) {
    UndoElement& ue = elements[i];
    if (ue.get_type() == UNDO_SAVE_POINT) {
      elements.erase(elements.begin() + i);
    }
  }

  elements.push_back(UndoElement(UNDO_SAVE_POINT, -1, "", -1, CursorLocation()));
}

bool UndoManager::is_save_point() {
  if (elements.size() == 0) return false;
  return elements.back().get_type() == UNDO_SAVE_POINT;
}

bool UndoManager::undo(CursorLocation& cl) {
  if (elements.size() == 0) return false;

  // This is the starting save point.  This one cannot be removed except by setting a new save
  // point.
  if (elements.size() == 1 && elements.back().get_type() == UNDO_SAVE_POINT) return false;

  // Ignore save points for purposes of undo.
  while (elements.size() > 0) {
    if (elements.back().get_type() == UNDO_SAVE_POINT) {
      elements.pop_back();
    } else break;
  }

  const int last_count = elements.back().get_count();

  for (int i = elements.size() - 1; i >= 0; i--) {
    const UndoElement& ue = elements[i];

    if (ue.get_count() != last_count) break;
    const int row = ue.get_row();
    switch (ue.get_type()) {
      case UNDO_CHANGE_LINE:
        {
          Line& l = text_file.get_line(row);
          l.trim(0);
          l.append(ue.get_line());
        }
        break;
      case UNDO_INSERT_LINE:
        text_file.remove_line(row);
        break;
      case UNDO_REMOVE_LINE:
        {
          Line& l = text_file.insert_line(row);
          l.append(ue.get_line());
        }
        break;
      case UNDO_SAVE_POINT:
        printf("WARNING: UNDO_SAVE_POINT unexpected\n");
        break;
    }

    cl = ue.get_cursor_before();
    elements.pop_back();
  }

  text_file.unsaved_edits = true;
  if (is_save_point()) text_file.unsaved_edits = false;

  return true;
}
