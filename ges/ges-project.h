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
#ifndef _GES_PROJECT_
#define _GES_PROJECT_

#include <glib-object.h>
#include <gio/gio.h>
#include <ges/ges-types.h>
#include <ges/ges-material.h>

G_BEGIN_DECLS
#define GES_TYPE_PROJECT ges_project_get_type()
#define GES_PROJECT(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_PROJECT, GESProject))
#define GES_PROJECT_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_PROJECT, GESProjectClass))
#define GES_IS_PROJECT(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_PROJECT))
#define GES_IS_PROJECT_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_PROJECT))
#define GES_PROJECT_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_PROJECT, GESProjectClass))

typedef struct _GESProjectPrivate GESProjectPrivate;

GType ges_project_get_type (void);

struct _GESProject
{
  /* FIXME or GstObject? Does it have a parent? It has a name... */
  GESMaterial parent;

  /* <private> */
  GESProjectPrivate *priv;

  /* Padding for API extension */
  gpointer __ges_reserved[GES_PADDING_LARGE];
};

struct _GESProjectClass
{
  GESMaterialClass parent_class;

  gpointer _ges_reserved[GES_PADDING];
};

gboolean
ges_project_add_material     (GESProject* self,
                              GESMaterial *material);
gboolean
ges_project_remove_material  (GESProject *self,
                              GESMaterial * material);
GList *
ges_project_list_materials   (GESProject * self,
                              GType filter);
gboolean
ges_project_save             (GESProject * self,
                              const gchar *uri,
                              GType formatter_type,
                              GError **error);

G_END_DECLS

#endif  /* _GES_PROJECT */
