#include <config.h>
#include <libhiha/libhiha.h>

static void
handler (void *state, buffered_token_getter_t getter,
         pratt_tables_t tables, token_t tok, void **lhs,
         const char **error_message)
{
  if (*error_message == NULL)
    *lhs = (void *) tok;
}

HIHA_VISIBLE void
plugin_init (void)
{
  pratt_tables_t tables = lexical_pratt_tables ();
  pratt_nud_put (tables, make_string_t ("R10"), &handler);
}
