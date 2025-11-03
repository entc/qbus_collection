#include "qtee_eval.h"

// cape includes
#include <sys/cape_log.h>
#include <fmt/cape_tokenizer.h>

//-----------------------------------------------------------------------------

int qtee_compare (const CapeString left, const CapeString right)
{
  int ret;

  CapeString l_trimmed = cape_str_trim_utf8 (left);
  CapeString r_trimmed = cape_str_trim_utf8 (right);

  if (cape_str_equal (l_trimmed, "EMPTY"))
  {
    ret = cape_str_empty (r_trimmed);
  }
  else if (cape_str_equal (r_trimmed, "EMPTY"))
  {
    ret = cape_str_empty (l_trimmed);
  }
  else if (cape_str_equal (l_trimmed, "VALID"))
  {
    ret = cape_str_not_empty (r_trimmed);
  }
  else if (cape_str_equal (r_trimmed, "VALID"))
  {
    ret = cape_str_not_empty (l_trimmed);
  }
  else
  {
    ret = cape_str_equal (l_trimmed, r_trimmed);
  }

  cape_log_fmt (CAPE_LL_TRACE, "CAPE", "template cmp", "expression: '%s' = '%s' -> %i", l_trimmed, r_trimmed, ret);

  cape_str_del (&l_trimmed);
  cape_str_del (&r_trimmed);

  return ret;
}

//-----------------------------------------------------------------------------

int qtee__evaluate_expression__greater_than (const CapeString left, const CapeString right)
{
  int ret;

  CapeString l_trimmed = cape_str_trim_utf8 (left);
  CapeString r_trimmed = cape_str_trim_utf8 (right);

  char* l_res = NULL;
  char* r_res = NULL;

  // try to convert into double
  double l_d = strtod (l_trimmed, &l_res);
  double r_d = strtod (r_trimmed, &r_res);

  if (l_res && r_res)
  {
    // do a mathematical comparison
    ret = l_d > r_d;

    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "template cmp", "expression [double]: %f > %f -> %i,   [orignal]: '%s' > '%s'", l_d, r_d, ret, l_trimmed, r_trimmed);
  }
  else
  {
    // do a textual comparision
    ret = cape_str_compare_c (l_trimmed, r_trimmed) > 0;

    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "template cmp", "expression [text]: '%s' > '%s' -> %i", l_trimmed, r_trimmed, ret);
  }

  cape_str_del (&l_trimmed);
  cape_str_del (&r_trimmed);

  return ret;
}

//-----------------------------------------------------------------------------

int qtee__evaluate_expression__smaller_than (const CapeString left, const CapeString right)
{
  int ret;

  CapeString l_trimmed = cape_str_trim_utf8 (left);
  CapeString r_trimmed = cape_str_trim_utf8 (right);

  char* l_res = NULL;
  char* r_res = NULL;

  // try to convert into double
  double l_d = strtod (l_trimmed, &l_res);
  double r_d = strtod (r_trimmed, &r_res);

  if (l_res && r_res)
  {
    // do a mathematical comparison
    ret = l_d < r_d;

    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "template cmp", "expression [double]: %f < %f -> %i,   [orignal]: '%s' < '%s'", l_d, r_d, ret, l_trimmed, r_trimmed);
  }
  else
  {
    // do a textual comparision
    ret = cape_str_compare_c (l_trimmed, r_trimmed) < 0;

    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "template cmp", "expression [text]: '%s' < '%s' -> %i", l_trimmed, r_trimmed, ret);
  }

  cape_str_del (&l_trimmed);
  cape_str_del (&r_trimmed);

  return ret;
}

//-----------------------------------------------------------------------------

CapeList qtee__evaluate_expression__list (const CapeString source)
{
  CapeList ret = NULL;

  number_t pos;
  number_t len;

  if (cape_str_next (source, '[', &pos) && cape_str_next (source, ']', &len))
  {
    ret = cape_tokenizer_buf (source + pos + 1, len - pos - 1, ',');
  }

  return ret;
}

//-----------------------------------------------------------------------------

int qtee__evaluate_expression__in (const CapeString left, const CapeString right)
{
  int ret = FALSE;

  CapeString l_trimmed = cape_str_trim_utf8 (left);
  CapeString r_trimmed = cape_str_trim_utf8 (right);

  CapeList right_list = qtee__evaluate_expression__list (r_trimmed);

  if (right_list)
  {
    CapeListCursor* cursor = cape_list_cursor_create (right_list, CAPE_DIRECTION_FORW);

    while (cape_list_cursor_next (cursor))
    {
      CapeString h = cape_str_trim_utf8 (cape_list_node_data (cursor->node));

      ret = ret || cape_str_equal (l_trimmed, h);

      cape_log_fmt (CAPE_LL_TRACE, "CAPE", "template cmp", "expression [left in right]: %s in %s -> %i", l_trimmed, h, ret);

      cape_str_del (&h);
    }

    cape_list_cursor_destroy (&cursor);
  }

  cape_str_del (&l_trimmed);
  cape_str_del (&r_trimmed);

  cape_list_del (&right_list);
  return ret;
}

//-----------------------------------------------------------------------------

int qtee__evaluate_expression__single (const CapeString expression)
{
  int ret = FALSE;

  // local objects
  CapeString left = NULL;
  CapeString right = NULL;

  //printf ("EXPR SINGLE: '%s'\n", expression);

  // find the '=' in the expression
  if (cape_tokenizer_split (expression, '=', &left, &right))
  {
    ret = qtee_compare (left, right);
  }
  else if (cape_tokenizer_split (expression, '>', &left, &right))
  {
    ret = qtee__evaluate_expression__greater_than (left, right);
  }
  else if (cape_tokenizer_split (expression, '<', &left, &right))
  {
    ret = qtee__evaluate_expression__smaller_than (left, right);
  }
  else if (cape_tokenizer_split (expression, 'I', &left, &right))
  {
    ret = qtee__evaluate_expression__in (left, right);
  }
  else
  {
    ret = cape_str_not_empty (expression);
  }

  cape_str_del (&left);
  cape_str_del (&right);

  return ret;
}

//-----------------------------------------------------------------------------

int qtee__evaluate_expression_not (const CapeString expression)
{
  if (cape_str_begins (expression, "NOT "))
  {
    return !qtee__evaluate_expression__single (expression + 4);
  }
  else
  {
    return qtee__evaluate_expression__single (expression);
  }
}

//-----------------------------------------------------------------------------

int qtee__evaluate_expression_or (const CapeString expression)
{
  int ret = FALSE;

  // local objects
  CapeListCursor* cursor = NULL;
  CapeList logical_parts = cape_tokenizer_str (expression, " OR ");

  //printf ("EXPR OR: '%s'\n", expression);

  if (cape_list_size (logical_parts))
  {
    cursor = cape_list_cursor_create (logical_parts, CAPE_DIRECTION_FORW);
    while (cape_list_cursor_next (cursor))
    {
      ret = qtee__evaluate_expression_not (cape_list_node_data (cursor->node));
      if (ret == TRUE)
      {
        goto exit_and_cleanup;
      }
    }
  }
  else
  {
    ret = qtee__evaluate_expression_not (expression);
  }

exit_and_cleanup:

  cape_list_cursor_destroy (&cursor);
  cape_list_del (&logical_parts);

  return ret;
}

//-----------------------------------------------------------------------------

int qtee__evaluate_expression_and (const CapeString expression)
{
  int ret = TRUE;

  // local objects
  CapeListCursor* cursor = NULL;
  CapeList logical_parts = cape_tokenizer_str (expression, " AND ");

  //printf ("EXPR AND: '%s'\n", expression);

  if (cape_list_size (logical_parts))
  {
    cursor = cape_list_cursor_create (logical_parts, CAPE_DIRECTION_FORW);
    while (cape_list_cursor_next (cursor))
    {
      ret = qtee__evaluate_expression_or (cape_list_node_data (cursor->node));
      if (ret == FALSE)
      {
        goto exit_and_cleanup;
      }
    }
  }
  else
  {
    ret = qtee__evaluate_expression_or (expression);
  }

exit_and_cleanup:

  cape_list_cursor_destroy (&cursor);
  cape_list_del (&logical_parts);

  return ret;
}

//-----------------------------------------------------------------------------

int __STDCALL qtee_eval__on_text (void* ptr, const char* bufdat, number_t buflen)
{
  cape_stream_append_buf (ptr, bufdat, buflen);

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int qtee_eval_b (const CapeString s, CapeUdc node, int* p_ret, fct_cape_template__on_pipe on_pipe, CapeErr err)
{
  int res;

  // local objects
  CapeTemplate tmpl = cape_template_new ();
  CapeStream stream = cape_stream_new ();

  res = cape_template_compile_str (tmpl, s, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = cape_template_apply (tmpl, node, stream, qtee_eval__on_text, NULL, on_pipe, NULL, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  *p_ret = qtee__evaluate_expression_and (cape_stream_get (stream));

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  cape_template_del (&tmpl);
  cape_stream_del (&stream);

  return res;
}

//-----------------------------------------------------------------------------
