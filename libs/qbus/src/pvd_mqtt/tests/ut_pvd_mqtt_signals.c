#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>

#include "MQTTClient.h"

#define ADDRESS  "127.0.0.1"
#define CLIENTID "SafeClient"
#define TOPIC    "test/topic"
#define QOS      1

static volatile sig_atomic_t stop = 0;

//------------------------------------------------------------------------------------------------------

static void handle_sigint(int sig)
{
    (void)sig;   // avoid unused warning
    stop = 1;
}

//------------------------------------------------------------------------------------------------------

static int messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    (void)context;
    (void)topicLen;

    printf("Message arrived on topic %s: %.*s\n",
           topicName,
           message->payloadlen,
           (const char *)message->payload);

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

//------------------------------------------------------------------------------------------------------

static void connectionLost(void *context, char *cause)
{
    (void)context;
    printf("Connection lost: %s\n", cause ? cause : "unknown");
}

//------------------------------------------------------------------------------------------------------

int main(void)
{

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGTERM, &sa, NULL) == -1)
    {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    // local objects
    MQTTClient client = NULL;

    if (MQTTClient_create (&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL) != MQTTCLIENT_SUCCESS)
    {
        fprintf(stderr, "MQTTClient_create failed\n");
        return EXIT_FAILURE;
    }

    MQTTClient_setCallbacks (client, NULL, connectionLost, messageArrived, NULL);

    // connect

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

    conn_opts.username = "test";
    conn_opts.password = "1234";
    conn_opts.keepAliveInterval = 120;     // give the server more time to respond

    if (MQTTClient_connect(client, &conn_opts) != MQTTCLIENT_SUCCESS)
    {
        fprintf(stderr, "MQTT connect failed\n");
        MQTTClient_destroy(&client);

        return EXIT_FAILURE;
    }

    MQTTClient_subscribe(client, TOPIC, QOS);

    printf("Running. Press Ctrl+C to exit.\n");

            /* --- epoll setup --- */
            int epfd = epoll_create1(0);
            if (epfd == -1) {
                perror("epoll_create1");
                MQTTClient_disconnect(client, 1000);
                MQTTClient_destroy(&client);
                return EXIT_FAILURE;
            }

            struct epoll_event events[8];

            /* --- main loop --- */
            while (!stop) {
                int i;

                int n = epoll_wait(epfd, events, 8, -1);  /* blocking */

                if (n == -1) {
                    if (errno == EINTR) {
                        /* interrupted by signal → check stop flag */
                        continue;
                    }
                    perror("epoll_wait");
                    break;
                }

                for (i = 0; i < n; ++i) {
                    /* handle your fds here */
                }
            }

            /* --- shutdown --- */
            printf("Shutting down...\n");

            MQTTClient_disconnect(client, 1000);
            MQTTClient_destroy(&client);

            close(epfd);

            return EXIT_SUCCESS;
}
