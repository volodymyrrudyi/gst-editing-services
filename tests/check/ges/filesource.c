/* GStreamer Editing Services
 * Copyright (C) 2009 Edward Hervey <bilboed@bilboed.com>
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

#include "test-utils.h"
#include <ges/ges.h>
#include <gst/check/gstcheck.h>

/* This test uri will eventually have to be fixed */
#define TEST_URI "http://nowhere/blahblahblah"


static gchar *av_uri;
GMainLoop *mainloop;

static void
material_created_cb (GESMaterial * material, gchar * id, GError * error,
    GESTimelineLayer * layer)
{
  GList *tracks, *tmp;
  GESTimelineFileSource *tlfs;

  fail_unless (error == NULL);
  fail_if (material == NULL);
  fail_if (g_strcmp0 (ges_material_get_id (material), av_uri));
  fail_if (g_strcmp0 (id, av_uri));

  tlfs = GES_TIMELINE_FILE_SOURCE (ges_timeline_layer_add_material (layer,
          material, 0, 0, GST_CLOCK_TIME_NONE, 1, GES_TRACK_TYPE_UNKNOWN));
  fail_unless (GES_IS_TIMELINE_FILE_SOURCE (tlfs));
  fail_if (g_strcmp0 (ges_timeline_filesource_get_uri (tlfs), av_uri));
  assert_equals_uint64 (GES_TIMELINE_OBJECT_DURATION (tlfs), GST_SECOND);

  fail_unless (ges_timeline_object_get_supported_formats
      (GES_TIMELINE_OBJECT (tlfs)) & GES_TRACK_TYPE_VIDEO);
  fail_unless (ges_timeline_object_get_supported_formats
      (GES_TIMELINE_OBJECT (tlfs)) & GES_TRACK_TYPE_AUDIO);

  tracks = ges_timeline_get_tracks (ges_timeline_layer_get_timeline (layer));
  for (tmp = tracks; tmp; tmp = tmp->next) {
    GList *tckobjs = ges_track_get_objects (GES_TRACK (tmp->data));

    assert_equals_int (g_list_length (tckobjs), 1);
    fail_unless (GES_IS_TRACK_FILESOURCE (tckobjs->data));
    g_list_free_full (tckobjs, gst_object_unref);
  }
  g_list_free_full (tracks, gst_object_unref);

  g_free (id);
  gst_object_unref (material);
  g_main_loop_quit (mainloop);
}

GST_START_TEST (test_filesource_basic)
{
  GESTimeline *timeline;
  GESMaterial *material;
  GESTimelineLayer *layer;

  fail_unless (ges_init ());

  mainloop = g_main_loop_new (NULL, FALSE);

  timeline = ges_timeline_new_audio_video ();
  fail_unless (timeline != NULL);

  layer = ges_timeline_layer_new ();
  fail_unless (layer != NULL);
  fail_unless (ges_timeline_add_layer (timeline, layer));

  fail_unless (ges_material_new (&material, GES_TYPE_TIMELINE_FILE_SOURCE,
          (GESMaterialCreatedCallback) material_created_cb, av_uri, layer),
      GES_MATERIAL_LOADING_ASYNC);
  fail_unless (material == NULL);

  g_main_loop_run (mainloop);
  g_main_loop_unref (mainloop);
  g_object_unref (timeline);
}

GST_END_TEST;

#define gnl_object_check(gnlobj, start, duration, mstart, mduration, priority, active) { \
  guint64 pstart, pdur, pmstart, pmdur, pprio, pact;			\
  g_object_get (gnlobj, "start", &pstart, "duration", &pdur,		\
		"media-start", &pmstart, "media-duration", &pmdur,	\
		"priority", &pprio, "active", &pact,			\
		NULL);							\
  assert_equals_uint64 (pstart, start);					\
  assert_equals_uint64 (pdur, duration);					\
  assert_equals_uint64 (pmstart, mstart);					\
  assert_equals_uint64 (pmdur, mduration);					\
  assert_equals_int (pprio, priority);					\
  assert_equals_int (pact, active);					\
  }


GST_START_TEST (test_filesource_properties)
{
  GESTrack *track;
  GESTrackObject *trackobject;
  GESTimelineObject *object;

  ges_init ();

  track = ges_track_new (GES_TRACK_TYPE_AUDIO, GST_CAPS_ANY);
  fail_unless (track != NULL);

  object = (GESTimelineObject *)
      ges_timeline_filesource_new ((gchar *)
      "crack:///there/is/no/way/this/exists");
  fail_unless (object != NULL);

  /* Set some properties */
  g_object_set (object, "start", (guint64) 42, "duration", (guint64) 51,
      "in-point", (guint64) 12, "supported-formats", GES_TRACK_TYPE_AUDIO,
      NULL);
  assert_equals_uint64 (GES_TIMELINE_OBJECT_START (object), 42);
  assert_equals_uint64 (GES_TIMELINE_OBJECT_DURATION (object), 51);
  assert_equals_uint64 (GES_TIMELINE_OBJECT_INPOINT (object), 12);

  trackobject = ges_timeline_object_create_track_object (object, track);
  ges_timeline_object_add_track_object (object, trackobject);
  fail_unless (trackobject != NULL);
  fail_unless (ges_track_object_set_track (trackobject, track));

  /* Check that trackobject has the same properties */
  assert_equals_uint64 (GES_TRACK_OBJECT_START (trackobject), 42);
  assert_equals_uint64 (GES_TRACK_OBJECT_DURATION (trackobject), 51);
  assert_equals_uint64 (GES_TRACK_OBJECT_INPOINT (trackobject), 12);

  /* And let's also check that it propagated correctly to GNonLin */
  gnl_object_check (ges_track_object_get_gnlobject (trackobject), 42, 51, 12,
      51, 0, TRUE);

  /* Change more properties, see if they propagate */
  g_object_set (object, "start", (guint64) 420, "duration", (guint64) 510,
      "in-point", (guint64) 120, NULL);
  assert_equals_uint64 (GES_TIMELINE_OBJECT_START (object), 420);
  assert_equals_uint64 (GES_TIMELINE_OBJECT_DURATION (object), 510);
  assert_equals_uint64 (GES_TIMELINE_OBJECT_INPOINT (object), 120);
  assert_equals_uint64 (GES_TRACK_OBJECT_START (trackobject), 420);
  assert_equals_uint64 (GES_TRACK_OBJECT_DURATION (trackobject), 510);
  assert_equals_uint64 (GES_TRACK_OBJECT_INPOINT (trackobject), 120);

  /* And let's also check that it propagated correctly to GNonLin */
  gnl_object_check (ges_track_object_get_gnlobject (trackobject), 420, 510, 120,
      510, 0, TRUE);

  /* Test mute support */
  g_object_set (object, "mute", TRUE, NULL);
  gnl_object_check (ges_track_object_get_gnlobject (trackobject), 420, 510, 120,
      510, 0, FALSE);
  g_object_set (object, "mute", FALSE, NULL);
  gnl_object_check (ges_track_object_get_gnlobject (trackobject), 420, 510, 120,
      510, 0, TRUE);

  ges_timeline_object_release_track_object (object, trackobject);

  g_object_unref (object);
  g_object_unref (track);
}

GST_END_TEST;

GST_START_TEST (test_filesource_images)
{
  GESTrackObject *trobj;
  GESTimelineObject *tlobj;
  GESTimelineFileSource *tfs;
  GESTrack *a, *v;

  ges_init ();

  tfs = ges_timeline_filesource_new ((gchar *) TEST_URI);
  g_object_set (G_OBJECT (tfs), "supported-formats",
      (GESTrackType) GES_TRACK_TYPE_AUDIO | GES_TRACK_TYPE_VIDEO, NULL);
  tlobj = GES_TIMELINE_OBJECT (tfs);

  a = ges_track_audio_raw_new ();
  v = ges_track_video_raw_new ();

  /* set the is_image property to true then create a video track object. */
  g_object_set (G_OBJECT (tfs), "is-image", TRUE, NULL);

  /* the returned track object should be an image source */
  trobj = ges_timeline_object_create_track_object (tlobj, v);
  ges_timeline_object_add_track_object (tlobj, trobj);
  fail_unless (GES_IS_TRACK_IMAGE_SOURCE (trobj));

  /* The track holds a reference to the object
   * and the timelinobject holds a reference to the object */
  ASSERT_OBJECT_REFCOUNT (trobj, "Video Track Object", 2);

  ges_track_remove_object (v, trobj);
  ges_timeline_object_release_track_object (tlobj, trobj);

  /* the timeline object should not create any TrackObject in the audio track */
  trobj = ges_timeline_object_create_track_object (tlobj, a);
  fail_unless (trobj == NULL);

  g_object_unref (a);
  g_object_unref (v);
  g_object_unref (tlobj);
}

GST_END_TEST;


static Suite *
ges_suite (void)
{
  Suite *s = suite_create ("ges-filesource");
  TCase *tc_chain = tcase_create ("filesource");

  suite_add_tcase (s, tc_chain);

  tcase_add_test (tc_chain, test_filesource_basic);
  tcase_add_test (tc_chain, test_filesource_images);
  tcase_add_test (tc_chain, test_filesource_properties);

  return s;
}

int
main (int argc, char **argv)
{
  int nf;

  Suite *s = ges_suite ();
  SRunner *sr = srunner_create (s);

  gst_check_init (&argc, &argv);

  av_uri = ges_test_get_audio_video_uri ();

  srunner_run_all (sr, CK_NORMAL);
  nf = srunner_ntests_failed (sr);
  srunner_free (sr);

  g_free (av_uri);

  return nf;
}
