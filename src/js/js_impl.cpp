static duk_ret_t wrapper_duk_addOverlay(duk_context* ctx) {
  int arg0 =  duk_require_int(ctx, 0);
  int arg1 =  duk_require_int(ctx, 1);
  int arg2 =  duk_require_int(ctx, 2);
  int arg3 =  duk_require_int(ctx, 3);
  const char* arg4 =  duk_require_string(ctx, 4);
   duk_addOverlay( arg0 , arg1 , arg2 , arg3 , arg4 ) ;
  return 0;
}

static duk_ret_t wrapper_duk_clearOverlays(duk_context* ctx) {
  int arg0 =  duk_require_int(ctx, 0);
   duk_clearOverlays( arg0 ) ;
  return 0;
}

static duk_ret_t wrapper_duk_clearPluginMenu(duk_context* ctx) {
   duk_clearPluginMenu( ) ;
  return 0;
}

static duk_ret_t wrapper_duk_docChar(duk_context* ctx) {
  int arg0 =  duk_require_int(ctx, 0);
  int arg1 =  duk_require_int(ctx, 1);
   duk_docChar( arg0 , arg1 ) ;
  return 0;
}

static duk_ret_t wrapper_duk_docCopy(duk_context* ctx) {
  int arg0 =  duk_require_int(ctx, 0);
  duk_push_string(ctx,  duk_docCopy( arg0 ) .c_str() ) ;
  return 1;
}

static duk_ret_t wrapper_duk_docCut(duk_context* ctx) {
  int arg0 =  duk_require_int(ctx, 0);
  duk_push_string(ctx,  duk_docCut( arg0 ) .c_str() ) ;
  return 1;
}

static duk_ret_t wrapper_duk_docMove(duk_context* ctx) {
  int arg0 =  duk_require_int(ctx, 0);
  int arg1 =  duk_require_int(ctx, 1);
  int arg2 =  duk_require_int(ctx, 2);
  bool arg3 =  duk_require_boolean(ctx, 3);
  bool arg4 =  duk_require_boolean(ctx, 4);
   duk_docMove( arg0 , arg1 , arg2 , arg3 , arg4 ) ;
  return 0;
}

static duk_ret_t wrapper_duk_docPaste(duk_context* ctx) {
  int arg0 =  duk_require_int(ctx, 0);
  const char* arg1 =  duk_require_string(ctx, 1);
   duk_docPaste( arg0 , arg1 ) ;
  return 0;
}

static duk_ret_t wrapper_duk_feedback(duk_context* ctx) {
  const char* arg0 =  duk_require_string(ctx, 0);
  const char* arg1 =  duk_require_string(ctx, 1);
   duk_feedback( arg0 , arg1 ) ;
  return 0;
}

static duk_ret_t wrapper_duk_getCurrentDoc(duk_context* ctx) {
  duk_push_int(ctx,  duk_getCurrentDoc( ) ) ;
  return 1;
}

static duk_ret_t wrapper_duk_getDocLine(duk_context* ctx) {
  int arg0 =  duk_require_int(ctx, 0);
  int arg1 =  duk_require_int(ctx, 1);
  duk_push_string(ctx,  duk_getDocLine( arg0 , arg1 ) .c_str() ) ;
  return 1;
}

static duk_ret_t wrapper_duk_getDocNumLines(duk_context* ctx) {
  int arg0 =  duk_require_int(ctx, 0);
  duk_push_int(ctx,  duk_getDocNumLines( arg0 ) ) ;
  return 1;
}

static duk_ret_t wrapper_getDocType(duk_context* ctx) {
  int arg0 =  duk_require_int(ctx, 0);
  duk_push_string(ctx,  getDocType( arg0 ) .c_str() ) ;
  return 1;
}

static duk_ret_t wrapper_duk_getDocSelectedText(duk_context* ctx) {
  int arg0 =  duk_require_int(ctx, 0);
  duk_push_string(ctx,  duk_getDocSelectedText( arg0 ) .c_str() ) ;
  return 1;
}

static duk_ret_t wrapper_duk_getDocText(duk_context* ctx) {
  int arg0 =  duk_require_int(ctx, 0);
  duk_push_string(ctx,  duk_getDocText( arg0 ) .c_str() ) ;
  return 1;
}

static duk_ret_t wrapper_duk_getShellDoc(duk_context* ctx) {
  duk_push_int(ctx,  duk_getShellDoc( ) ) ;
  return 1;
}

static duk_ret_t wrapper_duk_goTo(duk_context* ctx) {
  int arg0 =  duk_require_int(ctx, 0);
  int arg1 =  duk_require_int(ctx, 1);
  int arg2 =  duk_require_int(ctx, 2);
   duk_goTo( arg0 , arg1 , arg2 ) ;
  return 0;
}

static duk_ret_t wrapper_duk_ioReadFile(duk_context* ctx) {
  const char* arg0 =  duk_require_string(ctx, 0);
  duk_push_string(ctx,  duk_ioReadFile( arg0 ) .c_str() ) ;
  return 1;
}

static duk_ret_t wrapper_duk_ioRemoveFile(duk_context* ctx) {
  const char* arg0 =  duk_require_string(ctx, 0);
   duk_ioRemoveFile( arg0 ) ;
  return 0;
}

static duk_ret_t wrapper_duk_ioTempFile(duk_context* ctx) {
  duk_push_string(ctx,  duk_ioTempFile( ) .c_str() ) ;
  return 1;
}

static duk_ret_t wrapper_duk_ioWriteFile(duk_context* ctx) {
  const char* arg0 =  duk_require_string(ctx, 0);
  const char* arg1 =  duk_require_string(ctx, 1);
   duk_ioWriteFile( arg0 , arg1 ) ;
  return 0;
}

static duk_ret_t wrapper_duk_promptText(duk_context* ctx) {
  const char* arg0 =  duk_require_string(ctx, 0);
  const char* arg1 =  duk_require_string(ctx, 1);
  const char* arg2 =  duk_require_string(ctx, 2);
  duk_push_string(ctx,  duk_promptText( arg0 , arg1 , arg2 ) .c_str() ) ;
  return 1;
}

void init_wrappers(duk_context* ctx) {
  duk_push_global_object(ctx);
  duk_pop(ctx);
  duk_push_global_object(ctx);
  duk_push_string(ctx, "Syn");
  duk_push_object(ctx);
  duk_put_prop(ctx, -3);
  duk_pop(ctx);
  duk_get_global_string(ctx, "Syn");
  duk_push_c_function(ctx, wrapper_duk_addOverlay, 5);
  duk_put_prop_string(ctx, -2, "addOverlay");
  duk_push_c_function(ctx, wrapper_duk_clearOverlays, 1);
  duk_put_prop_string(ctx, -2, "clearOverlays");
  duk_push_c_function(ctx, wrapper_duk_clearPluginMenu, 0);
  duk_put_prop_string(ctx, -2, "clearPluginMenu");
  duk_push_c_function(ctx, wrapper_duk_docChar, 2);
  duk_put_prop_string(ctx, -2, "docChar");
  duk_push_c_function(ctx, wrapper_duk_docCopy, 1);
  duk_put_prop_string(ctx, -2, "docCopy");
  duk_push_c_function(ctx, wrapper_duk_docCut, 1);
  duk_put_prop_string(ctx, -2, "docCut");
  duk_push_c_function(ctx, wrapper_duk_docMove, 5);
  duk_put_prop_string(ctx, -2, "docMove");
  duk_push_c_function(ctx, wrapper_duk_docPaste, 2);
  duk_put_prop_string(ctx, -2, "docPaste");
  duk_push_c_function(ctx, wrapper_duk_feedback, 2);
  duk_put_prop_string(ctx, -2, "feedback");
  duk_push_c_function(ctx, wrapper_duk_getCurrentDoc, 0);
  duk_put_prop_string(ctx, -2, "getCurrentDoc");
  duk_push_c_function(ctx, wrapper_duk_getDocLine, 2);
  duk_put_prop_string(ctx, -2, "getDocLine");
  duk_push_c_function(ctx, wrapper_duk_getDocNumLines, 1);
  duk_put_prop_string(ctx, -2, "getDocNumLines");
  duk_push_c_function(ctx, wrapper_getDocType, 1);
  duk_put_prop_string(ctx, -2, "getDocType");
  duk_push_c_function(ctx, wrapper_duk_getDocSelectedText, 1);
  duk_put_prop_string(ctx, -2, "getDocSelectedText");
  duk_push_c_function(ctx, wrapper_duk_getDocText, 1);
  duk_put_prop_string(ctx, -2, "getDocText");
  duk_push_c_function(ctx, wrapper_duk_getShellDoc, 0);
  duk_put_prop_string(ctx, -2, "getShellDoc");
  duk_push_c_function(ctx, wrapper_duk_goTo, 3);
  duk_put_prop_string(ctx, -2, "goTo");
  duk_push_c_function(ctx, wrapper_duk_ioReadFile, 1);
  duk_put_prop_string(ctx, -2, "ioReadFile");
  duk_push_c_function(ctx, wrapper_duk_ioRemoveFile, 1);
  duk_put_prop_string(ctx, -2, "ioRemoveFile");
  duk_push_c_function(ctx, wrapper_duk_ioTempFile, 0);
  duk_put_prop_string(ctx, -2, "ioTempFile");
  duk_push_c_function(ctx, wrapper_duk_ioWriteFile, 2);
  duk_put_prop_string(ctx, -2, "ioWriteFile");
  duk_push_c_function(ctx, wrapper_duk_promptText, 3);
  duk_put_prop_string(ctx, -2, "promptText");
  duk_pop(ctx);
}
