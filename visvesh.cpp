//
// Created by ggovindan on 7/5/18.
//

//
// Created by ggovindan on 7/5/18.
//
#include <gst/gst.h>
#include <iostream>
using namespace std;

static void
on_pad_added (GstElement *element,
              GstPad     *pad,
              gpointer    data)
{
    GstPad *sinkpad;
    GstElement *decoder = (GstElement *) data;

    /* We can now link this pad with the vorbis-decoder sink pad */
    g_print ("Dynamic pad created, linking demuxer/decoder\n");

    sinkpad = gst_element_get_static_pad (decoder, "sink");

    gst_pad_link (pad, sinkpad);

    gst_object_unref (sinkpad);
}

int main(int argc, char* argv[])
{
    GMainLoop *loop;
    GstElement *pipeline, *source, *demuxer, *tsdemux, *h264parse, *vdecoder, *vsink;
    GstElement *aacparse, *adecoder, *aconvert, *asink;
    GstBus *bus;
    int bus_watch_id;
    GstStateChangeReturn ret;

    gst_init(&argc, &argv);
    loop = g_main_loop_new(NULL, FALSE);

    if (argc != 2)
    {
        g_printerr("Usage: %s <http stream source>\n", argv[0]);
        return -1;
    }

    pipeline = gst_pipeline_new("myApp");
    source = gst_element_factory_make("souphttpsrc", "http-src");
    demuxer = gst_element_factory_make("hlsdemux", "hls-demuxer");
    tsdemux = gst_element_factory_make("decodebin", "decodebin");
    h264parse = gst_element_factory_make("h264parse", "h264parse");
    vdecoder = gst_element_factory_make("avdec_h264", "h264decoder");
    vsink = gst_element_factory_make("autovideosink", "videosink");


    /* set the input url to the source element */
    g_object_set(G_OBJECT(source), "location", argv[1], NULL);

    bus = gst_pipeline_get_bus(GST_PIPELINE (pipeline));
    // bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);

    /* add elements into the pipeline */ //next aacparse
    gst_bin_add_many(GST_BIN (pipeline), source, demuxer, tsdemux, h264parse, vdecoder,vsink,  NULL);

    gst_element_link(source, demuxer);
    // this was wrong
    /*gst_element_link_many(tsdemux, h264parse, vdecoder, vsink, NULL);*/


    /* connect demuxer and decoder on pad added */
    /*g_signal_connect(demuxer, "pad-added", G_CALLBACK(on_pad_added), vdecoder);*/

    // Correct Implementation
    gst_element_link_many(h264parse, vdecoder, vsink, NULL);

    g_signal_connect(demuxer, "pad-added", G_CALLBACK(on_pad_added), tsdemux);
    g_signal_connect(tsdemux, "pad-added", G_CALLBACK(on_pad_added), h264parse);
    g_signal_connect(demuxer, "pad-added", G_CALLBACK(on_pad_added), vdecoder);
    // Correct Implementation

    g_print ("Starting play: %s\n", argv[1]);
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        cout << "Unable to set the pipeline to the playing state." << endl;
        gst_object_unref (pipeline);
        return -1;
    }

    g_print ("Running\n");
    g_main_loop_run(loop);

    /* Clean up after execution of main loop */
    g_print ("Stopping Playback: %s\n", argv[1]);
    gst_element_set_state(pipeline, GST_STATE_NULL);

    g_print ("Quitting\n");
    g_object_unref(G_OBJECT(pipeline));
    // g_source_remove(bus_watch_id);

    g_main_loop_unref(loop);

    return 0;
}
