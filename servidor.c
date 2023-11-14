#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CLIENTS 10

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int socket;
    char name[50];
} Client;

Client clients[MAX_CLIENTS];

int client_count = 0;

void send_message_to_all(char *message, int sender_socket, char *sender_name) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < client_count; ++i) {
        if (clients[i].socket != -1 && clients[i].socket != sender_socket) {
            write(clients[i].socket, message, strlen(message));
        }
    }
    pthread_mutex_unlock(&mutex);
}

void send_clients_list(int client_socket) {
    pthread_mutex_lock(&mutex);
    char list_message[256] = "Usuarios conectados:\n";
    for (int i = 0; i < client_count; ++i) {
        if (clients[i].socket != -1) {
            strcat(list_message, clients[i].name);
            strcat(list_message, "\n");
        }
    }
    pthread_mutex_unlock(&mutex);

    write(client_socket, list_message, strlen(list_message));
}

void remove_client(int current_client) {
    pthread_mutex_lock(&mutex);
    close(clients[current_client].socket);
    clients[current_client].socket = -1;
    pthread_mutex_unlock(&mutex);
}

void *handle_client(void *arg) {
    int current_client = client_count++;
    int client_socket = *((int *)arg);
    char buffer[1024];
    char name[50];

    // Obtener el nombre del cliente
    read(client_socket, name, sizeof(name));

    // Agregar el cliente a la lista
    pthread_mutex_lock(&mutex);
    clients[current_client].socket = client_socket;
    strcpy(clients[current_client].name, name);
    pthread_mutex_unlock(&mutex);

    // Notificar a los demás que un nuevo usuario se ha unido
    char join_message[100];
    sprintf(join_message, "%s se ha unido al chat.\n", name);
    send_message_to_all(join_message, client_socket, "Servidor");

    // Enviar la lista de usuarios conectados al nuevo cliente
    send_clients_list(client_socket);

    while (1) {
        int received = read(client_socket, buffer, sizeof(buffer));
        if (received <= 0) {
            // El cliente se desconectó
            // Notificar a los demás que un usuario se ha desconectado
            char leave_message[100];
            sprintf(leave_message, "%s se ha desconectado.\n", name);
            send_message_to_all(leave_message, client_socket, "Servidor");

            // Eliminar al cliente de la lista y cerrar el hilo
            remove_client(current_client);
            pthread_exit(NULL);
        } else if (strncmp(buffer, "/list", 5) == 0) {
            // Enviar la lista de usuarios conectados
            send_clients_list(client_socket);
        } else {
            // Enviar el mensaje a todos los clientes con indicación de nombre
            char message[1076];  // El tamaño se ajusta para manejar el nombre, el símbolo ">" y el mensaje
            snprintf(message, sizeof(message), "%s > %s", name, buffer);
            send_message_to_all(message, client_socket, name);
            memset(buffer, 0, sizeof(buffer));  // Limpiar el buffer
        }
    }
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    pthread_t tid;

    // Crear el socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección del servidor
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    // Enlazar el socket al puerto
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Error al enlazar el socket");
        exit(EXIT_FAILURE);
    }

    // Escuchar por conexiones
    if (listen(server_socket, 10) == -1) {
        perror("Error al escuchar por conexiones");
        exit(EXIT_FAILURE);
    }

    printf("Servidor en espera de conexiones en el puerto %d...\n", PORT);

    while (1) {
        socklen_t client_address_len = sizeof(client_address);

        // Aceptar la conexión del cliente
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
        if (client_socket == -1) {
            perror("Error al aceptar la conexión");
            exit(EXIT_FAILURE);
        }

        // Crear un hilo para manejar al cliente
        if (pthread_create(&tid, NULL, handle_client, &client_socket) != 0) {
            perror("Error al crear el hilo");
            exit(EXIT_FAILURE);
        }

        // Liberar recursos del hilo principal para que puedan ser recolectados cuando los hilos hijos terminen
        pthread_detach(tid);
    }

    // Cerrar el socket del servidor
    close(server_socket);

    return 0;
}
