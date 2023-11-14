#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080

void *receive_messages(void *arg) {
    int client_socket = *((int *)arg);
    char buffer[1024];

    while (1) {
        int received = read(client_socket, buffer, sizeof(buffer));
        if (received <= 0) {
            perror("Error al recibir mensajes");
            exit(EXIT_FAILURE);
        }

        printf("%s", buffer);
        memset(buffer, 0, sizeof(buffer));  // Limpiar el buffer
    }
}

int main() {
    int client_socket;
    struct sockaddr_in server_address;
    pthread_t tid;

    // Crear el socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección del servidor
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Conectar al servidor
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Error al conectar al servidor");
        exit(EXIT_FAILURE);
    }

    // Ingresar el nombre del usuario
    char name[50];
    printf("Ingresa tu nombre: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';

    // Enviar el nombre al servidor
    write(client_socket, name, sizeof(name));

    // Crear un hilo para recibir mensajes del servidor
    if (pthread_create(&tid, NULL, receive_messages, &client_socket) != 0) {
        perror("Error al crear el hilo");
        exit(EXIT_FAILURE);
    }

    // Procesar la entrada del usuario y enviar mensajes al servidor
    char message[1024];
    while (1) {
        //printf("Ingrese mensaje: ");
        fgets(message, sizeof(message), stdin);
        write(client_socket, message, strlen(message));

        // Limpiar el buffer de entrada
        memset(message, 0, sizeof(message));
    }

    // Esperar a que el hilo de recepción termine (no se llegará aquí en este ejemplo)
    pthread_join(tid, NULL);

    // Cerrar el socket del cliente
    close(client_socket);

    return 0;
}
