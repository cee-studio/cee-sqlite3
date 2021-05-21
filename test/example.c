static void 
cache_save_code(uint64_t author_id, uint64_t guild_id, uint64_t code_id, char *code)
{
  struct pair p[] = {
    {.name = "@author_id", .type = INT64, .value = &author_id},
    {.name = "@guild_id", .type = INT64, .value = &guild_id},
    {.name = "@id", .type = INT64, .value = &code_id},
    {.name = "@code", .type = TEXT, .value = code},
    {.name = NULL }
  };

  struct iu iu = {
    .select = "select * from code_snippet where code_id = @id;",
    .update = "update code_snippet set js_code = @code where code_id = @id;",
    .insert = "insert into code_snippet (author_id, guild_id, code_id, js_code) values (@author_id, @guild_id, @id, @code);",
    .pairs = p
  };
  insert_or_update(&iu);
}

static void
cache_channel_type(uint64_t id, int type)
{
  struct pair p[] = {
    {.name = "@id", .type = INT64, .value = &id},
    {.name = "@type", .type = INT, .value = &type},
    {.name = NULL }
  };

  struct iu iu = {
    .select = "select * from channel where id = @id?;",
    .update = "update channel set type = @type where id = @id;",
    .insert = "insert into channel (id, type) values (@id, @type);",
    .pairs = p
  };
  insert_or_update(&iu);
}

static void 
cache_track_eval(uint64_t code_id, uint64_t eval_id, uint64_t user_id)
{
  struct pair p[] = 
  {
    {.name = "@code_id", .type = INT64, .value = &code_id},
    {.name = "@eval_id", .type = INT64, .value = &eval_id},
    {.name = "@user_id", .type = INT64, .value = &user_id},
    {.name = NULL }
  };

  struct iu iu = {
    .select = "select * from eval_tracker where code_id = @code_id and user_id = @user_id;",
    .update = "update eval_tracker set eval_id = @eval_id where code_id = @code_id and user_id = @user_id;",
    .insert = "insert into eval_tracker (code_id, user_id, eval_id) values (@code_id, @user_id, @eval_id);",
    .pairs = p
  };
  insert_or_update(&iu);
}

static bool
cache_get_eval_id(uint64_t code_id,
                  uint64_t user_id,
                  uint64_t *p_eval_id)
{
  struct pair p[] = 
  {
    {.name = "@code_id", .type = INT64, .value = &code_id},
    {.name = "@user_id", .type = INT64, .value = &user_id},
    {.name = NULL }
  };

  char *select = "select eval_id from eval_tracker "
    " where code_id = @code_id and user_id = @user_id"
    ";";

  sqlite3_stmt *res;
  int rc, step;

  step = bind_run_sql(p, select, &res);
  if (step == SQLITE_ROW) {
    *p_eval_id = sqlite3_column_int64(res, 0);
    sqlite3_finalize(res);
    return true;
  }
  else {
    return false;
  }
}


static bool 
cache_get_code_by_code_id(uint64_t code_id, char *buf, size_t buf_size)
{
  sqlite3_stmt *res;
  struct pair p[] = 
  {
    { .name = "@code_id", .type = INT64, .value = &code_id },
    { .name = NULL}
  };

  char *select = "select js_code from code_snippet where code_id = @code_id";
  int step = bind_run_sql(p, select, &res);
  bool ret = false;
  if (step == SQLITE_ROW) {
    int nparam = snprintf(buf, buf_size, "%s", (char*)sqlite3_column_text(res, 0));
    // check the oob
    ret = true;
  }
  sqlite3_finalize(res);
  return ret;
}

static bool 
cache_get_code_by_message_id(uint64_t id, char *buf, size_t buf_size)
{
  sqlite3_stmt *res;
  struct pair p[] =
  {
    {.name = "@id", .type=INT64, .value= &id },
    {.name = NULL}
  };

  char *select = "select js_code from executable_message where message_id = @id";
  bool ret = false;
  int step = bind_run_sql(p, select, &res);
  if (step == SQLITE_ROW) {
    snprintf(buf, buf_size, "%s", (char*)sqlite3_column_text(res, 0));
    ret = true;
  }
  sqlite3_finalize(res);
  return ret;
}
