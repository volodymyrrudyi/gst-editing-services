/* GStreamer Editing Services
 * Copyright (C) 2010 Brandon Lewis <brandon.lewis@collabora.co.uk>
 *               2010 Nokia Corporation
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
 * SECTION:ges-formatter
 * @short_description: Timeline saving and loading.
 *
 * === FIXME OBSELETE DOCUMENTATION, TO BE UPDATED WITH PROJECT ===
 * The #GESFormatter is the object responsible for loading and/or saving the contents
 * of a #GESTimeline to/from various formats.
 *
 * In order to save a #GESTimeline, you can either let GES pick a default formatter by
 * using ges_timeline_save_to_uri(), or pick your own formatter and use
 * ges_formatter_save_to_uri().
 *
 * To load a #GESTimeline, you might want to be able to track the progress of the loading,
 * in which case you should create an empty #GESTimeline, connect to the relevant signals
 * and call ges_formatter_load_from_uri().
 *
 * If you do not care about tracking the loading progress, you can use the convenience
 * ges_timeline_new_from_uri() method.
 *
 * Support for saving or loading new formats can be added by creating a subclass of
 * #GESFormatter and implement the various vmethods of #GESFormatterClass.
 *
 * Note that subclasses should call ges_formatter_project_loaded when they are done
 * loading a project.
 **/

#include <gst/gst.h>
#include <gio/gio.h>
#include <stdlib.h>

#include "ges-formatter.h"
#include "ges-keyfile-formatter.h"
#include "ges-internal.h"
#include "ges.h"

static void ges_extractable_interface_init (GESExtractableInterface * iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (GESFormatter, ges_formatter, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (GES_TYPE_EXTRACTABLE,
        ges_extractable_interface_init));

struct _GESFormatterPrivate
{
  gchar *data;
  gsize length;

  /* Make sure not to emit several times "moved-source" when the user already
   * provided the new source URI. */
  GHashTable *uri_newuri_table;
  GHashTable *parent_newparent_table;

  GESProject *project;
};

static void ges_formatter_dispose (GObject * object);
static gboolean load_from_uri (GESFormatter * formatter, GESTimeline *
    timeline, const gchar * uri, GError ** error);
static gboolean save_to_uri (GESFormatter * formatter, GESTimeline *
    timeline, const gchar * uri, GError ** error);
static gboolean default_can_load_uri (const gchar * uri, GError ** error);
static gboolean default_can_save_uri (const gchar * uri, GError ** error);

enum
{
  SOURCE_MOVED_SIGNAL,
  LOADED_SIGNAL,
  LAST_SIGNAL
};

static guint ges_formatter_signals[LAST_SIGNAL] = { 0 };

/* Utils */
static GESFormatterClass *
ges_formatter_find_for_uri (const gchar * uri)
{
  GType *formatters;
  guint n_formatters, i;
  GESFormatterClass *class, *ret = NULL;

  formatters = g_type_children (GES_TYPE_FORMATTER, &n_formatters);
  for (i = 0; i < n_formatters; i++) {
    class = g_type_class_ref (formatters[i]);

    if (class->can_load_uri (uri, NULL)) {
      ret = class;
      break;
    }
    g_type_class_unref (class);
  }

  g_free (formatters);

  return ret;
}

/* GESExtractable implementation */
static gchar *
extractable_check_id (GType type, const gchar * id)
{
  if (gst_uri_is_valid (id))
    return g_strdup (id);

  return NULL;
}

static gchar *
extractable_get_id (GESExtractable * self)
{
  GESMaterial *material;

  if (!(material = ges_extractable_get_material (self)))
    return NULL;

  return g_strdup (ges_material_get_id (material));
}

static GType
extractable_get_real_extractable_type (GType type, const gchar * id)
{
  GType real_type = G_TYPE_NONE;
  GESFormatterClass *class;

  class = ges_formatter_find_for_uri (id);
  if (class) {
    real_type = G_OBJECT_CLASS_TYPE (class);
    g_type_class_unref (class);
  }

  return real_type;
}

static void
ges_extractable_interface_init (GESExtractableInterface * iface)
{
  iface->check_id = (GESExtractableCheckId) extractable_check_id;
  iface->get_id = extractable_get_id;
  iface->get_real_extractable_type = extractable_get_real_extractable_type;
}

static void
ges_formatter_class_init (GESFormatterClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GESFormatterPrivate));

  /**
   * GESFormatter::source-moved:
   * @formatter: the #GESFormatter
   * @source: The #GESTimelineFileSource that has an invalid URI. When this happens,
   * you can call #ges_formatter_update_source_uri with the new URI of the source so
   * the project can be loaded properly.
   */
  ges_formatter_signals[SOURCE_MOVED_SIGNAL] =
      g_signal_new ("source-moved", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_generic, G_TYPE_NONE,
      1, GES_TYPE_TIMELINE_FILE_SOURCE);

  /**
   * GESFormatter::loaded:
   * @formatter: the #GESFormatter that is done loading a project.
   */
  ges_formatter_signals[LOADED_SIGNAL] =
      g_signal_new ("loaded", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_generic, G_TYPE_NONE,
      1, GES_TYPE_TIMELINE);

  object_class->dispose = ges_formatter_dispose;

  klass->can_load_uri = default_can_load_uri;
  klass->can_save_uri = default_can_save_uri;
  klass->load_from_uri = load_from_uri;
  klass->save_to_uri = save_to_uri;
  klass->update_source_uri = NULL;
}

static void
ges_formatter_init (GESFormatter * object)
{
  object->priv = G_TYPE_INSTANCE_GET_PRIVATE (object,
      GES_TYPE_FORMATTER, GESFormatterPrivate);

  object->priv->uri_newuri_table = g_hash_table_new_full (g_str_hash,
      g_str_equal, g_free, g_free);
  object->priv->parent_newparent_table = g_hash_table_new_full (g_file_hash,
      (GEqualFunc) g_file_equal, g_object_unref, g_object_unref);
  object->priv->project = NULL;
}

static void
ges_formatter_dispose (GObject * object)
{
  GESFormatterPrivate *priv = GES_FORMATTER (object)->priv;

  if (priv->data) {
    g_free (priv->data);
  }
  g_hash_table_destroy (priv->uri_newuri_table);
  g_hash_table_destroy (priv->parent_newparent_table);

  ges_formatter_set_project (GES_FORMATTER (object), NULL);
}

/**
 * ges_formatter_new_for_uri:
 * @uri: a #gchar * pointing to the uri
 *
 * Creates a #GESFormatter that can handle the given URI.
 *
 * Returns: A GESFormatter that can load the given uri, or NULL if
 * the uri is not supported.
 */

GESFormatter *
ges_formatter_new_for_uri (const gchar * uri)
{
  if (ges_formatter_can_load_uri (uri, NULL))
    return GES_FORMATTER (ges_keyfile_formatter_new ());

  return NULL;
}

/**
 * ges_default_formatter_new:
 *
 * Creates a new instance of the default GESFormatter type on this system
 * (currently #GESKeyfileFormatter).
 *
 * Returns: (transfer full): a #GESFormatter instance or %NULL
 */

GESFormatter *
ges_default_formatter_new (void)
{
  return GES_FORMATTER (ges_keyfile_formatter_new ());
}

static gboolean
default_can_load_uri (const gchar * uri, GError ** error)
{
  GST_ERROR ("No 'can_load_uri' vmethod implementation");
  return FALSE;
}

static gboolean
default_can_save_uri (const gchar * uri, GError ** error)
{
  GST_ERROR ("No 'can_save_uri' vmethod implementation");
  return FALSE;
}

/**
 * ges_formatter_can_load_uri:
 * @uri: a #gchar * pointing to the URI
 * @error: A #GError that will be set in case of error
 *
 * Checks if there is a #GESFormatter available which can load a #GESTimeline
 * from the given URI.
 *
 * Returns: TRUE if there is a #GESFormatter that can support the given uri
 * or FALSE if not.
 */

gboolean
ges_formatter_can_load_uri (const gchar * uri, GError ** error)
{
  GESFormatterClass *class = NULL;

  if (!(gst_uri_is_valid (uri))) {
    GST_ERROR ("Invalid uri!");
    return FALSE;
  }

  if (!(gst_uri_has_protocol (uri, "file"))) {
    gchar *proto = gst_uri_get_protocol (uri);
    GST_ERROR ("Unspported protocol '%s'", proto);
    g_free (proto);
    return FALSE;
  }

  class = ges_formatter_find_for_uri (uri);
  if (class) {
    g_type_class_unref (class);

    return TRUE;
  }

  return FALSE;
}

/**
 * ges_formatter_can_save_uri:
 * @uri: a #gchar * pointing to a URI
 * @error: A #GError that will be set in case of error
 *
 * Returns TRUE if there is a #GESFormatter available which can save a
 * #GESTimeline to the given URI.
 *
 * Returns: TRUE if the given @uri is supported, else FALSE.
 */

gboolean
ges_formatter_can_save_uri (const gchar * uri, GError ** error)
{
  if (!(gst_uri_is_valid (uri))) {
    GST_ERROR ("%s invalid uri!", uri);
    return FALSE;
  }

  if (!(gst_uri_has_protocol (uri, "file"))) {
    gchar *proto = gst_uri_get_protocol (uri);
    GST_ERROR ("Unspported protocol '%s'", proto);
    g_free (proto);
    return FALSE;
  }

  /* TODO: implement file format registry */
  /* TODO: search through the registry and chose a GESFormatter class that can
   * handle the URI.*/

  return TRUE;
}

/**
 * ges_formatter_set_data:
 * @formatter: a #GESFormatter
 * @data: the data to be set on the formatter
 * @length: the length of the data in bytes
 *
 * Set the data that this formatter will use for loading. The formatter will
 * takes ownership of the data and will free the data if
 * @ges_formatter_set_data is called again or when the formatter itself is
 * disposed. You should call @ges_formatter_clear_data () if you do not wish
 * this to happen.
 */

void
ges_formatter_set_data (GESFormatter * formatter, void *data, gsize length)
{
  GESFormatterPrivate *priv = GES_FORMATTER (formatter)->priv;

  if (priv->data)
    g_free (priv->data);
  priv->data = data;
  priv->length = length;
}

/**
 * ges_formatter_get_data:
 * @formatter: a #GESFormatter
 * @length: location into which to store the size of the data in bytes.
 *
 * Lets you get the data @formatter used for loading.
 *
 * Returns: (transfer none): a pointer to the data.
 */
void *
ges_formatter_get_data (GESFormatter * formatter, gsize * length)
{
  GESFormatterPrivate *priv = GES_FORMATTER (formatter)->priv;

  *length = priv->length;

  return priv->data;
}

/**
 * ges_formatter_clear_data:
 * @formatter: a #GESFormatter
 *
 * clears the data from a #GESFormatter without freeing it. You should call
 * this before disposing or setting data on a #GESFormatter if the current data
 * pointer should not be freed.
 */

void
ges_formatter_clear_data (GESFormatter * formatter)
{
  GESFormatterPrivate *priv = GES_FORMATTER (formatter)->priv;

  priv->data = NULL;
  priv->length = 0;
}

/**
 * ges_formatter_load:
 * @formatter: a #GESFormatter
 * @timeline: a #GESTimeline
 *
 * Loads data from formatter to into timeline. You should first call
 * ges_formatter_set_data() with the location and size of a block of data
 * from which to read.
 *
 * Returns: TRUE if the data was successfully loaded into timeline
 * or FALSE if an error occured during loading.
 */

gboolean
ges_formatter_load (GESFormatter * formatter, GESTimeline * timeline)
{
  GESFormatterClass *klass;

  formatter->timeline = timeline;
  klass = GES_FORMATTER_GET_CLASS (formatter);

  if (klass->load)
    return klass->load (formatter, timeline);
  GST_ERROR ("not implemented!");
  return FALSE;
}

/**
 * ges_formatter_save:
 * @formatter: a #GESFormatter
 * @timeline: a #GESTimeline
 *
 * Save data from timeline into a block of data. You can retrieve the location
 * and size of this data with ges_formatter_get_data().
 *
 * Returns: TRUE if the timeline data was successfully saved for FALSE if
 * an error occured during saving.
 */

gboolean
ges_formatter_save (GESFormatter * formatter, GESTimeline * timeline)
{
  GESFormatterClass *klass;
  GList *layers;

  /* Saving an empty timeline is not allowed */
  /* FIXME : Having a ges_timeline_is_empty() would be more efficient maybe */
  layers = ges_timeline_get_layers (timeline);

  g_return_val_if_fail (layers != NULL, FALSE);
  g_list_foreach (layers, (GFunc) g_object_unref, NULL);
  g_list_free (layers);

  klass = GES_FORMATTER_GET_CLASS (formatter);

  if (klass->save)
    return klass->save (formatter, timeline);

  GST_ERROR ("not implemented!");

  return FALSE;
}

/**
 * ges_formatter_load_from_uri:
 * @formatter: a #GESFormatter
 * @timeline: a #GESTimeline
 * @uri: a #gchar * pointing to a URI
 * @error: A #GError that will be set in case of error
 *
 * Load data from the given URI into timeline.
 *
 * Returns: TRUE if the timeline data was successfully loaded from the URI,
 * else FALSE.
 */

gboolean
ges_formatter_load_from_uri (GESFormatter * formatter, GESTimeline * timeline,
    const gchar * uri, GError ** error)
{
  gboolean ret = FALSE;
  GESFormatterClass *klass = GES_FORMATTER_GET_CLASS (formatter);

  g_return_val_if_fail (GES_IS_FORMATTER (formatter), FALSE);
  g_return_val_if_fail (GES_IS_TIMELINE (timeline), FALSE);

  if (klass->load_from_uri) {
    ges_timeline_enable_update (timeline, FALSE);
    formatter->timeline = timeline;
    ret = klass->load_from_uri (formatter, timeline, uri, error);
    ges_timeline_enable_update (timeline, TRUE);
  }

  return ret;
}

static gboolean
load_from_uri (GESFormatter * formatter, GESTimeline * timeline,
    const gchar * uri, GError ** error)
{
  gchar *location;
  GError *e = NULL;
  gboolean ret = TRUE;
  GESFormatterPrivate *priv = GES_FORMATTER (formatter)->priv;


  if (priv->data) {
    GST_ERROR ("formatter already has data! please set data to NULL");
  }

  if (!(location = gst_uri_get_location (uri))) {
    return FALSE;
  }

  if (g_file_get_contents (location, &priv->data, &priv->length, &e)) {
    if (!ges_formatter_load (formatter, timeline)) {
      GST_ERROR ("couldn't deserialize formatter");
      ret = FALSE;
    }
  } else {
    GST_ERROR ("couldn't read file '%s': %s", location, e->message);
    ret = FALSE;
  }

  if (e)
    g_error_free (e);
  g_free (location);

  return ret;
}

/**
 * ges_formatter_save_to_uri:
 * @formatter: a #GESFormatter
 * @timeline: a #GESTimeline
 * @uri: a #gchar * pointing to a URI
 * @error: A #GError that will be set in case of error
 *
 * Save data from timeline to the given URI.
 *
 * Returns: TRUE if the timeline data was successfully saved to the URI
 * else FALSE.
 */

gboolean
ges_formatter_save_to_uri (GESFormatter * formatter, GESTimeline *
    timeline, const gchar * uri, GError ** error)
{
  GESFormatterClass *klass = GES_FORMATTER_GET_CLASS (formatter);

  if (klass->save_to_uri)
    return klass->save_to_uri (formatter, timeline, uri, error);

  GST_ERROR ("not implemented!");

  return FALSE;
}

static gboolean
save_to_uri (GESFormatter * formatter, GESTimeline * timeline,
    const gchar * uri, GError ** error)
{
  gchar *location;
  GError *e = NULL;
  gboolean ret = TRUE;
  GESFormatterPrivate *priv = GES_FORMATTER (formatter)->priv;

  if (!(location = g_filename_from_uri (uri, NULL, NULL))) {
    return FALSE;
  }

  if (!ges_formatter_save (formatter, timeline)) {
    GST_ERROR ("couldn't serialize formatter");
  } else {
    if (!g_file_set_contents (location, priv->data, priv->length, &e)) {
      GST_ERROR ("couldn't write file '%s': %s", location, e->message);
      ret = FALSE;
    }
  }

  if (e)
    g_error_free (e);
  g_free (location);

  return ret;
}

gboolean
ges_formatter_update_source_uri (GESFormatter * formatter,
    GESTimelineFileSource * source, gchar * new_uri)
{
  GESFormatterClass *klass = GES_FORMATTER_GET_CLASS (formatter);

  if (klass->update_source_uri) {
    const gchar *uri = ges_timeline_filesource_get_uri (source);
    gchar *cached_uri =
        g_hash_table_lookup (formatter->priv->uri_newuri_table, uri);

    if (!cached_uri) {
      GFile *parent, *new_parent, *new_file = g_file_new_for_uri (new_uri),
          *file = g_file_new_for_uri (uri);

      parent = g_file_get_parent (file);
      new_parent = g_file_get_parent (new_file);
      g_hash_table_insert (formatter->priv->uri_newuri_table, g_strdup (uri),
          g_strdup (new_uri));

      g_hash_table_insert (formatter->priv->parent_newparent_table,
          parent, new_parent);

      GST_DEBUG ("Adding %s and its parent to the new uri cache", new_uri);

      g_object_unref (file);
      g_object_unref (new_file);
    }

    return klass->update_source_uri (formatter, source, new_uri);
  }

  GST_ERROR ("not implemented!");

  return FALSE;
}

/*< protected >*/
/**
 * ges_formatter_emit_loaded:
 * @formatter: The #GESFormatter from which to emit the "project-loaded" signal
 *
 * Emits the "loaded" signal. This method should be called by sublasses when
 * the project is fully loaded.
 *
 * Returns: %TRUE if the signale could be emitted %FALSE otherwize
 */
gboolean
ges_formatter_emit_loaded (GESFormatter * formatter)
{
  GST_INFO_OBJECT (formatter, "Emit project loaded");
  g_signal_emit (formatter, ges_formatter_signals[LOADED_SIGNAL], 0,
      formatter->timeline);

  return TRUE;
}

void
ges_formatter_set_project (GESFormatter * formatter, GESProject * project)
{
  if (formatter->priv->project)
    g_object_unref (formatter->priv->project);

  formatter->priv->project = g_object_ref (project);
}

GESProject *
ges_formatter_get_project (GESFormatter * formatter)
{
  return formatter->priv->project;
}
