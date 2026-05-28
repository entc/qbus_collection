#include "qbus_mqtt.h"
#include "qbus.h"
#include "qbus_frame.h"

// cape includes
#include <sys/cape_log.h>
#include <aio/cape_aio_timer.h>
#include <sys/cape_mutex.h>
#include <fmt/cape_json.h>

#include <MQTTClient.h>
#define MQTT_TIMEOUT 5000L  // 10 seconds

//------------------------------------------------------------------------------------------------------

#define MQTT_TOPIC_PRE__MQTT_ALL  'M'
#define MQTT_TOPIC_PRE__MQTT_CID  'N'
#define MQTT_TOPIC_PRE__BY_ID     'A'
#define MQTT_TOPIC_PRE__CTRL      'C'
#define MQTT_TOPIC_PRE__VALUE     'V'

//------------------------------------------------------------------------------------------------------

struct QbusPvdConnection_s
{
  MQTTClient client;
  QbusPvdCtx ctx;

  void* user_ptr;

  // callbacks
  fct_qbus_pvd__on_con on_con;
  fct_qbus_pvd__on_snd on_snd;

};

//-----------------------------------------------------------------------------

#if defined __WINDOWS_OS

#include <windows.h>

// fix for linking mariadb library
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "shlwapi.lib")

//------------------------------------------------------------------------------------------------------

// this might work?
/*
extern "C" BOOL WINAPI DllMain (HINSTANCE const instance, DWORD const reason, LPVOID const reserved)
{
  // Perform actions based on the reason for calling.
  switch (reason)
  {
    case DLL_PROCESS_ATTACH:
    {

      break;
    }
    case DLL_THREAD_ATTACH:
    {

      break;
    }
    case DLL_THREAD_DETACH:
    {

      break;
    }
    case DLL_PROCESS_DETACH:
    {

      break;
    }
  }

  return TRUE;  // Successful DLL_PROCESS_ATTACH.
}
*/
//------------------------------------------------------------------------------------------------------

#else

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

//------------------------------------------------------------------------------------------------------

void __attribute__ ((constructor)) library_init (void)
{
}

//------------------------------------------------------------------------------------------------------

void __attribute__ ((destructor)) library_fini (void)
{
}

#endif

//------------------------------------------------------------------------------------------------------

static void handle_sigint (int sig)
{
    (void)sig;   // avoid unused warning
}

//------------------------------------------------------------------------------------------------------

int __STDCALL qbus_pvd_init (CapeErr err)
{
#if defined __LINUX_OS || defined __BSD_OS

    struct sigaction sa;

    // initialize the sigaction struct
    memset(&sa, 0, sizeof(sa));

    // set custom signal handler function
    sa.sa_handler = handle_sigint;

    // reset options
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if ((-1 == sigaction(SIGINT, &sa, NULL)) || (-1 == sigaction(SIGTERM, &sa, NULL)))
    {
        return cape_err_lastOSError (err);
    }

    return CAPE_ERR_NONE;

#endif
}

//------------------------------------------------------------------------------------------------------

void __STDCALL qbus_pvd_done (void)
{

}

//------------------------------------------------------------------------------------------------------

struct QbusPvdCtx_s
{
    CapeAioContext aio;
    CapeString cid;              // client id
    CapeString name;             // name of the module
    CapeUdc methods;             // a list of all methods of the module

    CapeMutex mutex;             // a standard mutex

    CapeList reconnect_pool;     // a list to store reconnect items
    CapeList connection_pool;    // a list to store all connection
};

//------------------------------------------------------------------------------------------------------

CapeStream qbus_pvd_ctx__internal__generate_payload (QbusPvdCtx self, number_t event_type)
{
    CapeStream ret = NULL;
    CapeUdc payload_node = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_s_cp (payload_node, "cid", self->cid);
    cape_udc_add_s_cp (payload_node, "name", self->name);
    cape_udc_add_b (payload_node, "direct", TRUE);
    cape_udc_add_n (payload_node, "type", event_type);
    
    switch (event_type)
    {
        case 1:  // new module
        {
            if (self->methods)
            {
                CapeUdc h = cape_udc_cp (self->methods);
                cape_udc_add_name (payload_node, &h, "methods");
            }
            
            break;
        }
    }
    
    ret = cape_json_to_stream (payload_node);

    cape_udc_del (&payload_node);
    return ret;
}

//------------------------------------------------------------------------------------------------------

void qbus_pvd_ctx__internal__publish_connection (QbusPvdCtx self, QbusPvdConnection connection, char pre, const CapeString cid)
{
  int res;

  // local objects
  CapeStream payload_stream = qbus_pvd_ctx__internal__generate_payload (self, 1);
  CapeString subscriber_topic = cape_str_fmt ("%c/%s", pre, cid);

  MQTTClient_message mqtt_msg = MQTTClient_message_initializer;
  MQTTClient_deliveryToken token;

  mqtt_msg.payload = (void*)cape_stream_data (payload_stream);
  mqtt_msg.payloadlen = (int)cape_stream_size (payload_stream);
  mqtt_msg.qos = 1;
  mqtt_msg.retained = 0;

  //printf ("publish message to %s\n", subscriber_topic);

  // send away
  res = MQTTClient_publishMessage (connection->client, subscriber_topic, &mqtt_msg, &token);
  if (MQTTCLIENT_SUCCESS != res)
  {
    cape_log_msg (CAPE_LL_ERROR, "MQTT", "public message", MQTTClient_strerror (res));
  }

  cape_stream_del (&payload_stream);
  cape_str_del (&subscriber_topic);
}

//------------------------------------------------------------------------------------------------------

int on_message (void* user_ptr, char* topicName, int topicLen, MQTTClient_message* message)
{
  QbusPvdConnection self = user_ptr;

  // debug output
  //printf ("on message [%s]: %s\n", topicName, message->payload);

  switch (topicName[0])
  {
    case MQTT_TOPIC_PRE__MQTT_ALL:
    {
      // parse into a json object
      CapeUdc payload_node = cape_json_from_buf (message->payload, message->payloadlen, NULL);

      number_t type = cape_udc_get_n (payload_node, "type", 0);
      const CapeString src_cid = cape_udc_get_s (payload_node, "cid", NULL);

      if (FALSE == cape_str_equal (src_cid, self->ctx->cid))
      {
        if (payload_node && self->on_con)
        {
            CapeUdc methods_list = cape_udc_ext (payload_node, "methods");
            
            self->on_con (self->user_ptr, src_cid, cape_udc_get_s (payload_node, "name", NULL), type, &methods_list);
        }

        if (type == 1)
        {
          // tell the new connection our credentials
          qbus_pvd_ctx__internal__publish_connection (self->ctx, self, MQTT_TOPIC_PRE__MQTT_CID, src_cid);
        }
      }

      cape_udc_del (&payload_node);
      break;
    }
    case MQTT_TOPIC_PRE__MQTT_CID:
    {
      // parse into a json object
      CapeUdc payload_node = cape_json_from_buf (message->payload, message->payloadlen, NULL);

      number_t type = cape_udc_get_n (payload_node, "type", 0);
      const CapeString src_cid = cape_udc_get_s (payload_node, "cid", NULL);

      if (payload_node && self->on_con)
      {
          CapeUdc methods_list = cape_udc_ext (payload_node, "methods");

          self->on_con (self->user_ptr, src_cid, cape_udc_get_s (payload_node, "name", NULL), type, &methods_list);
      }

      cape_udc_del (&payload_node);
      break;
    }
    case MQTT_TOPIC_PRE__BY_ID:
    {
      if (self->on_snd)
      {
        QBusFrame frame = qbus_frame_new ();   // create empty frame

        number_t written = 0;

        if (qbus_frame_deserialize (frame, message->payload, message->payloadlen, &written))
        {
          self->on_snd (self->user_ptr, frame, NULL);
        }
        else
        {
          cape_log_fmt (CAPE_LL_ERROR, "mqtt", "on message", "protocol error on message = %lu", message->msgid);
        }

        qbus_frame_del (&frame);
      }

      break;
    }
    case MQTT_TOPIC_PRE__VALUE:
    {
      if (self->on_snd)
      {
        QBusFrame frame = qbus_frame_new ();   // create empty frame

        number_t written = 0;

        if (qbus_frame_deserialize (frame, message->payload, message->payloadlen, &written))
        {
          self->on_snd (self->user_ptr, frame, topicName + 2);
        }
        else
        {
          cape_log_fmt (CAPE_LL_ERROR, "mqtt", "on message", "protocol error on message = %lu", message->msgid);
        }

        qbus_frame_del (&frame);
      }

      break;
    }
  }

  MQTTClient_freeMessage (&message);
  MQTTClient_free (topicName);

  return 1;
}

//------------------------------------------------------------------------------------------------------

void qbus_pvd_ctx__internal__schedule_connection (QbusPvdCtx self, QbusPvdConnection connection)
{
  cape_mutex_lock (self->mutex);

  cape_list_push_back (self->reconnect_pool, connection);

  cape_mutex_unlock (self->mutex);
}

//------------------------------------------------------------------------------------------------------

void on_connection_lost (void* user_ptr, char* cause)
{
  QbusPvdConnection self = user_ptr;

  cape_log_fmt (CAPE_LL_ERROR, "QBUS", "mqtt", "connection lost = %s", cause);

  qbus_pvd_ctx__internal__schedule_connection (self->ctx, self);
}

//------------------------------------------------------------------------------------------------------

void qbus_pvd_con_reg (QbusPvdConnection self)
{
  // use shared subscription
  // https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901250
  //CapeString subscriber_topic = cape_str_fmt ("$share/%s/%s", module, method);

  {
    CapeString subscriber_topic = cape_str_fmt ("%c/ALL", MQTT_TOPIC_PRE__MQTT_ALL);

    cape_log_fmt (CAPE_LL_TRACE, "QBUS", "reg", "subscribe to %s", subscriber_topic);

    MQTTClient_subscribe (self->client, subscriber_topic, 1);

    cape_str_del (&subscriber_topic);
  }
  {
    CapeString subscriber_topic = cape_str_fmt ("%c/%s", MQTT_TOPIC_PRE__MQTT_CID, self->ctx->cid);

    cape_log_fmt (CAPE_LL_TRACE, "QBUS", "reg", "subscribe to %s", subscriber_topic);

    MQTTClient_subscribe (self->client, subscriber_topic, 1);

    cape_str_del (&subscriber_topic);
  }

  {
    CapeString subscriber_topic = cape_str_fmt ("%c/%s", MQTT_TOPIC_PRE__BY_ID, self->ctx->cid);

    MQTTClient_subscribe (self->client, subscriber_topic, 1);

    cape_str_del (&subscriber_topic);
  }
}

//------------------------------------------------------------------------------------------------------

int qbus_pvd_ctx__internal__connect (QbusPvdCtx self, QbusPvdConnection connection)
{
  int ret = FALSE;

  // local objects
  CapeStream payload_stream = qbus_pvd_ctx__internal__generate_payload (self, 2);
  CapeString subscriber_topic = cape_str_fmt ("%c/ALL", MQTT_TOPIC_PRE__MQTT_ALL);

  MQTTClient_willOptions last_will = MQTTClient_willOptions_initializer;
  MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

  last_will.topicName = subscriber_topic;
  last_will.qos = 1;
  last_will.message = cape_stream_get (payload_stream);

  conn_opts.username = "test";
  conn_opts.password = "1234";
  conn_opts.keepAliveInterval = 120;     // give the server more time to respond
  conn_opts.maxInflightMessages = 10;    // limit inflight messages
  conn_opts.cleansession = 1;

  conn_opts.will = &last_will;

  if (MQTTCLIENT_SUCCESS == MQTTClient_connect (connection->client, &conn_opts))
  {
    cape_log_fmt (CAPE_LL_DEBUG, "QBUS", "mqtt", "successfull connected as %s", self->cid);

      // add connection to connection pool
      cape_list_push_back (self->connection_pool, connection);
      
    qbus_pvd_con_reg (connection);

    qbus_pvd_ctx__internal__publish_connection (self, connection, MQTT_TOPIC_PRE__MQTT_ALL, "ALL");

    ret = TRUE;
  }
  else
  {
    // cape_log_fmt (CAPE_LL_ERROR, "QBUS", "mqtt", "can't connected as %s", self->cid);
  }

  cape_stream_del (&payload_stream);
  cape_str_del (&subscriber_topic);

  return ret;
}

//-----------------------------------------------------------------------------

int __STDCALL qbus_pvd_ctx__on_timer (void* user_ptr)
{
  QbusPvdCtx self = user_ptr;

  cape_mutex_lock (self->mutex);

  {
    CapeListCursor* cursor = cape_list_cursor_new (self->reconnect_pool, CAPE_DIRECTION_FORW);

    while (cape_list_cursor_next (cursor))
    {
      // try to connect
      if (qbus_pvd_ctx__internal__connect (self, cape_list_node_data (cursor->node)))
      {
        // successfull, remove from pool
        cape_list_cursor_erase (self->reconnect_pool, cursor);
      }
    }

    cape_list_cursor_del (&cursor);
  }

  cape_mutex_unlock (self->mutex);

  return TRUE;
}

//------------------------------------------------------------------------------------------------------

int qbus_pvd_ctx__internal__setup_timer (QbusPvdCtx self, CapeUdc options, CapeErr err)
{
  int res;

  // local objects
  CapeAioTimer timer = cape_aio_timer_new ();

  res = cape_aio_timer_set (timer, cape_udc_get_n (options, "reconnect_timer", 1000), self, qbus_pvd_ctx__on_timer, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = cape_aio_timer_add (&timer, self->aio);
  if (res)
  {
    cape_err_set (err, CAPE_ERR_RUNTIME, "ERR.TIMER_ADD");
    goto exit_and_cleanup;
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  return res;
}

//------------------------------------------------------------------------------------------------------

void __STDCALL qbus_pvd_ctx__connections__on_del (void* user_ptr)
{
    QbusPvdConnection self = user_ptr;

    // local objects
    CapeStream payload_stream = qbus_pvd_ctx__internal__generate_payload (self->ctx, 2);
    CapeString subscriber_topic = cape_str_fmt ("%c/ALL", MQTT_TOPIC_PRE__MQTT_ALL);

    MQTTClient_message mqtt_msg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;

    cape_log_fmt (CAPE_LL_TRACE, "PVD_MQTT", "conn del", "send disconnect message");

    mqtt_msg.payload = (void*)cape_stream_data (payload_stream);
    mqtt_msg.payloadlen = (int)cape_stream_size (payload_stream);
    mqtt_msg.qos = 1;
    mqtt_msg.retained = 0;

    // send away
    {
        int res = MQTTClient_publishMessage (self->client, subscriber_topic, &mqtt_msg, &token);
        if (res)
        {
            cape_log_msg (CAPE_LL_ERROR, "MQTT", "message", MQTTClient_strerror (res));
        }
    }

    cape_stream_del (&payload_stream);
    cape_str_del (&subscriber_topic);

    // Wait until the message is actually delivered
    MQTTClient_waitForCompletion (self->client, token, MQTT_TIMEOUT);
    
    cape_log_fmt (CAPE_LL_TRACE, "PVD_MQTT", "conn del", "disconnect from MQTT");

    // try to disconnect first
    MQTTClient_disconnect (self->client, MQTT_TIMEOUT);

    MQTTClient_destroy (&(self->client));

    cape_log_fmt (CAPE_LL_TRACE, "PVD_MQTT", "conn del", "client destroyed");

    CAPE_DEL (&self, struct QbusPvdConnection_s);
}

//------------------------------------------------------------------------------------------------------

QbusPvdCtx __STDCALL qbus_pvd_ctx_new (CapeAioContext aio, CapeUdc options, CapeErr err)
{
    QbusPvdCtx self = CAPE_NEW (struct QbusPvdCtx_s);

    // create a seed and initialize with srand
    {
        number_t seed = time(NULL) * (number_t)self;

        // initialization of the random generator
        srand ((int)seed);
    }

    self->aio = aio;
    self->cid = cape_str_uuid ();
    self->name = cape_udc_ext_s (options, "name");
    self->methods = cape_udc_ext (options, "methods");
    self->mutex = cape_mutex_new ();

    // create the lists
    self->reconnect_pool = cape_list_new (NULL);
    self->connection_pool = cape_list_new (qbus_pvd_ctx__connections__on_del);

    if (qbus_pvd_ctx__internal__setup_timer (self, options, err))
    {
        qbus_pvd_ctx_del (&self);
    }
    else
    {
        cape_log_fmt (CAPE_LL_DEBUG, "PVD_MQTT", "ctx new", "create new MQTT context as %s", self->cid);
    }

    return self;
}

//------------------------------------------------------------------------------------------------------

void __STDCALL qbus_pvd_ctx_del (QbusPvdCtx* p_self)
{
    if (*p_self)
    {
        QbusPvdCtx self = *p_self;

        cape_log_fmt (CAPE_LL_DEBUG, "PVD_MQTT", "ctx del", "destroy MQTT context as %s", self->cid);

        cape_udc_del (&(self->methods));
        cape_list_del (&(self->connection_pool));
        cape_list_del (&(self->reconnect_pool));
        cape_mutex_del (&(self->mutex));
        cape_str_del (&(self->cid));
        cape_str_del (&(self->name));

        CAPE_DEL (p_self, struct QbusPvdCtx_s);
    }
}

//------------------------------------------------------------------------------------------------------

void __STDCALL qbus_pvd_ctx_add (QbusPvdCtx self, QbusPvdConnection* p_con, CapeUdc options, void* user_ptr, fct_qbus_pvd__on_con on_con, fct_qbus_pvd__on_snd on_snd)
{
  QbusPvdConnection ret = CAPE_NEW (struct QbusPvdConnection_s);

  ret->user_ptr = user_ptr;
  ret->on_con = on_con;
  ret->on_snd = on_snd;

  ret->ctx = self;

  *p_con = ret;

  // fetch host from options
  const CapeString host = cape_udc_get_s (options, "host", "127.0.0.1");

  // do some debug output
  cape_log_fmt (CAPE_LL_TRACE, "PVD_MQTT", "ctx add", "using host = '%s' for client connection", host);

  // creates a new client instance
  MQTTClient_create (&(ret->client), host, self->cid, MQTTCLIENT_PERSISTENCE_NONE, NULL);

  // set callbacks
  MQTTClient_setCallbacks (ret->client, ret, on_connection_lost, on_message, NULL);

  // try directly to connect
  if (FALSE == qbus_pvd_ctx__internal__connect (self, ret))
  {
    // no connection possible add this to the retry pool
    qbus_pvd_ctx__internal__schedule_connection (self, ret);
  }
}

//------------------------------------------------------------------------------------------------------

void __STDCALL qbus_pvd_ctx_rm (QbusPvdCtx self, QbusPvdConnection conn)
{
    // find conn in connection pool
    CapeListCursor* cursor = cape_list_cursor_new (self->connection_pool, CAPE_DIRECTION_FORW);

    // do some debug output
    cape_log_fmt (CAPE_LL_TRACE, "PVD_MQTT", "ctx rm", "remove client connection = %p", conn);

    while (cape_list_cursor_next (cursor))
    {
        QbusPvdConnection conn_cursor = cape_list_node_data (cursor->node);

        if (conn_cursor == conn)
        {
            cape_list_cursor_erase (self->connection_pool, cursor);
        }
    }

    cape_list_cursor_del (&cursor);
}

//------------------------------------------------------------------------------------------------------

const CapeString __STDCALL qbus_pvd_con_cid (QbusPvdConnection self)
{
  return self->ctx->cid;
}

//------------------------------------------------------------------------------------------------------

void __STDCALL qbus_pvd_con_snd (QbusPvdConnection self, const CapeString cid, QBusFrame frame)
{
  // local objects
  CapeString subscriber_topic = cape_str_fmt ("%c/%s", MQTT_TOPIC_PRE__BY_ID, cid);
  CapeStream payload = cape_stream_new ();

  MQTTClient_message mqtt_msg = MQTTClient_message_initializer;
  MQTTClient_deliveryToken token;

  // convert from frame into a byte stream
  qbus_frame_serialize (frame, payload);

  mqtt_msg.payload = (void*)cape_stream_data (payload);
  mqtt_msg.payloadlen = (int)cape_stream_size (payload);

  mqtt_msg.qos = 0;
  mqtt_msg.retained = FALSE;

  // send away
  MQTTClient_publishMessage (self->client, subscriber_topic, &mqtt_msg, &token);

  cape_stream_del (&payload);
  cape_str_del(&subscriber_topic);
}

//------------------------------------------------------------------------------------------------------

void __STDCALL qbus_pvd_con_subscribe (QbusPvdConnection self, const CapeString topic)
{
  CapeString subscriber_topic = cape_str_fmt ("%c/%s", MQTT_TOPIC_PRE__VALUE, topic);

  cape_log_fmt (CAPE_LL_TRACE, "PVD_MQTT", "reg", "subscribe to %s", subscriber_topic);

  MQTTClient_subscribe (self->client, subscriber_topic, 1);

  cape_str_del (&subscriber_topic);
}

//------------------------------------------------------------------------------------------------------

void __STDCALL qbus_pvd_con_unsubscribe (QbusPvdConnection self, const CapeString topic)
{
  CapeString subscriber_topic = cape_str_fmt ("%c/%s", MQTT_TOPIC_PRE__VALUE, topic);

  cape_log_fmt (CAPE_LL_TRACE, "PVD_MQTT", "reg", "unsubscribe to %s", subscriber_topic);

  MQTTClient_unsubscribe (self->client, subscriber_topic);

  cape_str_del (&subscriber_topic);
}

//------------------------------------------------------------------------------------------------------

void __STDCALL qbus_pvd_con_next (QbusPvdConnection self, const CapeString topic, QBusFrame frame)
{
  // local objects
  CapeString subscriber_topic = cape_str_fmt ("%c/%s", MQTT_TOPIC_PRE__VALUE, topic);
  CapeStream payload = cape_stream_new ();

  MQTTClient_message mqtt_msg = MQTTClient_message_initializer;
  MQTTClient_deliveryToken token;

  //cape_log_fmt (CAPE_LL_TRACE, "QBUS", "reg", "commit value to %s", subscriber_topic);

  // convert from frame into a byte stream
  qbus_frame_serialize (frame, payload);

  mqtt_msg.payload = (void*)cape_stream_data (payload);
  mqtt_msg.payloadlen = (int)cape_stream_size (payload);
  mqtt_msg.qos = 0;

  // if we want to store the last message, even if there are no subscribers
  // turn on retained
  mqtt_msg.retained = TRUE;

  // send away
  MQTTClient_publishMessage (self->client, subscriber_topic, &mqtt_msg, &token);

  cape_stream_del (&payload);
  cape_str_del(&subscriber_topic);
}

//------------------------------------------------------------------------------------------------------
