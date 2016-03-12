#ifndef SYNTAXIC_JS_DEFS_HPP
#define SYNTAXIC_JS_DEFS_HPP

#define DFUNC static inline

// DUKTAPE_NAMESPACE Syn

DFUNC void duk_addOverlay(int handle, int row, int col, int type, const char* text);
DFUNC void duk_clearOverlays(int handle);
DFUNC void duk_clearPluginMenu();
DFUNC void duk_docChar(int handle, int ch);
DFUNC std::string duk_docCopy(int handle);
DFUNC std::string duk_docCut(int handle);
DFUNC void duk_docMove(int handle, int row, int col, bool ctrl, bool shift);
DFUNC void duk_docPaste(int handle, const char* text);
DFUNC void duk_feedback(const char* title, const char* text);
DFUNC int duk_getCurrentDoc();
DFUNC std::string duk_getDocLine(int handle, int row);
DFUNC int duk_getDocNumLines(int handle);
DFUNC std::string getDocType(int handle);
DFUNC std::string duk_getDocSelectedText(int handle);
DFUNC std::string duk_getDocText(int handle);
DFUNC int duk_getShellDoc();
DFUNC void duk_goTo(int handle, int row, int col);
DFUNC std::string duk_ioReadFile(const char* path);
DFUNC void duk_ioRemoveFile(const char* path);
DFUNC std::string duk_ioTempFile();
DFUNC void duk_ioWriteFile(const char* path, const char* contents);
DFUNC std::string duk_promptText(const char* title, const char* text, const char* defaultText);

#endif