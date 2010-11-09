 /**
  * @package subtle
  *
  * @file subtle ruby extension
  * @copyright (c) 2005-2010 Christoph Kappel <unexist@dorfelite.net>
  * @version $Id$
  *
  * This program can be distributed under the terms of the GNU GPL.
  * See the file COPYING.
  **/

#include "subtlext.h"

/* subTagInstantiate {{{ */
VALUE
subTagInstantiate(char *name)
{
  VALUE klass = Qnil, tag = Qnil;

  /* Create new instance */
  klass = rb_const_get(mod, rb_intern("Tag"));
  tag   = rb_funcall(klass, rb_intern("new"), 1, rb_str_new2(name));

  return tag;
} /* }}} */

/* subTagInit {{{ */
/*
 * call-seq: new(name) -> Subtlext::Tag
 *
 * Create new Tag object
 *
 *  tag = Subtlext::Tag.new("subtle")
 *  => #<Subtlext::Tag:xxx>
 */

VALUE
subTagInit(VALUE self,
  VALUE name)
{
  if(T_STRING != rb_type(name))
    rb_raise(rb_eArgError, "Invalid value type");

  rb_iv_set(self, "@id",   Qnil);
  rb_iv_set(self, "@name", name);

  subSubtlextConnect(NULL); ///< Implicit open connection

  return self;
} /* }}} */

/* subTagFind {{{ */
/*
 * call-seq: find(value) -> Subtlext::Tag or nil
 *           [value]     -> Subtlext::Tag or nil
 *
 * Find Tag by a given value which can be of following type:
 *
 * [fixnum] Array id
 * [string] Match against name of Tag
 * [symbol] :all for an array of all Tag
 *
 *  Subtlext::Tag.find("subtle")
 *  => #<Subtlext::Tag:xxx>
 *
 *  Subtlext::Tag[1]
 *  => #<Subtlext::Tag:xxx>
 *
 *  Subtlext::Tag["subtle"]
 *  => nil
 */

VALUE
subTagFind(VALUE self,
  VALUE value)
{
  int id = 0;
  VALUE parsed = Qnil, tag = Qnil;
  char *name = NULL, buf[50] = { 0 };

  subSubtlextConnect(NULL); ///< Implicit open connection

  /* Check object type */
  if(T_SYMBOL == rb_type(parsed = subSubtlextParse(
      value, buf, sizeof(buf), NULL)))
    {
      if(CHAR2SYM("visible") == parsed)
        return subTagVisible(Qnil);
      else if(CHAR2SYM("all") == parsed)
        return subTagAll(Qnil);
    }

  /* Find tag */
  if(-1 != (id = subSubtlextFind("SUBTLE_TAG_LIST", buf, &name)))
    {
      if(!NIL_P((tag = subTagInstantiate(name))))
        rb_iv_set(tag, "@id", INT2FIX(id));

      free(name);
    }

  return tag;
} /* }}} */

/* subTagVisible {{{ */
/*
 * call-seq: visible -> Array
 *
 * Get Array of all visible Tag
 *
 *  Subtlext::Tag.visible
 *  => [#<Subtlext::Tag:xxx>, #<Subtlext::Tag:xxx>]
 *
 *  Subtlext::Tag.visible
 *  => []
 */

VALUE
subTagVisible(VALUE self)
{
  int i, size = 0;
  char **tags = NULL;
  unsigned long *visible = NULL;
  VALUE meth = Qnil, klass = Qnil, array = Qnil, tag = Qnil;

  subSubtlextConnect(NULL); ///< Implicit open connection

  /* Fetch data */
  meth    = rb_intern("new");
  klass   = rb_const_get(mod, rb_intern("Tag"));
  tags    = subSharedPropertyStrings(display, DefaultRootWindow(display),
    XInternAtom(display, "SUBTLE_TAG_LIST", False), &size);
  visible = (unsigned long *)subSharedPropertyGet(display,
    DefaultRootWindow(display), XA_CARDINAL, XInternAtom(display,
    "SUBTLE_VISIBLE_TAGS", False), NULL);
  array   = rb_ary_new2(size);

  /* Populate array */
  if(tags && visible)
    {
      for(i = 0; i < size; i++)
        {
          /* Create tag on match */
          if(*visible & (1L << (i + 1)) && !NIL_P(tag = rb_funcall(klass,
              meth, 1, rb_str_new2(tags[i]))))
            {
              rb_iv_set(tag, "@id", INT2FIX(i));
              rb_ary_push(array, tag);
            }
        }

      XFreeStringList(tags);
      free(visible);
    }

  return array;
} /* }}} */

/* subTagAll {{{ */
/*
 * call-seq: all -> Array
 *
 * Get Array of all Tag
 *
 *  Subtlext::Tag.all
 *  => [#<Subtlext::Tag:xxx>, #<Subtlext::Tag:xxx>]
 *
 *  Subtlext::Tag.all
 *  => []
 */

VALUE
subTagAll(VALUE self)
{
  int i, size = 0;
  char **tags = NULL;
  VALUE meth = Qnil, klass = Qnil, array = Qnil;

  subSubtlextConnect(NULL); ///< Implicit open connection

  /* Fetch data */
  meth  = rb_intern("new");
  klass = rb_const_get(mod, rb_intern("Tag"));
  tags  = subSharedPropertyStrings(display, DefaultRootWindow(display),
    XInternAtom(display, "SUBTLE_TAG_LIST", False), &size);
  array = rb_ary_new2(size);

  /* Populate array */
  if(tags)
    {
      for(i = 0; i < size; i++)
        {
          VALUE t = rb_funcall(klass, meth, 1, rb_str_new2(tags[i]));

          rb_iv_set(t, "@id", INT2FIX(i));
          rb_ary_push(array, t);
        }

      XFreeStringList(tags);
    }

  return array;
} /* }}} */

/* subTagUpdate {{{ */
/*
 * call-seq: update -> nil
 *
 * Update Tag properties
 *
 *  tag.update
 *  => nil
 */

VALUE
subTagUpdate(VALUE self)
{
  VALUE name = rb_iv_get(self, "@name");

  rb_check_frozen(self);

  /* Check object type */
  if(T_STRING == rb_type(name))
    {
      int id = -1;

      /* Create tag if needed */
      if(-1 == (id = subSubtlextFind("SUBTLE_TAG_LIST",
          RSTRING_PTR(name), NULL)))
        {
          SubMessageData data = { { 0, 0, 0, 0, 0 } };

          snprintf(data.b, sizeof(data.b), "%s", RSTRING_PTR(name));
          subSharedMessage(display, DefaultRootWindow(display),
            "SUBTLE_TAG_NEW", data, 8, True);

          id = subSubtlextFind("SUBTLE_TAG_LIST", RSTRING_PTR(name), NULL);
        }

      /* Guess tag id */
      if(-1 == id)
        {
          int size = 0;
          char **tags = NULL;

          tags = subSharedPropertyStrings(display, DefaultRootWindow(display),
            XInternAtom(display, "SUBTLE_TAG_LIST", False), &size);

          id = size; ///< New id should be last

          XFreeStringList(tags);
        }

      rb_iv_set(self, "@id", INT2FIX(id));
    }
  else rb_raise(rb_eArgError, "Unknown value type");

  return Qnil;
} /* }}} */

/* subTagClients {{{ */
/*
 * call-seq: clients -> Array
 *
 * Get Array of Client that are tagged with this
 *
 *  tag.clients
 *  => [#<Subtlext::Client:xxx>, #<Subtlext::Client:xxx>]
 *
 *  tag.clients
 *  => []
 */

VALUE
subTagClients(VALUE self)
{
  return subSubtlextAssoc(self, SUB_TYPE_CLIENT);
} /* }}} */

/* subTagViews {{{ */
/*
 * call-seq: views -> Array
 *
 * Get Array of View that are tagged with this
 *
 *  tag.views
 *  => [#<Subtlext::Views:xxx>, #<Subtlext::Views:xxx>]
 *
 *  tag.views
 *  => []
 */

VALUE
subTagViews(VALUE self)
{
  return subSubtlextAssoc(self, SUB_TYPE_VIEW);
} /* }}} */

/* subTagToString {{{ */
/*
 * call-seq: to_str -> String
 *
 * Convert Tag object to String
 *
 *  puts tag
 *  => "subtle"
 */

VALUE
subTagToString(VALUE self)
{
  VALUE name = rb_iv_get(self, "@name");

  return RTEST(name) ? name : Qnil;
} /* }}} */

/* subTagKill {{{ */
/*
 * call-seq: kill -> nil
 *
 * Kill a Tag
 *
 *  tag.kill
 *  => nil
 */

VALUE
subTagKill(VALUE self)
{
  VALUE id = rb_iv_get(self, "@id");

  subSubtlextConnect(NULL); ///< Implicit open connection

  if(RTEST(id))
    {
      SubMessageData data = { { 0, 0, 0, 0, 0 } };

      data.l[0] = FIX2INT(id);

      subSharedMessage(display, DefaultRootWindow(display),
        "SUBTLE_TAG_KILL", data, 32, True);
    }
  else rb_raise(rb_eStandardError, "Failed killing tag");

  rb_obj_freeze(self); ///< Freeze object

  return Qnil;
} /* }}} */

// vim:ts=2:bs=2:sw=2:et:fdm=marker
