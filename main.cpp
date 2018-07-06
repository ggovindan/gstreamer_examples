#include <gst/gst.h>
#include <iostream>
using namespace std;

static GMainLoop *loop;
static GstBus *bus;
static GstElement *pipeline, *source, *hlsdemux, *decodebin, *queue1, *queue2, *autovideosink, *autoaudiosink, *file_sink;

static gboolean
message_cb (GstBus * bus, GstMessage * message, gpointer user_data)
{
    switch (GST_MESSAGE_TYPE (message)) {
        case GST_MESSAGE_ERROR:{
            GError *err = NULL;
            gchar *name, *debug = NULL;

            name = gst_object_get_path_string (message->src);
            gst_message_parse_error (message, &err, &debug);

            g_printerr ("ERROR: from element %s: %s\n", name, err->message);
            if (debug != NULL)
                g_printerr ("Additional debug info:\n%s\n", debug);

            g_error_free (err);
            g_free (debug);
            g_free (name);

            g_main_loop_quit (loop);
            break;
        }
        case GST_MESSAGE_WARNING:{
            GError *err = NULL;
            gchar *name, *debug = NULL;

            name = gst_object_get_path_string (message->src);
            gst_message_parse_warning (message, &err, &debug);

            g_printerr ("ERROR: from element %s: %s\n", name, err->message);
            if (debug != NULL)
                g_printerr ("Additional debug info:\n%s\n", debug);

            g_error_free (err);
            g_free (debug);
            g_free (name);
            break;
        }
        case GST_MESSAGE_EOS:{
            g_print ("Got EOS\n");
            g_main_loop_quit (loop);
            gst_element_set_state (pipeline, GST_STATE_NULL);
            g_main_loop_unref (loop);
            gst_object_unref (pipeline);
            exit(0);
            break;
        }
        default:
            break;
    }

    return TRUE;
}

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

int sigintHandler(int unused) {
    g_print("You ctrl-c-ed!");
    gst_element_send_event(pipeline, gst_event_new_eos());
    return 0;
}

int main(int argc, char *argv[]) {
//    GstElement *pipeline, *source, *hlsdemux, *decodebin, *queue1, *queue2, *autovideosink, *autoaudiosink, *file_sink;
//    GstBus *bus;
    GstMessage *msg;
//    GMainLoop *loop;
    GstStateChangeReturn ret;
    //initialize all elements
    gst_init(&argc, &argv);
    pipeline = gst_pipeline_new ("pipeline");
    source = gst_element_factory_make ("souphttpsrc", "source");
    hlsdemux = gst_element_factory_make ("hlsdemux", "hlsdemux");
    decodebin = gst_element_factory_make ("decodebin", "decodebin");
    queue1 = gst_element_factory_make ("queue", "queue1");
    queue2 = gst_element_factory_make ("queue", "queue2");
    autovideosink = gst_element_factory_make ("autovideosink", "autovideosink");
    autoaudiosink = gst_element_factory_make ("autoaudiosink", "autoaudiosink");
    file_sink = gst_element_factory_make ("filesink", "file_sink");

    //check for null objects
    if (!pipeline || !source || !hlsdemux || !decodebin || !queue1 || !queue2 || !autovideosink  || !autoaudiosink || !file_sink) {
    // if (!pipeline || !source || !file_sink) {
        cout << "not all elements created: pipeline"<<endl;
        return -1;
    }

    //set video source
    g_object_set(G_OBJECT (source), "location", argv[1], NULL);
    cout << "==>Set video source." << endl;

    bus = gst_pipeline_get_bus(GST_PIPELINE (pipeline));
    gst_object_unref(bus);

    //add all elements together
    gst_bin_add_many(GST_BIN(pipeline), source, hlsdemux, decodebin, queue1, queue2, autovideosink, autoaudiosink, NULL);

    // gst-launch-1.0 -v souphttpsrc location=<HLS_URL> ! hlsdemux ! decodebin name=decoder ! queue ! autovideosink decoder. ! queue ! autoaudiosink

    if (!gst_element_link_many(source, hlsdemux,  NULL)) {
        g_error("could not link source with hlsdemux");
    }


    if (!gst_element_link_many(queue1, autovideosink, NULL)) {
        g_error("could not link queue1, autovideosink !!");
    }

    if (!gst_element_link_many(queue2, autoaudiosink, NULL)) {
        g_error ("could not link queue2, autoaudiosink");
    }

    g_signal_connect(hlsdemux, "pad-added", G_CALLBACK(on_pad_added), decodebin);
    g_signal_connect(decodebin, "pad-added", G_CALLBACK(on_pad_added), queue1);
    g_signal_connect(decodebin, "pad-added", G_CALLBACK(on_pad_added), queue2);



    g_print ("Starting play: %s\n", argv[1]);
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    cout << "ret="<<ret<<endl;

    //get pipeline's bus
    bus = gst_element_get_bus (pipeline);
    cout << "==>Setup bus." << endl;

    gst_bus_add_signal_watch(bus);

    loop = g_main_loop_new(NULL, FALSE);
    //set the pipeline state to playing
    ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        cout << "Unable to set the pipeline to the playing state." << endl;
        gst_object_unref (pipeline);
        return -1;
    }
    cout << "==>Set video to play." << endl;

    cout << "==>Begin stream." << endl;
    g_main_loop_run(loop);
    g_signal_connect(G_OBJECT(bus), "message", G_CALLBACK(message_cb), NULL);
    g_main_loop_unref(loop);
    gst_object_unref (bus);
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);
}

// gst-launch-1.0 -v souphttpsrc location=<HLS_URL> ! hlsdemux ! decodebin name=decoder ! queue ! autovideosink decoder. ! queue ! autoaudiosink