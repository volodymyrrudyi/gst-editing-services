/* GStreamer Editing Services
 *
 * Copyright (C) 2012 Thibault Saunier <thibault.saunier@collabora.com>
 * Copyright (C) 2012 Volodymyr Rudyi <vladimir.rudoy@gmail.com>
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
 * SECTION: ges-extractable
 * @short_description: An interface letting an object be extracted from a #GESMaterial
 *
 * FIXME: Long description needed
 */
#include "ges-material.h"
#include "ges-internal.h"
#include "ges-extractable.h"
#include "ges-timeline-file-source.h"

static GQuark ges_material_key;

G_DEFINE_INTERFACE_WITH_CODE (GESExtractable, ges_extractable, G_TYPE_OBJECT,
    ges_material_key = g_quark_from_static_string ("ges-extractable-data"));

static gchar *
ges_extractable_check_id_default (GType type, const gchar * id)
{
  return g_strdup (g_type_name (type));
}

static GType
ges_extractable_get_real_extractable_type_default (GType type, const gchar * id)
{
  return type;
}

static GParameter *
extractable_get_parameters_from_id (const gchar * id, guint * n_params)
{
  *n_params = 0;

  return NULL;
}

static gchar *
extractable_get_id (GESExtractable * self)
{
  return g_strdup (g_type_name (G_OBJECT_TYPE (self)));

}

static void
ges_extractable_default_init (GESExtractableInterface * iface)
{
  iface->material_type = GES_TYPE_MATERIAL;
  iface->check_id = ges_extractable_check_id_default;
  iface->get_real_extractable_type =
      ges_extractable_get_real_extractable_type_default;
  iface->get_parameters_from_id = extractable_get_parameters_from_id;
  iface->set_material = NULL;
  iface->get_id = extractable_get_id;
}

/**
 * ges_extractable_object_get_material:
 * @object: Target object
 *
 * Method to get material which was used to instaniate specified object
 *
 * Returns: (transfer none): origin material
 */
GESMaterial *
ges_extractable_get_material (GESExtractable * self)
{
  g_return_val_if_fail (GES_IS_EXTRACTABLE (self), NULL);

  return g_object_get_qdata (G_OBJECT (self), ges_material_key);
}

/**
 * ges_extractable_object_set_material:
 * @object: Target object
 * @material: (transfer none): The #GESMaterial to set
 *
 * Method to set material which was used to instaniate specified object
 */
void
ges_extractable_set_material (GESExtractable * self, GESMaterial * material)
{
  GESExtractableInterface *iface;

  g_return_if_fail (GES_IS_EXTRACTABLE (self));

  GST_DEBUG_OBJECT (self, "Setting material to %" GST_PTR_FORMAT, material);
  if (g_object_get_qdata (G_OBJECT (self), ges_material_key)) {
    GST_WARNING_OBJECT (self, "Can not reset material on object");

    return;
  }

  g_object_set_qdata_full (G_OBJECT (self), ges_material_key,
      gst_object_ref (material), gst_object_unref);

  /* Let classes that implement the interface know that a material has been set */
  iface = GES_EXTRACTABLE_GET_INTERFACE (self);
  if (iface->set_material)
    iface->set_material (self, material);
}

/**
 * ges_extractable_get_id:
 * @self: The #GESExtractable
 *
 * Returns: The #id of the associated #GESMaterial, free with #g_free
 */
gchar *
ges_extractable_get_id (GESExtractable * self)
{
  g_return_val_if_fail (GES_IS_EXTRACTABLE (self), NULL);

  return GES_EXTRACTABLE_GET_INTERFACE (self)->get_id (self);
}

/**
 * ges_extractable_type_get_parameters_for_id:
 * @type: The #GType implementing #GESExtractable
 * @id: The ID of the Extractable
 * @n_params: (out): Return location for the returned array
 *
 * Returns: (transfer full) (array length=n_params): an array of #GParameter
 * needed to extract the #GESExtractable from a #GESMaterial of @id
 */
GParameter *
ges_extractable_type_get_parameters_from_id (GType type, const gchar * id,
    guint * n_params)
{
  GObjectClass *klass;
  GESExtractableInterface *iface;

  GParameter *ret = NULL;

  g_return_val_if_fail (g_type_is_a (type, G_TYPE_OBJECT), NULL);
  g_return_val_if_fail (g_type_is_a (type, GES_TYPE_EXTRACTABLE), NULL);

  klass = g_type_class_ref (type);
  iface = g_type_interface_peek (klass, GES_TYPE_EXTRACTABLE);

  ret = iface->get_parameters_from_id (id, n_params);

  g_type_class_unref (klass);

  return ret;
}

/**
 * ges_extractable_type_get_material_type:
 * @type: The #GType implementing #GESExtractable
 *
 * Get the #GType, subclass of #GES_TYPE_MATERIAL to instanciate
 * to be able to extract a @type
 *
 * Returns: the #GType to use to create a material to extract @type
 */
GType
ges_extractable_type_get_material_type (GType type)
{
  GObjectClass *klass;
  GESExtractableInterface *iface;

  g_return_val_if_fail (g_type_is_a (type, G_TYPE_OBJECT), G_TYPE_INVALID);
  g_return_val_if_fail (g_type_is_a (type, GES_TYPE_EXTRACTABLE),
      G_TYPE_INVALID);

  klass = g_type_class_ref (type);

  iface = g_type_interface_peek (klass, GES_TYPE_EXTRACTABLE);

  g_type_class_unref (klass);

  return iface->material_type;
}

/**
 * ges_extractable_type_check_id:
 * @type: The #GType implementing #GESExtractable
 * @id: The ID to check
 *
 * Check if @id is valid for @type
 *
 * Returns: (transfer full): A newly allocated string containing the actuall
 * ID (after some processing) or %NULL if the ID is wrong.
 */
gchar *
ges_extractable_type_check_id (GType type, const gchar * id)
{
  GObjectClass *klass;
  GESExtractableInterface *iface;

  g_return_val_if_fail (g_type_is_a (type, G_TYPE_OBJECT), G_TYPE_INVALID);
  g_return_val_if_fail (g_type_is_a (type, GES_TYPE_EXTRACTABLE),
      G_TYPE_INVALID);

  klass = g_type_class_ref (type);

  iface = g_type_interface_peek (klass, GES_TYPE_EXTRACTABLE);

  g_type_class_unref (klass);

  return iface->check_id (type, id);
}

/**
 * ges_extractable_get_real_extractable_type:
 * @type: The #GType implementing #GESExtractable
 * @id: The ID to check
 *
 * Get the #GType that should be used as extractable_type for @type and
 * @id. Usually this will be the same as @type but in some cases they can
 * be some subclasses of @type. For example, in the case of #GESFormatter,
 * the returned #GType will be a subclass of #GESFormatter that can be used
 * to load the file pointed by @id.
 *
 * Returns: Return the #GESExtractable type that should be used for @id
 */
GType
ges_extractable_get_real_extractable_type_for_id (GType type, const gchar * id)
{
  GType ret;
  GObjectClass *klass;
  GESExtractableInterface *iface;

  klass = g_type_class_ref (type);
  iface = g_type_interface_peek (klass, GES_TYPE_EXTRACTABLE);
  g_type_class_unref (klass);

  ret = iface->get_real_extractable_type (type, id);

  GST_DEBUG ("Extractable type for id %s and wanted type %s is: %s",
      id, g_type_name (type), g_type_name (ret));

  return ret;
}
