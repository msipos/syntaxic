// INSTALL INSTRUCTIONS
//
//  1) Click Plugins->Manage plugins...
//  2) Copy following text to the bottom of the file.
//  3) Press F10 to reload.
//
// NOTE: You can customize keyboard shortcut by changing the text below.

Syn.addPluginMenuItem("To &Lowercase", "Ctrl+Shift+U", function() {
  var handle = Syn.getCurrentDoc();
  if (handle >= 0) {
    // Get selected text, lowercase it and paste it back in.
    var text = Syn.getDocSelectedText(handle);
    Syn.docPaste(handle, text.toLowerCase());
  }
});
