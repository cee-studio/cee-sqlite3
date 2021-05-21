#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "sqlite3.h"

void cee_sqlite3_init_db(char *dbname, char *stmt[])
{
  sqlite3_stmt *res;

  int rc = sqlite3_open(dbname, &db);
  if (rc != SQLITE_OK) 
  {
    fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    db = NULL;
    return;
  }

  char *sql = "begin transaction;"
    "create table if not exists code_snippet "
    "(author_id int, guild_id int, code_id int, js_code text, title text, keywords text);"
    "create table if not exists eval (code_id int, user_id int, eval_id int);"
    "create table if not exists channel (channel_id int, type int);"
    "create table if not exists eval_tracker (code_id int, user_id int, eval_id int);"
    // link a code to a message
    "create table if not exists executable_message"
    "(guild_id int, channel_id int, message_id int, js_code text, author_id int);"
    // key = guild_id
    "create table if not exists cached_guild (key text, json text);"
    // key = guild_id
    "create table if not exists cached_guild_roles (key text, json text);"
    // key = channel_id
    "create table if not exists cached_channel (key text, json text);"

    // key = guild_id + "_" + user_id
    "create table if not exists cached_guild_member (key text, json text);"
    /*
     *  key: "guild_" + guild_id, "roles_" + guild_id, 
     *       "channel_" + channel_id, 
     *       "member_" + guild_id + "_" + "user_id"
     * 
     *  json: text
     */
    "create table if not exists cached_json (key text, json text);"
    "create table if not exists bot_message (guild_id int, channel_id int, message_id int);"
    "create table if not exists cached_message (channel_id int, message_id int, content text);"
    "commit;"
  ;

  char *err_msg=NULL;
  rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", err_msg);
    sqlite3_free(err_msg);
    sqlite3_close(db);
    db = NULL;
    return;
  }
  return;
}

struct pair {
  char * name;
  enum type {
    INT,
    INT64,
    TEXT
  } type;
  void * value;
};

struct iu {
  char * select;
  char * update;
  char * insert;
  struct pair *pairs;
};


int cee_sqlite3_bind_run_sql(struct pair *pairs, char *sql, sqlite3_stmt **res_p)
{
  sqlite3_stmt *res;
  int rc = sqlite3_prepare_v2(db, sql, -1, res_p, 0);
  res = *res_p;
  int idx = 0;

  if (rc == SQLITE_OK) {
    for(int i = 0; pairs[i].name; i++) {
      idx = sqlite3_bind_parameter_index(res, pairs[i].name);
      if (idx <= 0) continue;
      switch(pairs[i].type) 
      {
        case INT:
          sqlite3_bind_int(res, idx, *(int *)pairs[i].value);
          break;
        case INT64:
          sqlite3_bind_int64(res, idx, *(int64_t *)pairs[i].value);
          break;
        case TEXT:
	  typedef void (*f0)(void *);
          sqlite3_bind_text(res, idx, (char*)pairs[i].value, -1, (f0)0);
          break;
      }
    }
    return sqlite3_step(res);
  }
  else {
    fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
    return rc;
  }
}

void cee_sqlite_3_insert_or_update(struct iu *p)
{
  sqlite3_stmt *res;
  int rc, step;

  sqlite3_exec(db, "begin transaction;", NULL, NULL, NULL);

  step = bind_run_sql(p->pairs, p->select, &res);
  sqlite3_finalize(res);
  if (step == SQLITE_ROW) {
    step = bind_run_sql(p->pairs, p->update, &res);
    sqlite3_finalize(res);
  }
  else {
    step = bind_run_sql(p->pairs, p->insert, &res);
    if (step != SQLITE_DONE)
      fprintf(stderr, "execution failed: %s", sqlite3_errmsg(db));
    sqlite3_finalize(res);
  }
  sqlite3_exec(db, "end transaction;", NULL, NULL, NULL);
}
