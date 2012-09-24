/* GStreamer Editing Services
 * Copyright (C) 2009 Edward Hervey <edward.hervey@collabora.co.uk>
 *               2009 Nokia Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:ges-custom-timeline-source
 * @short_description: Convenience #GESTimelineSource
 *
 * #GESCustomTimelineSource allows creating #GESTimelineSource(s) without the
 * need to subclass.
 *
 * Its usage should be limited to testing and prototyping purposes.
 *
 * To instanciate a material to extract GESCustomTimelineSource-s the expected
 * ID is:
 *  'PointerToFuncAsInt!PointerToUDataAsInt'
 *
 * You should use the #ges_material_custom_timeline_source_new helper to create
 * a new GESMaterial letting you extract GESCustomTimelineSource.
 */

#include "ges-internal.h"
#include "ges-custom-timeline-source.h"
#include "ges-timeline-source.h"
#include "ges-track-source.h"
#include "ges-extractable.h"

enum
{
  PROP_0,
  PROP_FILL_FUNC,
  PROP_USER_DATA
};

struct _GESCustomTimelineSourcePrivate
{
  GESFillTrackObjectUserFunc filltrackobjectfunc;
  gpointer user_data;
};

static void ges_extractable_interface_init (GESExtractableInterface * iface);

G_DEFINE_TYPE_WITH_CODE (GESCustomTimelineSource, ges_custom_timeline_source,
    GES_TYPE_TIMELINE_SOURCE,
    G_IMPLEMENT_INTERFACE (GES_TYPE_EXTRACTABLE,
        ges_extractable_interface_init));

static GParameter *
extractable_get_parameters_from_id (const gchar * id, guint * n_params)
{
  gchar **func_udata;
  GParameter *params = g_new0 (GParameter, 2);
  *n_params = 2;

  /* We already know that we have a valid ID here */
  func_udata = g_strsplit (id, "!", -1);

  params[0].name = g_strdup ("fill-func");
  g_value_init (&params[0].value, G_TYPE_POINTER);
  g_value_set_pointer (&params[0].value,
      GUINT_TO_POINTER (g_ascii_strtoll (func_udata[0], NULL, 10)));

  params[1].name = g_strdup ("user-data");
  g_value_init (&params[1].value, G_TYPE_POINTER);
  g_value_set_pointer (&params[1].value,
      GUINT_TO_POINTER (g_ascii_strtoll (func_udata[1], NULL, 10)));

  g_strfreev (func_udata);

  return params;
}

static gchar *
extractable_check_id (GType type, const gchar * id)
{

  gchar *ret, **strv = g_strsplit (id, "!", -1);

  if (g_strv_length (strv) != 2) {
    g_strfreev (strv);

    return NULL;
  }

  /* Remove any whitespace */
  strv[0] = g_strstrip (strv[0]);
  strv[1] = g_strstrip (strv[1]);
  ret = g_strjoinv ("!", strv);

  g_strfreev (strv);

  return ret;
}

static gchar *
extractable_get_id (GESExtractable * self)
{
  GESCustomTimelineSourcePrivate *priv =
      GES_CUSTOM_TIMELINE_SOURCE (self)->priv;

  return g_strdup_printf ("%i!%i", GPOINTER_TO_INT (priv->filltrackobjectfunc),
      GPOINTER_TO_INT (priv->user_data));
}

static void
ges_extractable_interface_init (GESExtractableInterface * iface)
{
  iface->check_id = (GESExtractableCheckId) extractable_check_id;
  iface->get_id = extractable_get_id;
  iface->get_parameters_from_id = extractable_get_parameters_from_id;
}

static gboolean
ges_custom_timeline_source_fill_track_object (GESTimelineObject * object,
    GESTrackObject * trobject, GstElement * gnlobj);

static GESTrackObject *
ges_custom_timeline_source_create_track_object (GESTimelineObject * obj,
    GESTrack * track)
{
  return g_object_new (GES_TYPE_TRACK_SOURCE, NULL);
}

static void
_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GESCustomTimelineSourcePrivate *priv =
      GES_CUSTOM_TIMELINE_SOURCE (object)->priv;
  switch (property_id) {
    case PROP_FILL_FUNC:
      priv->filltrackobjectfunc = g_value_get_pointer (value);
      break;
    case PROP_USER_DATA:
      priv->user_data = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_custom_timeline_source_class_init (GESCustomTimelineSourceClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GESTimelineObjectClass *tlobj_class = GES_TIMELINE_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GESCustomTimelineSourcePrivate));

  tlobj_class->fill_track_object = ges_custom_timeline_source_fill_track_object;
  tlobj_class->create_track_object =
      ges_custom_timeline_source_create_track_object;

  object_class->set_property = _set_property;

  /**
   * GESCustomTimelineSource:fill-func:
   *
   * The function pointer to create the TrackObject content
   */
  g_object_class_install_property (object_class, PROP_FILL_FUNC,
      g_param_spec_pointer ("fill-func", "Fill func",
          "A pointer to a GESFillTrackObjectUserFunc",
          G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  /**
   * GESCustomTimelineSource:user-data:
   *
   * The user data that will be passed
   */
  g_object_class_install_property (object_class, PROP_USER_DATA,
      g_param_spec_pointer ("user-data", "User data",
          "The user data pointer that will be passed when creating TrackObjects",
          G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
ges_custom_timeline_source_init (GESCustomTimelineSource * self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      GES_TYPE_CUSTOM_TIMELINE_SOURCE, GESCustomTimelineSourcePrivate);
}

static gboolean
ges_custom_timeline_source_fill_track_object (GESTimelineObject * object,
    GESTrackObject * trobject, GstElement * gnlobj)
{
  gboolean res;
  GESCustomTimelineSourcePrivate *priv;

  GST_DEBUG ("Calling callback (timelineobj:%p, trackobj:%p, gnlobj:%p)",
      object, trobject, gnlobj);

  priv = GES_CUSTOM_TIMELINE_SOURCE (object)->priv;
  res = priv->filltrackobjectfunc (object, trobject, gnlobj, priv->user_data);

  GST_DEBUG ("Returning res:%d", res);

  return res;
}

/**
 * ges_custom_timeline_source_new:
 * @func: (scope notified): The #GESFillTrackObjectUserFunc that will be used to fill the track
 * objects.
 * @user_data: (closure): a gpointer that will be used when @func is called.
 *
 * Creates a new #GESCustomTimelineSource.
 *
 * Returns: The new #GESCustomTimelineSource.
 */
GESCustomTimelineSource *
ges_custom_timeline_source_new (GESFillTrackObjectUserFunc func,
    gpointer user_data)
{
  GESCustomTimelineSource *src;

  src = g_object_new (GES_TYPE_CUSTOM_TIMELINE_SOURCE, NULL);
  src->priv->filltrackobjectfunc = func;
  src->priv->user_data = user_data;

  return src;
}

/**
 * ges_material_custom_timeline_source_new:
 * @func: (scope notified): The #GESFillTrackObjectUserFunc that will be used to fill the track
 * objects.
 * @user_data: (closure): a gpointer that will be used when @func is called.
 *
 * Helper constructor to instanciate a new #GESMaterial from which you can
 * extract #GESCustomTimelineSource-s
 *
 * Returns: The new #GESMaterial.
 */
GESMaterial *
ges_material_custom_timeline_source_new (GESFillTrackObjectUserFunc func,
    gpointer user_data)
{
  GESMaterial *material;
  gchar *id = g_strdup_printf ("%i!%i", GPOINTER_TO_INT (func),
      GPOINTER_TO_INT (user_data));

  ges_material_new_simple (&material, GES_TYPE_CUSTOM_TIMELINE_SOURCE, id);

  g_free (id);

  return material;
}
