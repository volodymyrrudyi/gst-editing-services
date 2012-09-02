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
};

/* GESMaterial vmethod implementation */
static GESMaterialLoadingReturn
ges_project_start_loading (GESMaterial * material)
{
  if (ges_formatter_can_load_uri (ges_material_get_id (material), NULL))
    return GES_MATERIAL_LOADING_ASYNC;

  return GES_MATERIAL_LOADING_OK;
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

  object_class->finalize = _finalize;
  GES_MATERIAL_CLASS (klass)->start_loading = ges_project_start_loading;
}

static void
ges_project_init (GESProject * self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      GES_TYPE_PROJECT, GESProjectPrivate);

  self->priv->materials = g_hash_table_new_full (g_str_hash, g_str_equal,
      g_free, gst_object_unref);
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
ges_project_remove_material (GESProject * self, const gchar * id)
{
  g_return_val_if_fail (GES_IS_PROJECT (self), FALSE);

  return g_hash_table_remove (self->priv->materials, (gpointer) id);
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
