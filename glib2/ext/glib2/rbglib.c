/* -*- c-file-style: "ruby"; indent-tabs-mode: nil -*- */
/*
 *  Copyright (C) 2011  Ruby-GNOME2 Project Team
 *  Copyright (C) 2002,2003  Masahiro Sakai
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA  02110-1301  USA
 */

#include <locale.h>
#include "rbgprivate.h"
#include "rbglib.h"

#define RG_TARGET_NAMESPACE mGLib

static ID id_inspect;

VALUE RG_TARGET_NAMESPACE;

const gchar *
rbg_rval2cstr(VALUE *str)
{
    StringValue(*str);

#ifdef HAVE_RUBY_ENCODING_H
    if (rb_enc_get(*str) != rb_utf8_encoding())
        *str = rb_str_export_to_enc(*str, rb_utf8_encoding());
#endif

    return RSTRING_PTR(*str);
}

const gchar *
rbg_rval_inspect(VALUE object)
{
    VALUE inspected = rb_funcall(object, id_inspect, 0);

    return RVAL2CSTR(inspected);
}

char *
rbg_string_value_ptr(volatile VALUE *ptr)
{
    return rb_string_value_ptr(ptr);
}

const gchar *
rbg_rval2cstr_accept_nil(VALUE *str)
{
    return NIL_P(*str) ? NULL : RVAL2CSTR(*str);
}

/* TODO: How do we deal with encodings? */
const gchar *
rbg_rval2cstr_accept_symbol(volatile VALUE *value)
{
    if (!SYMBOL_P(*value))
        return rbg_rval2cstr((VALUE *)value);

    return rb_id2name(SYM2ID(*value));
}

const gchar *
rbg_rval2cstr_accept_symbol_accept_nil(volatile VALUE *value)
{
    return NIL_P(*value) ? NULL : rbg_rval2cstr_accept_symbol(value);
}

VALUE
rbg_cstr2rval(const gchar *str)
{
    return str != NULL ? CSTR2RVAL_LEN(str, strlen(str)) : Qnil;
}

VALUE
rbg_cstr2rval_len(const gchar *str, gsize len)
{
    if (str == NULL)
        return Qnil;

#ifdef HAVE_RUBY_ENCODING_H
    return rb_external_str_new_with_enc(str, len, rb_utf8_encoding());
#else
    return rb_str_new(str, len);
#endif
}

struct rbg_cstr2rval_len_free_args {
    gchar *str;
    gsize len;
};

static VALUE
rbg_cstr2rval_len_free_body(VALUE value)
{
    struct rbg_cstr2rval_len_free_args *args = (struct rbg_cstr2rval_len_free_args *)value;

    return CSTR2RVAL_LEN(args->str, args->len);
}

static VALUE
rbg_cstr2rval_len_free_ensure(VALUE str)
{
    g_free((gchar *)str);

    return Qnil;
}

VALUE
rbg_cstr2rval_len_free(gchar *str, gsize len)
{
    struct rbg_cstr2rval_len_free_args args = { str, len };

    return str != NULL ? rb_ensure(rbg_cstr2rval_len_free_body, (VALUE)&args,
                                   rbg_cstr2rval_len_free_ensure, (VALUE)str) : Qnil;
}

VALUE
rbg_cstr2rval_with_encoding(const gchar *str, const gchar *encoding)
{
    return str != NULL ? CSTR2RVAL_LEN_ENC(str, strlen(str), encoding) : Qnil;
}

#ifdef HAVE_RUBY_ENCODING_H
VALUE
rbg_cstr2rval_len_with_encoding(const gchar *str, gsize len,
                                const gchar *encoding)
{
    if (str == NULL)
        return Qnil;

    return rb_external_str_new_with_enc(str, len,
                                        encoding != NULL ?
                                            rb_enc_find(encoding) :
                                            rb_utf8_encoding());
}
#else
VALUE
rbg_cstr2rval_len_with_encoding(const gchar *str, gsize len,
                                G_GNUC_UNUSED const gchar *encoding)
{
    if (str == NULL)
        return Qnil;

    return rb_str_new(str, len);
}
#endif

static VALUE
rbg_cstr2rval_free_body(VALUE str)
{
    return CSTR2RVAL((const gchar *)str);
}

static VALUE
rbg_cstr2rval_free_ensure(VALUE str)
{
    g_free((gchar *)str);

    return Qnil;
}

VALUE
rbg_cstr2rval_free(gchar *str)
{
    return str != NULL? rb_ensure(rbg_cstr2rval_free_body, (VALUE)str,
                                  rbg_cstr2rval_free_ensure, (VALUE)str) : Qnil;
}

/* just for backward compatibility. */
VALUE
rbg_cstr2rval_with_free(gchar *str)
{
    return rbg_cstr2rval_free(str);
}

#ifdef HAVE_RUBY_ENCODING_H
static rb_encoding *filename_encoding_if_not_utf8;
#endif

#ifdef HAVE_RUBY_ENCODING_H
static VALUE
rbg_filename_to_ruby_body(VALUE filename)
{
    const gchar *filename_utf8 = (const gchar *)filename;
    VALUE rb_filename;

    rb_filename = rb_external_str_new_with_enc(filename_utf8,
                                               strlen(filename_utf8),
                                               rb_utf8_encoding());

    /* if needed, change encoding of Ruby String to filename encoding, so that
       upcoming File operations will work properly */
    return filename_encoding_if_not_utf8 != NULL ?
        rb_str_export_to_enc(rb_filename, filename_encoding_if_not_utf8) :
        rb_filename;
}

static VALUE
rbg_filename_to_ruby_ensure(VALUE filename)
{
    g_free((gchar *)filename);

    return Qnil;
}
#endif

VALUE
rbg_filename_to_ruby(const gchar *filename)
{
#ifdef HAVE_RUBY_ENCODING_H
    gchar *filename_utf8;
    gsize written;
    GError *error = NULL;

    if (filename == NULL)
        return Qnil;

    if (filename_encoding_if_not_utf8 == NULL)
        return CSTR2RVAL(filename);

    filename_utf8 = g_filename_to_utf8(filename, -1, NULL, &written, &error);
    if (error != NULL)
        RAISE_GERROR(error);

    return rb_ensure(rbg_filename_to_ruby_body, (VALUE)filename_utf8,
                     rbg_filename_to_ruby_ensure, (VALUE)filename_utf8);
#else
    return CSTR2RVAL(filename);
#endif
}

VALUE
rbg_filename_to_ruby_free(gchar *filename)
{
#ifdef HAVE_RUBY_ENCODING_H
    gchar *filename_utf8;
    gsize written;

    if (filename == NULL)
        return Qnil;

    /* convert filename to UTF-8 if needed */
    if (filename_encoding_if_not_utf8 != NULL) {
        GError *error = NULL;

        filename_utf8 = g_filename_to_utf8(filename, -1, NULL, &written, &error);
        g_free(filename);
        if (error != NULL)
            RAISE_GERROR(error);
    } else {
        filename_utf8 = filename;
    }

    return rb_ensure(rbg_filename_to_ruby_body, (VALUE)filename_utf8,
                     rbg_filename_to_ruby_ensure, (VALUE)filename_utf8);
#else
    return CSTR2RVAL_FREE(filename);
#endif
}

gchar *
rbg_filename_from_ruby(VALUE filename)
{
#ifdef HAVE_RUBY_ENCODING_H
    gchar *retval;
    gsize written;
    GError *error = NULL;

    /* if needed, change encoding of Ruby String to UTF-8 */
    StringValue(filename);
    if (rb_enc_get(filename) != rb_utf8_encoding())
        filename = rb_str_export_to_enc(filename, rb_utf8_encoding());

    /* convert it to filename encoding if needed */
    if (filename_encoding_if_not_utf8 == NULL)
        return g_strdup(RSTRING_PTR(filename));

    retval = g_filename_from_utf8(RSTRING_PTR(filename), -1, NULL, &written, &error);
    if (error != NULL)
        RAISE_GERROR(error);

    return retval;
#else
    return g_strdup(RVAL2CSTR(filename));
#endif
}

static VALUE
rbg_filename_gslist_to_array_free_body(VALUE list)
{
    VALUE ary = rb_ary_new();
    GSList *p;

    for (p = (GSList *)list; p != NULL; p = g_slist_next(p))
        rb_ary_push(ary, CSTRFILENAME2RVAL(p->data));

    return ary;
}

static VALUE
rbg_filename_gslist_to_array_free_ensure(VALUE val)
{
    GSList *list = (GSList *)val;
    GSList *p;

    for (p = list; p != NULL; p = g_slist_next(p))
        g_free((gchar *)p->data);

    g_slist_free(list);

    return Qnil;
}

VALUE
rbg_filename_gslist_to_array_free(GSList *list)
{
    return rb_ensure(rbg_filename_gslist_to_array_free_body, (VALUE)list,
                     rbg_filename_gslist_to_array_free_ensure, (VALUE)list);
}

struct rval2strv_args {
    VALUE ary;
    long n;
    const gchar **result;
};

static VALUE
rbg_rval2strv_body(VALUE value)
{
    long i;
    struct rval2strv_args *args = (struct rval2strv_args *)value;

    for (i = 0; i < args->n; i++)
        args->result[i] = RVAL2CSTR(RARRAY_PTR(args->ary)[i]);
    args->result[args->n] = NULL;

    return Qnil;
}

static G_GNUC_NORETURN VALUE
rbg_rval2strv_rescue(VALUE value)
{
    g_free(((struct rval2strv_args *)value)->result);

    rb_exc_raise(rb_errinfo());
}

const gchar **
rbg_rval2strv(volatile VALUE *value, long *n)
{
    struct rval2strv_args args;

    args.ary = *value = rb_ary_dup(rb_ary_to_ary(*value));
    args.n = RARRAY_LEN(args.ary);
    args.result = g_new(const gchar *, args.n + 1);

    rb_rescue(rbg_rval2strv_body, (VALUE)&args,
              rbg_rval2strv_rescue, (VALUE)&args);

    if (n != NULL)
        *n = args.n;

    return args.result;
}

const gchar **
rbg_rval2strv_accept_nil(volatile VALUE *value, long *n)
{
    if (!NIL_P(*value))
        return rbg_rval2strv(value, n);

    if (n != NULL)
        *n = 0;

    return NULL;
}

struct rval2strv_dup_args {
    VALUE ary;
    long n;
    gchar **result;
};

static VALUE
rbg_rval2strv_dup_body(VALUE value)
{
    long i;
    struct rval2strv_dup_args *args = (struct rval2strv_dup_args *)value;

    for (i = 0; i < args->n; i++)
        args->result[i] = g_strdup(RVAL2CSTR(RARRAY_PTR(args->ary)[i]));
    args->result[args->n] = NULL;

    return Qnil;
}

static G_GNUC_NORETURN VALUE
rbg_rval2strv_dup_rescue(VALUE value)
{
    g_free(((struct rval2strv_dup_args *)value)->result);

    rb_exc_raise(rb_errinfo());
}

gchar **
rbg_rval2strv_dup(volatile VALUE *value, long *n)
{
    struct rval2strv_dup_args args;

    args.ary = *value = rb_ary_dup(rb_ary_to_ary(*value));
    args.n = RARRAY_LEN(args.ary);
    args.result = g_new(gchar *, args.n + 1);

    rb_rescue(rbg_rval2strv_dup_body, (VALUE)&args,
              rbg_rval2strv_dup_rescue, (VALUE)&args);

    if (n != NULL)
        *n = args.n;

    return args.result;
}

gchar **
rbg_rval2strv_dup_accept_nil(volatile VALUE *value, long *n)
{
    if (!NIL_P(*value))
        rbg_rval2strv_dup(value, n);

    if (n != NULL)
        *n = 0;

    return NULL;
}

VALUE
rbg_strv2rval(const gchar **strings)
{
    VALUE ary;
    const gchar **p;

    if (strings == NULL)
        return Qnil;

    ary = rb_ary_new();
    for (p = strings; *p != NULL; p++)
        rb_ary_push(ary, CSTR2RVAL(*p));

    return ary;
}

static VALUE
rbg_strv2rval_free_body(VALUE strings)
{
    return STRV2RVAL((const gchar **)strings);
}

static VALUE
rbg_strv2rval_free_ensure(VALUE strings)
{
    g_strfreev((gchar **)strings);

    return Qnil;
}

VALUE
rbg_strv2rval_free(gchar **strings)
{
    return rb_ensure(rbg_strv2rval_free_body, (VALUE)strings,
                     rbg_strv2rval_free_ensure, (VALUE)strings);
}

struct rbg_rval2gbooleans_args {
    VALUE ary;
    long n;
    gboolean *result;
};

static VALUE
rbg_rval2gbooleans_body(VALUE value)
{
    long i;
    struct rbg_rval2gbooleans_args *args = (struct rbg_rval2gbooleans_args *)value;

    for (i = 0; i < args->n; i++)
        args->result[i] = RVAL2CBOOL(RARRAY_PTR(args->ary)[i]);

    return Qnil;
}

static G_GNUC_NORETURN VALUE
rbg_rval2gbooleans_rescue(VALUE value)
{
    g_free(((struct rbg_rval2gbooleans_args *)value)->result);

    rb_exc_raise(rb_errinfo());
}

gboolean *
rbg_rval2gbooleans(volatile VALUE *value, long *n)
{
    struct rbg_rval2gbooleans_args args;

    args.ary = *value = rb_ary_dup(rb_ary_to_ary(*value));
    args.n = RARRAY_LEN(args.ary);
    args.result = g_new(gboolean, args.n + 1);

    rb_rescue(rbg_rval2gbooleans_body, (VALUE)&args,
              rbg_rval2gbooleans_rescue, (VALUE)&args);

    *n = args.n;

    return args.result;
}

struct rbg_rval2gints_args {
    VALUE ary;
    long n;
    gint *result;
};

static VALUE
rbg_rval2gints_body(VALUE value)
{
    long i;
    struct rbg_rval2gints_args *args = (struct rbg_rval2gints_args *)value;

    for (i = 0; i < args->n; i++)
        args->result[i] = NUM2INT(RARRAY_PTR(args->ary)[i]);

    return Qnil;
}

static G_GNUC_NORETURN VALUE
rbg_rval2gints_rescue(VALUE value)
{
    g_free(((struct rbg_rval2gints_args *)value)->result);

    rb_exc_raise(rb_errinfo());
}

gint *
rbg_rval2gints(volatile VALUE *value, long *n)
{
    struct rbg_rval2gints_args args;

    args.ary = *value = rb_ary_dup(rb_ary_to_ary(*value));
    args.n = RARRAY_LEN(args.ary);
    args.result = g_new(gint, args.n + 1);

    rb_rescue(rbg_rval2gints_body, (VALUE)&args,
              rbg_rval2gints_rescue, (VALUE)&args);

    *n = args.n;

    return args.result;
}

struct rbg_rval2gint8s_args {
    VALUE ary;
    long n;
    gint8 *result;
};

static VALUE
rbg_rval2gint8s_body(VALUE value)
{
    long i;
    struct rbg_rval2gint8s_args *args = (struct rbg_rval2gint8s_args *)value;

    for (i = 0; i < args->n; i++)
        args->result[i] = NUM2CHR(RARRAY_PTR(args->ary)[i]);

    return Qnil;
}

static G_GNUC_NORETURN VALUE
rbg_rval2gint8s_rescue(VALUE value)
{
    g_free(((struct rbg_rval2gint8s_args *)value)->result);

    rb_exc_raise(rb_errinfo());
}

gint8 *
rbg_rval2gint8s(volatile VALUE *value, long *n)
{
    struct rbg_rval2gint8s_args args;

    args.ary = *value = rb_ary_dup(rb_ary_to_ary(*value));
    args.n = RARRAY_LEN(args.ary);
    args.result = g_new(gint8, args.n + 1);

    rb_rescue(rbg_rval2gint8s_body, (VALUE)&args,
              rbg_rval2gint8s_rescue, (VALUE)&args);

    *n = args.n;

    return args.result;
}

struct rbg_rval2guint8s_args {
    VALUE ary;
    long n;
    guint8 *result;
};

static VALUE
rbg_rval2guint8s_body(VALUE value)
{
    long i;
    struct rbg_rval2guint8s_args *args = (struct rbg_rval2guint8s_args *)value;

    for (i = 0; i < args->n; i++)
        args->result[i] = NUM2UINT(RARRAY_PTR(args->ary)[i]);

    return Qnil;
}

static G_GNUC_NORETURN VALUE
rbg_rval2guint8s_rescue(VALUE value)
{
    g_free(((struct rbg_rval2guint8s_args *)value)->result);

    rb_exc_raise(rb_errinfo());
}

guint8 *
rbg_rval2guint8s(volatile VALUE *value, long *n)
{
    struct rbg_rval2guint8s_args args;

    args.ary = *value = rb_ary_dup(rb_ary_to_ary(*value));
    args.n = RARRAY_LEN(args.ary);
    args.result = g_new(guint8, args.n + 1);

    rb_rescue(rbg_rval2guint8s_body, (VALUE)&args,
              rbg_rval2guint8s_rescue, (VALUE)&args);

    *n = args.n;

    return args.result;
}

struct rbg_rval2guint16s_args {
    VALUE ary;
    long n;
    guint16 *result;
};

static VALUE
rbg_rval2guint16s_body(VALUE value)
{
    long i;
    struct rbg_rval2guint16s_args *args = (struct rbg_rval2guint16s_args *)value;

    for (i = 0; i < args->n; i++)
        args->result[i] = NUM2UINT(RARRAY_PTR(args->ary)[i]);

    return Qnil;
}

static G_GNUC_NORETURN VALUE
rbg_rval2guint16s_rescue(VALUE value)
{
    g_free(((struct rbg_rval2guint16s_args *)value)->result);

    rb_exc_raise(rb_errinfo());
}

guint16 *
rbg_rval2guint16s(volatile VALUE *value, long *n)
{
    struct rbg_rval2guint16s_args args;

    args.ary = *value = rb_ary_dup(rb_ary_to_ary(*value));
    args.n = RARRAY_LEN(args.ary);
    args.result = g_new(guint16, args.n + 1);

    rb_rescue(rbg_rval2guint16s_body, (VALUE)&args,
              rbg_rval2guint16s_rescue, (VALUE)&args);

    *n = args.n;

    return args.result;
}

struct rbg_rval2guint32s_args {
    VALUE ary;
    long n;
    guint32 *result;
};

static VALUE
rbg_rval2guint32s_body(VALUE value)
{
    long i;
    struct rbg_rval2guint32s_args *args = (struct rbg_rval2guint32s_args *)value;

    for (i = 0; i < args->n; i++)
        args->result[i] = NUM2UINT(RARRAY_PTR(args->ary)[i]);

    return Qnil;
}

static G_GNUC_NORETURN VALUE
rbg_rval2guint32s_rescue(VALUE value)
{
    g_free(((struct rbg_rval2guint32s_args *)value)->result);

    rb_exc_raise(rb_errinfo());
}

guint32 *
rbg_rval2guint32s(volatile VALUE *value, long *n)
{
    struct rbg_rval2guint32s_args args;

    args.ary = *value = rb_ary_dup(rb_ary_to_ary(*value));
    args.n = RARRAY_LEN(args.ary);
    args.result = g_new(guint32, args.n + 1);

    rb_rescue(rbg_rval2guint32s_body, (VALUE)&args,
              rbg_rval2guint32s_rescue, (VALUE)&args);

    *n = args.n;

    return args.result;
}

struct rbg_rval2gdoubles_args {
    VALUE ary;
    long n;
    gdouble *result;
};

static VALUE
rbg_rval2gdoubles_body(VALUE value)
{
    long i;
    struct rbg_rval2gdoubles_args *args = (struct rbg_rval2gdoubles_args *)value;

    for (i = 0; i < args->n; i++)
        args->result[i] = NUM2DBL(RARRAY_PTR(args->ary)[i]);

    return Qnil;
}

static G_GNUC_NORETURN VALUE
rbg_rval2gdoubles_rescue(VALUE value)
{
    g_free(((struct rbg_rval2gdoubles_args *)value)->result);

    rb_exc_raise(rb_errinfo());
}

gdouble *
rbg_rval2gdoubles(volatile VALUE *value, long *n)
{
    struct rbg_rval2gdoubles_args args;

    args.ary = *value = rb_ary_dup(rb_ary_to_ary(*value));
    args.n = RARRAY_LEN(args.ary);
    args.result = g_new(gdouble, args.n + 1);

    rb_rescue(rbg_rval2gdoubles_body, (VALUE)&args,
              rbg_rval2gdoubles_rescue, (VALUE)&args);

    *n = args.n;

    return args.result;
}

VALUE
rbg_gints2rval(const gint *gints, long n)
{
    long i;
    VALUE ary = rb_ary_new();

    for (i = 0; i < n; i++)
        rb_ary_push(ary, INT2NUM(gints[i]));

    return ary;
}

struct rbg_gints2rval_free_args {
    gint *gints;
    long n;
};

static VALUE
rbg_gints2rval_free_body(VALUE value)
{
    struct rbg_gints2rval_free_args *args = (struct rbg_gints2rval_free_args *)value;

    return rbg_gints2rval(args->gints, args->n);
}

static VALUE
rbg_gints2rval_free_ensure(VALUE value)
{
    g_free(((struct rbg_gints2rval_free_args *)value)->gints);

    return Qnil;
}

VALUE
rbg_gints2rval_free(gint *gints, long n)
{
    struct rbg_gints2rval_free_args args = { gints, n };

    return rb_ensure(rbg_gints2rval_free_body, (VALUE)&args,
                     rbg_gints2rval_free_ensure, (VALUE)&args);
}

static const char *
rbg_inspect (VALUE object)
{
    VALUE inspected;

    inspected = rb_funcall(object, rb_intern("inspect"), 0);
    return StringValueCStr(inspected);
}


void
rbg_scan_options (VALUE options, ...)
{
    VALUE original_options = options;
    VALUE available_keys;
    const char *key;
    VALUE *value;
    va_list args;

    options = rb_check_convert_type(options, T_HASH, "Hash", "to_hash");
    if (NIL_P(options)) {
        options = rb_hash_new();
    } else if (options == original_options) {
        options = rb_funcall(options, rb_intern("dup"), 0);
    }

    available_keys = rb_ary_new();
    va_start(args, options);
    key = va_arg(args, const char *);
    while (key) {
        VALUE rb_key;
        value = va_arg(args, VALUE *);

        rb_key = ID2SYM(rb_intern(key));
        rb_ary_push(available_keys, rb_key);
        *value = rb_funcall(options, rb_intern("delete"), 1, rb_key);

        key = va_arg(args, const char *);
    }
    va_end(args);

    if (RVAL2CBOOL(rb_funcall(options, rb_intern("empty?"), 0)))
        return;

    rb_raise(rb_eArgError,
             "unexpected key(s) exist: %s: available keys: %s",
             rbg_inspect(rb_funcall(options, rb_intern("keys"), 0)),
             rbg_inspect(available_keys));
}

#if 0
/*
2004-04-15 Commented out by Masao.

These functions replace g_malloc/g_realloc/g_free of GLib.
When g_malloc is called and the memory area can not reserved,
rb_gc() will be called. It makes Ruby-GNOME2 uses memory efficiently.

But rb_gc() does not work under multithread.
So they occur "cross-thread violation".
*/

static gpointer
my_malloc(gsize n_bytes)
{
    /* Should we rescue NoMemoryError? */ 
    return ruby_xmalloc(n_bytes);
}

static gpointer
my_realloc(gpointer mem, gsize n_bytes)
{
    /* Should we rescue NoMemoryError? */ 
    return ruby_xrealloc(mem, n_bytes);
}

static void
my_free(gpointer mem)
{
    return ruby_xfree(mem);
}

static void
Init_mem()
{
    GMemVTable mem_table = {
        my_malloc,
        my_realloc,
        my_free,
        NULL,
        NULL,
        NULL,
    };
    g_mem_set_vtable(&mem_table);
}
#endif

static VALUE
rg_m_os_win32_p(G_GNUC_UNUSED VALUE self)
{
#ifdef G_OS_WIN32
    return Qtrue;
#else
    return Qfalse;
#endif
}

static VALUE
rg_m_os_beos_p(G_GNUC_UNUSED VALUE self)
{
#ifdef G_OS_BEOS
    return Qtrue;
#else
    return Qfalse;
#endif
}

static VALUE
rg_m_os_unix_p(G_GNUC_UNUSED VALUE self)
{
#ifdef G_OS_UNIX
    return Qtrue;
#else
    return Qfalse;
#endif
}

extern void Init_glib2(void);

void
Init_glib2(void)
{
#ifdef HAVE_RUBY_ENCODING_H
    const gchar **filename_charsets;
#endif

    id_inspect = rb_intern("inspect");

    RG_TARGET_NAMESPACE = rb_define_module("GLib");

    setlocale (LC_CTYPE, "");
#ifdef LC_MESSAGES
    setlocale (LC_MESSAGES, "");
#endif

    /* Version Information */
    rb_define_const(RG_TARGET_NAMESPACE, "VERSION",
                    rb_ary_new3(3,
                                INT2FIX(glib_major_version),
                                INT2FIX(glib_minor_version),
                                INT2FIX(glib_micro_version)));
    rb_define_const(RG_TARGET_NAMESPACE, "MAJOR_VERSION", INT2FIX(glib_major_version));
    rb_define_const(RG_TARGET_NAMESPACE, "MINOR_VERSION", INT2FIX(glib_minor_version));
    rb_define_const(RG_TARGET_NAMESPACE, "MICRO_VERSION", INT2FIX(glib_micro_version));
    rb_define_const(RG_TARGET_NAMESPACE, "INTERFACE_AGE", INT2FIX(glib_interface_age));
    rb_define_const(RG_TARGET_NAMESPACE, "BINARY_AGE", INT2FIX(glib_binary_age));

    rb_define_const(RG_TARGET_NAMESPACE, "BINDING_VERSION",
                    rb_ary_new3(3,
                                INT2FIX(RBGLIB_MAJOR_VERSION),
                                INT2FIX(RBGLIB_MINOR_VERSION),
                                INT2FIX(RBGLIB_MICRO_VERSION)));

    rb_define_const(RG_TARGET_NAMESPACE, "BUILD_VERSION",
                    rb_ary_new3(3,
                                INT2FIX(GLIB_MAJOR_VERSION),
                                INT2FIX(GLIB_MINOR_VERSION),
                                INT2FIX(GLIB_MICRO_VERSION)));

    /* Limits of Basic Types */
    rb_define_const(RG_TARGET_NAMESPACE, "MININT", INT2FIX(G_MININT));
    rb_define_const(RG_TARGET_NAMESPACE, "MAXINT", INT2NUM(G_MAXINT));
    rb_define_const(RG_TARGET_NAMESPACE, "MAXUINT", UINT2NUM(G_MAXUINT));

    rb_define_const(RG_TARGET_NAMESPACE, "MINSHORT", INT2FIX(G_MINSHORT));
    rb_define_const(RG_TARGET_NAMESPACE, "MAXSHORT", INT2FIX(G_MAXSHORT));
    rb_define_const(RG_TARGET_NAMESPACE, "MAXUSHORT", UINT2NUM(G_MAXUSHORT));

    rb_define_const(RG_TARGET_NAMESPACE, "MINLONG", INT2FIX(G_MINLONG));
    rb_define_const(RG_TARGET_NAMESPACE, "MAXLONG", INT2NUM(G_MAXLONG));
    rb_define_const(RG_TARGET_NAMESPACE, "MAXULONG", UINT2NUM(G_MAXULONG));

#if GLIB_CHECK_VERSION(2,4,0)
    rb_define_const(RG_TARGET_NAMESPACE, "MININT8", INT2FIX(G_MININT8));
    rb_define_const(RG_TARGET_NAMESPACE, "MAXINT8", INT2FIX(G_MAXINT8));
    rb_define_const(RG_TARGET_NAMESPACE, "MAXUINT8", UINT2NUM(G_MAXUINT8));

    rb_define_const(RG_TARGET_NAMESPACE, "MININT16", INT2FIX(G_MININT16));
    rb_define_const(RG_TARGET_NAMESPACE, "MAXINT16", INT2FIX(G_MAXINT16));
    rb_define_const(RG_TARGET_NAMESPACE, "MAXUINT16", UINT2NUM(G_MAXUINT16));

    rb_define_const(RG_TARGET_NAMESPACE, "MININT32", INT2FIX(G_MININT32));
    rb_define_const(RG_TARGET_NAMESPACE, "MAXINT32", INT2NUM(G_MAXINT32));
    rb_define_const(RG_TARGET_NAMESPACE, "MAXUINT32", UINT2NUM(G_MAXUINT32));
#endif
    rb_define_const(RG_TARGET_NAMESPACE, "MININT64", INT2FIX(G_MININT64));
    rb_define_const(RG_TARGET_NAMESPACE, "MAXINT64", LL2NUM(G_MAXINT64));
    rb_define_const(RG_TARGET_NAMESPACE, "MAXUINT64", ULL2NUM(G_MAXUINT64));
#if GLIB_CHECK_VERSION(2,4,0)
    rb_define_const(RG_TARGET_NAMESPACE, "MAXSIZE", UINT2NUM(G_MAXSIZE));
#endif
    rb_define_const(RG_TARGET_NAMESPACE, "MINFLOAT", INT2FIX(G_MINFLOAT));
    rb_define_const(RG_TARGET_NAMESPACE, "MAXFLOAT", DBL2NUM(G_MAXFLOAT));

    rb_define_const(RG_TARGET_NAMESPACE, "MINDOUBLE", INT2FIX(G_MINDOUBLE));
    rb_define_const(RG_TARGET_NAMESPACE, "MAXDOUBLE", DBL2NUM(G_MAXDOUBLE));

    /* Standard Macros */
    RG_DEF_MODFUNC_P(os_win32, 0);
    RG_DEF_MODFUNC_P(os_beos, 0);
    RG_DEF_MODFUNC_P(os_unix, 0);

    rb_define_const(RG_TARGET_NAMESPACE, "DIR_SEPARATOR", CSTR2RVAL(G_DIR_SEPARATOR_S));
    rb_define_const(RG_TARGET_NAMESPACE, "SEARCHPATH_SEPARATOR", CSTR2RVAL(G_SEARCHPATH_SEPARATOR_S));

    /* discover and store glib filename encoding */
#ifdef HAVE_RUBY_ENCODING_H
    if (g_get_filename_charsets(&filename_charsets)
        || filename_charsets == NULL
        || filename_charsets[0] == NULL
        || !strcmp(filename_charsets[0], "UTF-8")
        || rb_enc_find(filename_charsets[0]) == rb_enc_find("ASCII-8BIT")) {
        /* set to NULL, mean do not perform transcoding, either filename
           encoding is unknown, UTF-8, or unsupported */
        filename_encoding_if_not_utf8 = NULL;
    } else {
        filename_encoding_if_not_utf8 = rb_enc_find(filename_charsets[0]);
    }
#endif

/* Don't implement them.
#define     G_DIR_SEPARATOR_S
#define     G_IS_DIR_SEPARATOR              (c)
#define     G_SEARCHPATH_SEPARATOR
#define     TRUE
#define     FALSE
#define     NULL
#define     MIN                             (a, b)
#define     MAX                             (a, b)
#define     ABS                             (a)
#define     CLAMP                           (x, low, high)
#define     G_STRUCT_MEMBER                 (member_type, struct_p, struct_offset)
#define     G_STRUCT_MEMBER_P               (struct_p, struct_offset)
#define     G_STRUCT_OFFSET                 (struct_type, member)
#define     G_MEM_ALIGN
#define     G_CONST_RETURN
*/

    /* Numerical Definitions */
/* Don't implement them.
#define     G_IEEE754_FLOAT_BIAS
#define     G_IEEE754_DOUBLE_BIAS
union       GFloatIEEE754;
union       GDoubleIEEE754;
*/
    rb_define_const(RG_TARGET_NAMESPACE, "E", CSTR2RVAL(G_STRINGIFY(G_E)));
    rb_define_const(RG_TARGET_NAMESPACE, "LN2", CSTR2RVAL(G_STRINGIFY(G_LN2)));
    rb_define_const(RG_TARGET_NAMESPACE, "LN10", CSTR2RVAL(G_STRINGIFY(G_LN10)));
    rb_define_const(RG_TARGET_NAMESPACE, "PI", CSTR2RVAL(G_STRINGIFY(G_PI)));
    rb_define_const(RG_TARGET_NAMESPACE, "PI_2", CSTR2RVAL(G_STRINGIFY(G_PI_2)));
    rb_define_const(RG_TARGET_NAMESPACE, "PI_4", CSTR2RVAL(G_STRINGIFY(G_PI_4)));
    rb_define_const(RG_TARGET_NAMESPACE, "SQRT2", CSTR2RVAL(G_STRINGIFY(G_SQRT2)));
    rb_define_const(RG_TARGET_NAMESPACE, "LOG_2_BASE_10", CSTR2RVAL(G_STRINGIFY(G_LOG_2_BASE_10)));

    /* From "The Main Event Loop" */
    rb_define_const(RG_TARGET_NAMESPACE, "PRIORITY_HIGH", INT2FIX(G_PRIORITY_HIGH));
    rb_define_const(RG_TARGET_NAMESPACE, "PRIORITY_DEFAULT", INT2FIX(G_PRIORITY_DEFAULT));
    rb_define_const(RG_TARGET_NAMESPACE, "PRIORITY_HIGH_IDLE", INT2FIX(G_PRIORITY_HIGH_IDLE));
    rb_define_const(RG_TARGET_NAMESPACE, "PRIORITY_DEFAULT_IDLE", INT2FIX(G_PRIORITY_DEFAULT_IDLE));
    rb_define_const(RG_TARGET_NAMESPACE, "PRIORITY_LOW", INT2FIX(G_PRIORITY_LOW));

/*    Init_mem(); */
    Init_gutil();
    Init_gutil_callback();
    Init_glib_int64();
    Init_glib_error();
    Init_glib_threads();
    Init_glib_convert();
    Init_glib_messages();
    Init_glib_fileutils();
    Init_glib_i18n();
    Init_glib_win32();

    Init_gobject();

    /* Require GBoxed/GObject */
    Init_glib_utils();
    Init_glib_spawn();
    Init_glib_spawnerror();
    Init_glib_main_loop();
    Init_glib_source();
    Init_glib_main_context();
    Init_glib_poll_fd();
    Init_glib_io_channel();
    Init_glib_io_channelerror();
    Init_glib_io_channel_win32_socket();
    Init_glib_shell();
    Init_glib_shellerror();
    Init_glib_completion();
    Init_glib_timer();
    Init_glib_unicode();
    Init_glib_utf8();
    Init_glib_utf16();
    Init_glib_ucs4();
    Init_glib_unichar();
    Init_glib_keyfile();
    Init_glib_bookmark_file();

    /* This is called here once. */
    G_DEF_SETTERS(RG_TARGET_NAMESPACE);
}
