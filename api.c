
#include <stdio.h>
#include <ulfius.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#define PORT 8080
#define SOCKET_PATH "/tmp/mpi_socket"

void send_to_mpi(const char *data) {
    int sock;
    struct sockaddr_un addr;

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return;
    }

    write(sock, data, strlen(data));
    close(sock);
}


int callback_hello_world (const struct _u_request * request, struct _u_response * response, void * user_data) {
  ulfius_set_string_body_response(response, 200, "Hello World!");
  return U_CALLBACK_CONTINUE;
}

int callback_post (const struct _u_request * request, struct _u_response * response, void * user_data) {
json_t * json_body = ulfius_get_json_body_request(request, NULL);
    char *json_string = (char*)malloc(10000);

if (json_body) {
        json_string = json_dumps(json_body, JSON_INDENT(2));
        if (json_string) {
            printf("Received JSON body:\n%s\n", json_string);
            send_to_mpi(json_string);
            ulfius_set_string_body_response(response, 200, json_string);
            free(json_string);
        }
        json_decref(json_body);
    } else {
        printf("No JSON body or invalid JSON\n");
    }

  return U_CALLBACK_CONTINUE;
}


int main(void) {
  struct _u_instance instance;

  if (ulfius_init_instance(&instance, PORT, NULL, NULL) != U_OK) {
    fprintf(stderr, "Error ulfius_init_instance, abort\n");
    return(1);
  }

  ulfius_add_endpoint_by_val(&instance, "GET", "/helloworld", NULL, 0, &callback_hello_world, NULL);
  ulfius_add_endpoint_by_val(&instance, "POST", "/api", NULL, 0, &callback_post, NULL);

  if (ulfius_start_framework(&instance) == U_OK) {
    printf("Start framework on port %d\n", instance.port);

    getchar();
  } else {
    fprintf(stderr, "Error starting framework\n");
  }
  printf("End framework\n");

  ulfius_stop_framework(&instance);
  ulfius_clean_instance(&instance);

  return 0;
}