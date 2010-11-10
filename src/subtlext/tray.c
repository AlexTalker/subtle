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

/* subTrayInstantiate {{{ */
VALUE
subTrayInstantiate(Window win)
{
  VALUE klass = Qnil, tray = Qnil;

  /* Create new instance */
  klass = rb_const_get(mod, rb_intern("Tray"));
  tray  = rb_funcall(klass, rb_intern("new"), 1, LONG2NUM(win));

  return tray;
} /* }}} */

/* subTrayInit {{{ */
/*
 * call-seq: new(name) -> Subtlext::Tray
 *
 * Create new Tray object
 *
 *  tag = Subtlext::Tray.new("subtle")
 *  => #<Subtlext::Tray:xxx>
 */

VALUE
subTrayInit(VALUE self,
  VALUE win)
{
  if(!FIXNUM_P(win) && T_BIGNUM != rb_type(win))
    rb_raise(rb_eArgError, "Unexpected value-type `%s'",
      rb_obj_classname(win));

  rb_iv_set(self, "@id",    Qnil);
  rb_iv_set(self, "@win",   win);
  rb_iv_set(self, "@name",  Qnil);
  rb_iv_set(self, "@klass", Qnil);
  rb_iv_set(self, "@title", Qnil);

  subSubtlextConnect(NULL); ///< Implicit open connection

  return self;
} /* }}} */

/* subTrayFind {{{ */
/*
 * call-seq: find(value) -> Subtlext::Client or nil
 *           [value]     -> Subtlext::Client or nil
 *
 * Find Client by a given value which can be of following type:
 *
 * [fixnum] Array id
 * [string] Match against WM_NAME or WM_CLASS
 * [hash]   With one of following keys: :title, :name, :class, :gravity
 * [symbol] Either :current for current Client or :all for an array
 *
 *  Subtlext::Tray.find("subtle")
 *  => #<Subtlext::Tray:xxx>
 *
 *  Subtlext::Tray[1]
 *  => #<Subtlext::Tray:xxx>
 *
 *  Subtlext::Tray["subtle"]
 *  => nil
 */

VALUE
subTrayFind(VALUE self,
  VALUE value)
{
  int id = 0, flags = 0;
  Window win = None;
  VALUE parsed = Qnil, tray = Qnil;
  char *name = NULL, buf[50] = { 0 };

  subSubtlextConnect(NULL); ///< Implicit open connection

  /* Check object type */
  if(T_SYMBOL == rb_type(parsed = subSubtlextParse(
      value, buf, sizeof(buf), &flags)))
    {
      if(CHAR2SYM("all") == parsed)
        return subTrayAll(Qnil);
    }

  /* Find tray */
  if(-1 != (id = subSubtlextFindWindow("SUBTLE_TRAY_LIST",
      buf, NULL, &win, flags)))
    {
      if(!NIL_P((tray = subTrayInstantiate(win))))
        {
          rb_iv_set(tray, "@id", INT2FIX(id));

          subTrayUpdate(tray);
        }

      free(name);
    }

  return tray;
} /* }}} */

/* subTrayAll {{{ */
/*
 * call-seq: all -> Array
 *
 * Get Array of all Tray
 *
 *  Subtlext::Tray.all
 *  => [#<Subtlext::Tray:xxx>, #<Subtlext::Tray:xxx>]
 *
 *  Subtlext::Tray.all
 *  => []
 */

VALUE
subTrayAll(VALUE self)
{
  int i, size = 0;
  Window *trays = NULL;
  VALUE meth = Qnil, klass = Qnil, array = Qnil;

  subSubtlextConnect(NULL); ///< Implicit open connection

  /* Fetch data */
  meth  = rb_intern("new");
  klass = rb_const_get(mod, rb_intern("Tray"));
  trays = subSubtlextList("SUBTLE_TRAY_LIST", &size);
  array = rb_ary_new2(size);

  /* Populate array */
  if(trays)
    {
      for(i = 0; i < size; i++)
        {
          VALUE t = rb_funcall(klass, meth, 1, LONG2NUM(trays[i]));

          if(!NIL_P(t)) subTrayUpdate(t);

          rb_ary_push(array, t);
        }

      free(trays);
    }

  return array;
} /* }}} */

/* subTrayUpdate {{{ */
/*
 * call-seq: update -> nil
 *
 * Update Tray properties
 *
 *  tray.update
 *  => nil
 */

VALUE
subTrayUpdate(VALUE self)
{
  VALUE win = rb_iv_get(self, "@win");

  rb_check_frozen(self);
  subSubtlextConnect(NULL); ///< Implicit open connection

  /* Check object type */
  if(RTEST(win) && (T_FIXNUM == rb_type(win) || T_BIGNUM == rb_type(win)))
    {
      int id = 0;
      char buf[20] = { 0 };

      snprintf(buf, sizeof(buf), "%#lx", NUM2LONG(win));
      if(-1 != (id = subSubtlextFindWindow("SUBTLE_TRAY_LIST", buf, NULL,
          NULL, (SUB_MATCH_NAME|SUB_MATCH_CLASS))))
        {
          char *title = NULL, *wmname = NULL, *wmclass = NULL;

          XFetchName(display, NUM2LONG(win), &title);
          subSharedPropertyClass(display, NUM2LONG(win), &wmname, &wmclass);

          /* Set properties */
          rb_iv_set(self, "@id",    INT2FIX(id));
          rb_iv_set(self, "@title", rb_str_new2(title));
          rb_iv_set(self, "@name",  rb_str_new2(wmname));
          rb_iv_set(self, "@klass", rb_str_new2(wmclass));

          free(title);
          free(wmname);
          free(wmclass);
        }
      else rb_raise(rb_eStandardError, "Failed finding tray");
    }
  else rb_raise(rb_eStandardError, "Failed updating tray");

  return Qnil;
} /* }}} */

/* subTrayToString {{{ */
/*
 * call-seq: to_str -> String
 *
 * Convert Tray object to String
 *
 *  puts tray
 *  => "subtle"
 */

VALUE
subTrayToString(VALUE self)
{
  VALUE name = rb_iv_get(self, "@name");

  return RTEST(name) ? name : Qnil;
} /* }}} */

/* subTrayKill {{{ */
/*
 * call-seq: kill -> nil
 *
 * Kill a Tray
 *
 *  tray.kill
 *  => nil
 */

VALUE
subTrayKill(VALUE self)
{
  VALUE id = rb_iv_get(self, "@id");

  subSubtlextConnect(NULL); ///< Implicit open connection

  if(RTEST(id))
    {
      SubMessageData data = { { 0, 0, 0, 0, 0 } };

      data.l[0] = FIX2INT(id);

      subSharedMessage(display, DefaultRootWindow(display),
        "SUBTLE_TRAY_KILL", data, 32, True);
    }
  else rb_raise(rb_eStandardError, "Failed killing tray");

  rb_obj_freeze(self); ///< Freeze object

  return Qnil;
} /* }}} */

// vim:ts=2:bs=2:sw=2:et:fdm=marker
