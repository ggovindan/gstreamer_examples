#include <gst/gst.h>
#include <iostream>
using namespace std;

static GMainLoop *loop;
static GstBus *bus;
static GstElement *pipeline, *source, *hlsdemux, *decodebin, *queue1, *videoconvert, *x264enc, *audioconvert, *mpegtsmux, *voaacenc, *file_sink;

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
    decodebin = gst_element_factory_make ("decodebin", "decodebin");
    queue1 = gst_element_factory_make ("queue", "queue1");
    videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    x264enc = gst_element_factory_make("x264enc", "x264enc");
    audioconvert = gst_element_factory_make("audioconvert", "audioconvert");
    voaacenc = gst_element_factory_make("voaacenc", "voaacenc");
    mpegtsmux = gst_element_factory_make("mpegtsmux", "mpegtsmux");
    file_sink = gst_element_factory_make ("filesink", "file_sink");

    //check for null objects
    if (!pipeline || !source || !decodebin || !queue1  || !videoconvert  || !x264enc || !audioconvert || !voaacenc || !mpegtsmux || !file_sink) {
    // if (!pipeline || !source || !file_sink) {
        cout << "not all elements created: pipeline"<<endl;
        return -1;
    }

    //set video source
    g_object_set(G_OBJECT (source), "location", argv[1], NULL);
    cout << "==>Set video source." << endl;

    g_object_set(G_OBJECT (file_sink), "location", "test1.ts", NULL);

    bus = gst_pipeline_get_bus(GST_PIPELINE (pipeline));
    gst_object_unref(bus);

    //add all elements together
    gst_bin_add_many(GST_BIN(pipeline), source, decodebin, videoconvert, queue1, x264enc, audioconvert, voaacenc, mpegtsmux, file_sink, NULL);

    // gst-launch-1.0 --gst-debug=3 mpegtsmux name=mux ! filesink location=mariamma.ts souphttpsrc location=<> ! \
    // decodebin name=decode  ! videoconvert  ! queue ! x264enc ! mux. decode. ! audioconvert ! voaacenc ! mux.

    if (!gst_element_link_many(source, decodebin,  NULL)) {
        g_error("could not link source with decodebin");
    }


    if (!gst_element_link_many(videoconvert, queue1, NULL)) {
        g_error("could not link videoconvert, queue1 !!");
    }

    if (!gst_element_link_many(queue1, x264enc, NULL)) {
        g_error ("could not link queue2, autoaudiosink");
    }

    if (!gst_element_link_many(audioconvert, voaacenc, NULL)) {
        g_error ("could not link audioconvert, voaacenc");
    }

    //mux pipes
    if (!gst_element_link_many(mpegtsmux, file_sink, NULL)) {
        g_error ("could not link mpegtsmux, file_sink");
    }

    if (!gst_element_link_many(voaacenc, mpegtsmux, NULL)) {
        g_error ("could not link voaacenc, mpegtsmux");
    }

    if (!gst_element_link_many(x264enc, mpegtsmux, NULL)) {
        g_error ("could not link x264enc, mpegtsmux");
    }
    // g_signal_connect(hlsdemux, "pad-added", G_CALLBACK(on_pad_added), decodebin);
    g_signal_connect(decodebin, "pad-added", G_CALLBACK(on_pad_added), videoconvert);
    g_signal_connect(decodebin, "pad-added", G_CALLBACK(on_pad_added), audioconvert);



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