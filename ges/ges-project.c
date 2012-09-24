/* GStreamer Editing Services
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
* SECTION: ges-project
* @short_description: An object that is used to manage materials of a project
*
* FIXME: Long description needed
*/
#include "ges.h"
#include "ges-internal.h"

G_DEFINE_TYPE (GESProject, ges_project, GES_TYPE_MATERIAL);

struct _GESProjectPrivate
{
  GHashTable *materials;
  GESMaterial *formatter_material;
};

enum
{
  MATERIAL_ADDED_SIGNAL,
  MATERIAL_REMOVED_SIGNAL,
  LAST_SIGNAL
};

static guint _signals[LAST_SIGNAL] = { 0 };

/* GESMaterial vmethod implementation */
static GESExtractable *
ges_project_extract (GESMaterial * project, GError ** error)
{
  GESProjectPrivate *priv = GES_PROJECT (project)->priv;
  GESTimeline *timeline = ges_timeline_new ();
  GError *lerr = NULL;


  if (ges_material_new_simple (&priv->formatter_material, GES_TYPE_FORMATTER,
          ges_material_get_id (project)) == GES_MATERIAL_LOADING_OK) {
    GESFormatter *formatter =
        GES_FORMATTER (ges_material_extract (priv->formatter_material, &lerr));

    if (lerr) {
      GST_WARNING_OBJECT (project, "Could not create the formatter: %s",
          (*error)->message);
      gst_object_unref (timeline);
      g_propagate_error (error, lerr);

      return NULL;
    }

    ges_formatter_set_project (formatter, GES_PROJECT (project));
    ges_formatter_load_from_uri (formatter, timeline,
        ges_material_get_id (project), &lerr);

    if (lerr) {
      GST_WARNING_OBJECT (project, "Could not load the timeline, returning");
      gst_object_unref (timeline);
      g_propagate_error (error, lerr);

      return NULL;
    }

  } else {
    GST_LOG_OBJECT (project, "No way to load the timeline... returning an "
        "empty timeline");
  }

  return GES_EXTRACTABLE (timeline);
}

/* GObject vmethod implementation */
static void
_finalize (GObject * object)
{
  GESProjectPrivate *priv = GES_PROJECT (object)->priv;

  if (priv->materials)
    g_hash_table_unref (priv->materials);

  G_OBJECT_CLASS (ges_project_parent_class)->finalize (object);
}

static void
ges_project_class_init (GESProjectClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GESProjectPrivate));

  /**
   * GESProject::material-added:
   * @formatter: the #GESProject
   * @material: The #GESMaterial that has been added to @project
   */
  _signals[MATERIAL_ADDED_SIGNAL] =
      g_signal_new ("material-added", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_generic, G_TYPE_NONE,
      1, GES_TYPE_MATERIAL);

  /**
   * GESProject::material-removed:
   * @formatter: the #GESProject
   * @material: The #GESMaterial that has been removed from @project
   */
  _signals[MATERIAL_REMOVED_SIGNAL] =
      g_signal_new ("material-removed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_generic, G_TYPE_NONE,
      1, GES_TYPE_MATERIAL);

  object_class->finalize = _finalize;
  GES_MATERIAL_CLASS (klass)->extract = ges_project_extract;
}

static void
ges_project_init (GESProject * self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      GES_TYPE_PROJECT, GESProjectPrivate);

  self->priv->materials = g_hash_table_new_full (g_str_hash, g_str_equal,
      g_free, gst_object_unref);
  self->priv->formatter_material = NULL;
}

/**
 * ges_project_add_material:
 * @self: A #GESProject
 * @material: (transfer none): A #GESMaterial to add to @project
 *
 * Adds a #Material to @project, the project will keep a reference on
 * @material.
 *
 * Returns: %TRUE if the material could be added %FALSE otherwise
 */
gboolean
ges_project_add_material (GESProject * self, GESMaterial * material)
{
  g_return_val_if_fail (GES_IS_PROJECT (self), FALSE);

  g_hash_table_insert (self->priv->materials,
      g_strdup (ges_material_get_id (material)), gst_object_ref (material));

  g_signal_emit (self, _signals[MATERIAL_ADDED_SIGNAL], 0, material);

  return TRUE;
}

/**
 * ges_project_remove_material:
 * @self: A #GESProject
 * @material: (transfer none): A #GESMaterial to remove from @project
 *
 * remove a @material to from @project.
 *
 * Returns: %TRUE if the material could be removed %FALSE otherwise
 */
gboolean
ges_project_remove_material (GESProject * self, GESMaterial * material)
{
  gboolean ret;

  g_return_val_if_fail (GES_IS_PROJECT (self), FALSE);

  ret = g_hash_table_remove (self->priv->materials,
      ges_material_get_id (material));

  g_signal_emit (self, _signals[MATERIAL_REMOVED_SIGNAL], 0, material);

  return ret;
}

/**
 * ges_project_list_materials:
 * @self: A #GESProject
 * @type: Type of materials to list, #GES_TYPE_EXTRACTABLE will list
 * all materials
 *
 * List all @material contained in @project filtering per extractable_type
 * as defined by @filter. It copies the material and thus will not be updated
 * in time.
 *
 * Returns: (transfer full) (element-type GESMaterial): The list of
 * #GESMaterial the object contains
 */
GList *
ges_project_list_materials (GESProject * self, GType filter)
{
  GList *ret = NULL;
  GHashTableIter iter;
  gpointer key, value;

  g_return_val_if_fail (GES_IS_PROJECT (self), FALSE);

  g_hash_table_iter_init (&iter, self->priv->materials);
  while (g_hash_table_iter_next (&iter, &key, &value)) {
    if (g_type_is_a (ges_material_get_extractable_type (GES_MATERIAL (value)),
            filter))
      ret = g_list_append (ret, g_object_ref (value));
  }

  return ret;
}

gboolean
ges_project_save (GESProject * self,
    const gchar * uri, GType formatter_type, GError ** error)
{
  return TRUE;
}
