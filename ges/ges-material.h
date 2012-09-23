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

#ifndef _GES_MATERIAL_
#define _GES_MATERIAL_

#include <glib-object.h>
#include <ges/ges-types.h>
#include <ges/ges-enums.h>
#include <gio/gio.h>
#include <gst/gst.h>

G_BEGIN_DECLS
#define GES_TYPE_MATERIAL ges_material_get_type()
#define GES_MATERIAL(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_MATERIAL, GESMaterial))
#define GES_MATERIAL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_MATERIAL, GESMaterialClass))
#define GES_IS_MATERIAL(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_MATERIAL))
#define GES_IS_MATERIAL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_MATERIAL))
#define GES_MATERIAL_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_MATERIAL, GESMaterialClass))

/**
 * GESMaterialCreatedCallback:
 * @material: the #newly created #GESMaterial or %NULL if something went wrong
 * @error: The #GError filled if previsouly provided in the constructor or %NULL
 * @user_data: The user data pointer
 *
 * A function that will be called when a #GESMaterial is ready to be used.
 */
typedef void (*GESMaterialCreatedCallback)(GESMaterial *material, GError *error, gpointer user_data);

typedef struct _GESMaterialPrivate GESMaterialPrivate;

GType ges_material_get_type (void);

typedef enum
{
  GES_MATERIAL_LOADING_ERROR,
  GES_MATERIAL_LOADING_ASYNC,
  GES_MATERIAL_LOADING_OK
} GESMaterialLoadingReturn;

struct _GESMaterial
{
  GObject parent;

  /* <private> */
  GESMaterialPrivate *priv;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

struct _GESMaterialClass
{
  GObjectClass parent;

  GESMaterialLoadingReturn (*start_loading) (GESMaterial *self);
  GESExtractable* (*extract)(GESMaterial *self);

  gpointer _ges_reserved[GES_PADDING];
};

GType
ges_material_get_extractable_type     (GESMaterial * self);

GESMaterialLoadingReturn
ges_material_new                      (GESMaterial **material,
                                       GType extractable_type,
                                       GESMaterialCreatedCallback callback,
                                       const gchar * id,
                                       gpointer user_data);
GESMaterialLoadingReturn
ges_material_new_simple               (GESMaterial **material,
                                       GType extractable_type,
                                       const gchar * id);
const gchar *
ges_material_get_id                   (GESMaterial* self);

GESExtractable *
ges_material_extract                  (GESMaterial *self);

G_END_DECLS
#endif /* _GES_MATERIAL */
