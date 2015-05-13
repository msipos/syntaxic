// INSTALL INSTRUCTIONS
//
//  1) Click Plugins->Manage plugins...
//  2) Copy following text to the bottom of the file.
//  3) Press F10 to reload.
//
// NOTE: You can customize keyboard shortcut by changing the text below.

Syn.addPluginMenuItem("&Go to line", "Ctrl+.", function() {
  var handle = Syn.getCurrentDoc();
  if (handle < 0) return;
  var line = parseInt(Syn.promptText("Go to line", "Enter line number:", ""));
  Syn.goTo(handle, line, 0);
});
