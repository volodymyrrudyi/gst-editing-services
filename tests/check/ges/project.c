/**
 * Gstreamer
 *
 * Copyright (C) <2012> Thibault Saunier <thibault.saunier@collabora.com>
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

#include <ges/ges.h>
#include <gst/check/gstcheck.h>

GST_START_TEST (test_project_simple)
{
  gchar *id;
  GESProject *project;
  GESTimeline *timeline;

  ges_init ();

  fail_unless (ges_material_new_simple ((GESMaterial **) & project,
          GES_TYPE_TIMELINE, NULL) == GES_MATERIAL_LOADING_OK);
  fail_unless (GES_IS_PROJECT (project));
  assert_equals_string (ges_material_get_id (GES_MATERIAL (project)),
      "project-0");

  timeline = GES_TIMELINE (ges_material_extract (GES_MATERIAL (project), NULL));
  fail_unless (GES_IS_TIMELINE (timeline));
  id = ges_extractable_get_id (GES_EXTRACTABLE (timeline));
  assert_equals_string (id, "project-0");

  g_free (id);
}

GST_END_TEST;

static void
material_removed_add_cb (GESProject * project, GESMaterial * material,
    gboolean * called)
{
  *called = TRUE;
}

GST_START_TEST (test_project_add_materials)
{
  GESProject *project;
  GESMaterial *material;
  gboolean added_cb_called = FALSE;
  gboolean removed_cb_called = FALSE;

  ges_init ();

  fail_unless (ges_material_new_simple ((GESMaterial **) & project,
          GES_TYPE_TIMELINE, NULL) == GES_MATERIAL_LOADING_OK);
  fail_unless (GES_IS_PROJECT (project));
  assert_equals_string (ges_material_get_id (GES_MATERIAL (project)),
      "project-0");

  g_signal_connect (project, "material-added",
      (GCallback) material_removed_add_cb, &added_cb_called);
  g_signal_connect (project, "material-removed",
      (GCallback) material_removed_add_cb, &removed_cb_called);

  fail_unless (ges_material_new_simple (&material,
          GES_TYPE_TIMELINE_TEST_SOURCE, NULL) == GES_MATERIAL_LOADING_OK);
  fail_unless (GES_IS_MATERIAL (material));

  fail_unless (ges_project_add_material (project, material));
  fail_unless (added_cb_called);
  ASSERT_OBJECT_REFCOUNT (project, "The project", 1);
  ASSERT_OBJECT_REFCOUNT (material, "The material (1 for project and one for "
      "main code)", 2);

  fail_unless (ges_project_remove_material (project, material));
  fail_unless (removed_cb_called);
  ASSERT_OBJECT_REFCOUNT (material, "The material (only our ref remaining)", 1);

  gst_object_unref (project);
  gst_object_unref (material);
}

GST_END_TEST;

/*GST_START_TEST (test_project_load_xptv)*/
/*{*/
/*GESProject *project;*/

/*fail_unless (ges_material_new_simple ((GESMaterial **) & project,*/
/*GES_TYPE_TIMELINE, "") == GES_MATERIAL_LOADING_OK);*/
/*fail_unless (GES_IS_PROJECT (project));*/

/*g_signal_connect (project, "material-added", (GCallback) material_re,*/
/*&added_cb_called);*/

/*fail_unless (ges_material_new_simple (&material,*/
/*GES_TYPE_TIMELINE_TEST_SOURCE, NULL) == GES_MATERIAL_LOADING_OK);*/
/*fail_unless (GES_IS_MATERIAL (material));*/

/*fail_unless (ges_project_add_material (project, material));*/
/*fail_unless (added_cb_called);*/

/*ASSERT_OBJECT_REFCOUNT (project, "The project", 1);*/
/*ASSERT_OBJECT_REFCOUNT (material, "The material (1 for project and one for "*/
/*"main code)", 2);*/

/*gst_object_unref (project);*/
/*ASSERT_OBJECT_REFCOUNT (material, "The material (only our ref remaining)", 1);*/
/*gst_object_unref (material);*/
/*}*/
/*GST_END_TEST;*/

static Suite *
ges_suite (void)
{
  Suite *s = suite_create ("ges-project");
  TCase *tc_chain = tcase_create ("project");

  suite_add_tcase (s, tc_chain);

  tcase_add_test (tc_chain, test_project_simple);
  tcase_add_test (tc_chain, test_project_add_materials);

  return s;
}

int
main (int argc, char **argv)
{
  int nf;

  Suite *s = ges_suite ();
  SRunner *sr = srunner_create (s);

  gst_check_init (&argc, &argv);
  ges_init ();

  srunner_run_all (sr, CK_NORMAL);
  nf = srunner_ntests_failed (sr);
  srunner_free (sr);

  return nf;
}
