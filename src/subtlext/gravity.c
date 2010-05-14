 /**
  * @package subtlext
  *
  * @file Gravity functions
  * @copyright (c) 2005-2010 Christoph Kappel <unexist@dorfelite.net>
  * @version $Id$
  *
  * This program can be distributed under the terms of the GNU GPL.
  * See the file COPYING.
  **/

#include "subtlext.h"

/* GravityToRect {{{ */
void
GravityToRect(VALUE self,
  XRectangle *r)
{
  VALUE geometry = rb_iv_get(self, "@geometry");

  subGeometryToRect(geometry, r); ///< Get values
} /* }}} */

/* subGravityInstantiate {{{ */
VALUE
subGravityInstantiate(char *name)
{
  VALUE klass = Qnil, gravity = Qnil;

  /* Create new instance */
  klass   = rb_const_get(mod, rb_intern("Gravity"));
  gravity = rb_funcall(klass, rb_intern("new"), 1, rb_str_new2(name));

  return gravity;
} /* }}} */

/* subGravityInit {{{ */
/*
 * call-seq: new(name, gravity) -> Subtlext::Gravity
 *
 * Create a new Gravity object
 *
 *  gravity = Subtlext::Gravity.new("top")
 *  => #<Subtlext::Gravity:xxx>
 */

VALUE
subGravityInit(int argc,
  VALUE *argv,
  VALUE self)
{
  VALUE data[2] = { Qnil };

  rb_scan_args(argc, argv, "02", &data[0], &data[1]);

  if(T_STRING != rb_type(data[0]))
    rb_raise(rb_eArgError, "Invalid value type");

  rb_iv_set(self, "@id",       Qnil);
  rb_iv_set(self, "@name",     data[0]);
  rb_iv_set(self, "@geometry", data[1]);

  subSubtlextConnect(); ///< Implicit open connection

  return self;
} /* }}} */

/* subGravityFind {{{ */
/*
 * call-seq: find(value) -> Subtlext::Gravity or nil
 *           [value]     -> Subtlext::Gravity or nil
 *
 * Find Gravity by a given value which can be of following type:
 *
 * [fixnum] Array id
 * [string] Match against name of Gravity
 * [symbol] Symbol of the Gravity or :all for an array of all Gravity
 *
 *  Subtlext::Gravity.find("center")
 *  => #<Subtlext::Gravity:xxx>
 *
 *  Subtlext::Gravity[:center]
 *  => #<Subtlext::Gravity:xxx>
 *
 *  Subtlext::Gravity["center"]
 *  => nil
 */

VALUE
subGravityFind(VALUE self,
  VALUE value)
{
  return subSubtlextFind(SUB_TYPE_GRAVITY, value, True);
} /* }}} */

/* subGravityAll {{{ */
/*
 * call-seq: gravities -> Array
 *
 * Get Array of all Gravity
 *
 *  Subtlext::Gravity.all
 *  => [#<Subtlext::Gravity:xxx>, #<Subtlext::Gravity:xxx>]
 *
 *  Subtlext::Gravity.all
 *  => []
 */

VALUE
subGravityAll(VALUE self)
{
  int size = 0;
  char **gravities = NULL;

  VALUE array = rb_ary_new();

  subSubtlextConnect(); ///< Implicit open connection

  /* Get gravity list */
  if((gravities = subSharedPropertyStrings(display, DefaultRootWindow(display),
      XInternAtom(display, "SUBTLE_GRAVITY_LIST", False), &size)))
    {
      int i;
      XRectangle geometry = { 0 };
      char buf[30] = { 0 };
      VALUE klass_grav = Qnil, klass_geom = Qnil, meth = Qnil, gravity = Qnil, geom = Qnil;

      klass_grav = rb_const_get(mod, rb_intern("Gravity"));
      klass_geom = rb_const_get(mod, rb_intern("Geometry"));
      meth       = rb_intern("new");

      /* Create gravity list */
      for(i = 0; i < size; i++)
        {
          sscanf(gravities[i], "%hdx%hd+%hd+%hd#%s", &geometry.x, &geometry.y,
            &geometry.width, &geometry.height, buf);

          gravity = rb_funcall(klass_grav, meth, 1, rb_str_new2(buf));
          geom    = rb_funcall(klass_geom, meth, 4, INT2FIX(geometry.x), INT2FIX(geometry.y),
            INT2FIX(geometry.width), INT2FIX(geometry.height));

          rb_iv_set(gravity, "@id", INT2FIX(i));
          rb_iv_set(gravity, "@geometry", geom);

          rb_ary_push(array, gravity);
        }

      XFreeStringList(gravities);
    }
  else rb_raise(rb_eStandardError, "Failed getting gravity list");

  return array;
} /* }}} */

/* subGravityUpdate {{{ */
/*
 * call-seq: update -> nil
 *
 * Update Gravity properties
 *
 *  gravity.update
 *  => nil
 */

VALUE
subGravityUpdate(VALUE self)
{
  VALUE match = rb_iv_get(self, "@name");

  /* Create gravity if needed */
  if(T_STRING == rb_type(match))
    {
      int id = -1;
      XRectangle geom = { 0 };
      char *name = NULL;

      if(-1 == (id = subSharedGravityFind(RSTRING_PTR(match), &name, &geom)))
        {
          SubMessageData data = { { 0, 0, 0, 0, 0 } };
          VALUE geometry = rb_iv_get(self, "@geometry");

          if(NIL_P(geometry = rb_iv_get(self, "@geometry")))
            rb_raise(rb_eStandardError, "No geometry given");

          subGeometryToRect(geometry, &geom); ///< Get values

          snprintf(data.b, sizeof(data.b), "%hdx%hd+%hd+%hd#%s",
            geom.x, geom.y, geom.width, geom.width, RSTRING_PTR(name));
          subSharedMessage(DefaultRootWindow(display), "SUBTLE_GRAVITY_NEW", data, True);

          id = subSharedGravityFind(RSTRING_PTR(name), NULL, NULL);
        }
      else ///< Update gravity
        {
          VALUE geometry = Qnil;

          geometry = subGeometryInstantiate(geom.x, geom.y,
            geom.width, geom.height);

          rb_iv_set(self, "@name",    rb_str_new2(name));
          rb_iv_set(self, "@gravity", geometry);

          free(name);
        }

      /* Guess gravity id */
      if(-1 == id)
        {
          int size = 0;
          char **gravities = NULL;

          gravities = subSharedPropertyStrings(display, DefaultRootWindow(display),
            XInternAtom(display, "SUBTLE_GRAVITY_LIST", False), &size);

          id = size; ///< New id should be last

          XFreeStringList(gravities);
        }

      rb_iv_set(self, "@id", INT2FIX(id));
    }
  else rb_raise(rb_eArgError, "Unknown value type");

  return Qnil;
} /* }}} */

/* subGravityGeometryReader {{{ */
/*
 * call-seq: geometry -> Subtlext::Geometry
 *
 * Get Gravity Geometry
 *
 *  gravity.geometry
 *  => #<Subtlext::Geometry:xxx>
 */

VALUE
subGravityGeometryReader(VALUE self)
{
  VALUE geometry = Qnil, name = Qnil;

  /* Load on demand */
  if(NIL_P((geometry = rb_iv_get(self, "@geometry"))) &&
      T_STRING == rb_type((name = rb_iv_get(self, "@name"))))
    {
      XRectangle geom = { 0 };

      subSharedGravityFind(RSTRING_PTR(name), NULL, &geom);

      geometry = subGeometryInstantiate(geom.x, geom.y, geom.width, geom.height);
      rb_iv_set(self, "@geometry", geometry);
    }

  return geometry;
} /* }}} */

/* subGravityGeometryWriter {{{ */
/*
 * call-seq: geometry=(geometry) -> nil
 *
 * Set Gravity Geometry
 *
 *  gravity.geometry=geometry
 *  => #<Subtlext::Geometry:xxx>
 */

VALUE
subGravityGeometryWriter(VALUE self,
  VALUE value)
{
  /* Check value type */
  if(T_OBJECT == rb_type(value))
    {
      VALUE klass = rb_const_get(mod, rb_intern("Geometry"));

      /* Check object instance */
      if(rb_obj_is_instance_of(value, klass))
        {
          rb_iv_set(self, "@geometry", value);
        }
      else rb_raise(rb_eArgError, "Unknown value type");
    }

  return Qnil;
} /* }}} */

/* subGravityGeometryFor {{{ */
/*
 * call-seq: geometry_for(screen) -> nil
 *
 * Get Gravity Geometry for Screen in pixel values
 *
 *  gravity.geometry_for(screen)
 *  => #<Subtlext::Geometry:xxx>
 */

VALUE
subGravityGeometryFor(VALUE self,
  VALUE value)
{
  VALUE ary = rb_ary_new2(4);

  /* Check value type */
  if(T_OBJECT == rb_type(value))
    {
      VALUE klass = rb_const_get(mod, rb_intern("Screen"));

      /* Check object instance */
      if(rb_obj_is_instance_of(value, klass))
        {
          XRectangle real = { 0 }, geom_grav = { 0 }, geom_screen = { 0 };

          GravityToRect(self,  &geom_grav);
          GravityToRect(value, &geom_screen);

          /* Calculate real values for screen */
          real.width  = geom_screen.width * geom_grav.width / 100;
          real.height = geom_screen.height * geom_grav.height / 100;
          real.x      = geom_screen.x +
            (geom_screen.width - real.width) * geom_grav.x / 100;
          real.y      = geom_screen.y +
            (geom_screen.height - real.height) * geom_grav.y / 100;

          /* Fill into result array */
          rb_ary_push(ary, INT2FIX(real.x));
          rb_ary_push(ary, INT2FIX(real.y));
          rb_ary_push(ary, INT2FIX(real.width));
          rb_ary_push(ary, INT2FIX(real.height));
        }
      else rb_raise(rb_eArgError, "Unknown value type");
    }

  return ary;
} /* }}} */

/* subGravityToString {{{ */
/*
 * call-seq: to_str -> String
 *
 * Convert Gravity object to String
 *
 *  puts gravity
 *  => "TopLeft"
 */

VALUE
subGravityToString(VALUE self)
{
  return rb_iv_get(self, "@name");
} /* }}} */

/* subGravityToSym {{{ */
/*
 * call-seq: to_sym -> Symbol
 *
 * Convert Gravity object to Symbol
 *
 *  puts gravity.to_sym
 *  => :center
 */

VALUE
subGravityToSym(VALUE self)
{
  VALUE name = rb_iv_get(self, "@name");

  return CHAR2SYM(RSTRING_PTR(name));
} /* }}} */

/* subGravityKill {{{ */
/*
 * call-seq: kill -> nil
 *
 * Kill a Gravity
 *
 *  gravity.kill
 *  => nil
 */

VALUE
subGravityKill(VALUE self)
{
  return subSubtlextKill(self, SUB_TYPE_GRAVITY);
} /* }}} */

// vim:ts=2:bs=2:sw=2:et:fdm=marker
