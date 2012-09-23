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

  project =
      GES_PROJECT (ges_material_new_simple (GES_TYPE_TIMELINE, "timeline1"));
  fail_unless (GES_IS_PROJECT (project));
  fail_if (g_strcmp0 (ges_material_get_id (GES_MATERIAL (project)),
          "timeline1"));

  timeline = GES_TIMELINE (ges_material_extract (GES_MATERIAL (project)));
  fail_unless (GES_IS_TIMELINE (timeline));
  id = ges_extractable_get_id (GES_EXTRACTABLE (timeline));
  fail_if (g_strcmp0 (id, "timeline1"));

  g_free (id);
}

GST_END_TEST;

static Suite *
ges_suite (void)
{
  Suite *s = suite_create ("ges-project");
  TCase *tc_chain = tcase_create ("project");

  suite_add_tcase (s, tc_chain);

  tcase_add_test (tc_chain, test_project_simple);

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
