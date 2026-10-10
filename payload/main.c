#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define PORT 9022
#define ROOT "/data/ps5-transfer"
#define BUFFER_SIZE 16384

static int valid_name(const char *name) {
    return name[0] != '\0' && strchr(name, '/') == NULL && strchr(name, '\\') == NULL && strcmp(name, ".") != 0 && strcmp(name, "..") != 0;
}

static int send_text(int client, const char *text) {
    return send(client, text, strlen(text), 0) < 0 ? -1 : 0;
}

static int receive_line(int client, char *line, size_t capacity) {
    size_t length = 0;
    while (length + 1 < capacity) {
        char character;
        const ssize_t received = recv(client, &character, 1, 0);
        if (received <= 0) return -1;
        if (character == '\n') break;
        line[length++] = character;
    }
    line[length] = '\0';
    return 0;
}

static void list_files(int client) {
    DIR *directory = opendir(ROOT);
    if (!directory) {
        mkdir(ROOT, 0755);
        send_text(client, "OK 0\n");
        return;
    }
    char line[512];
    send_text(client, "OK\n");
    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL) {
        if (!valid_name(entry->d_name)) continue;
        struct stat info;
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", ROOT, entry->d_name);
        if (stat(path, &info) == 0 && S_ISREG(info.st_mode)) {
            snprintf(line, sizeof(line), "%s %lld\n", entry->d_name, (long long)info.st_size);
            send_text(client, line);
        }
    }
    send_text(client, ".\n");
    closedir(directory);
}

static void get_file(int client, const char *name) {
    if (!valid_name(name)) { send_text(client, "ERR invalid-name\n"); return; }
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", ROOT, name);
    FILE *file = fopen(path, "rb");
    if (!file) { send_text(client, "ERR not-found\n"); return; }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char header[64];
    snprintf(header, sizeof(header), "OK %ld\n", size);
    send_text(client, header);
    char buffer[BUFFER_SIZE];
    size_t count;
    while ((count = fread(buffer, 1, sizeof(buffer), file)) > 0) send(client, buffer, count, 0);
    fclose(file);
}

static void put_file(int client, const char *name, long size) {
    if (!valid_name(name) || size < 0) { send_text(client, "ERR invalid-request\n"); return; }
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", ROOT, name);
    FILE *file = fopen(path, "wb");
    if (!file) { send_text(client, "ERR cannot-open-output\n"); return; }
    char buffer[BUFFER_SIZE];
    long remaining = size;
    while (remaining > 0) {
        const size_t requested = remaining < (long)sizeof(buffer) ? (size_t)remaining : sizeof(buffer);
        const ssize_t received = recv(client, buffer, requested, 0);
        if (received <= 0) { fclose(file); remove(path); return; }
        if (fwrite(buffer, 1, (size_t)received, file) != (size_t)received) { fclose(file); remove(path); return; }
        remaining -= received;
    }
    fclose(file);
    send_text(client, "OK\n");
}

int main(void) {
    mkdir(ROOT, 0755);
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) { perror("socket"); return 1; }
    int reuse = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server, (struct sockaddr *)&address, sizeof(address)) < 0 || listen(server, 4) < 0) {
        perror("bind/listen"); close(server); return 1;
    }
    printf("PS5 LAN Transfer listening on port %d\n", PORT);
    for (;;) {
        int client = accept(server, NULL, NULL);
        if (client < 0) continue;
        char command[512] = {0};
        if (receive_line(client, command, sizeof(command)) == 0) {
            if (strcmp(command, "LIST") == 0) list_files(client);
            else if (strncmp(command, "GET ", 4) == 0) get_file(client, command + 4);
            else if (strncmp(command, "PUT ", 4) == 0) {
                char name[256];
                long size;
                if (sscanf(command + 4, "%255s %ld", name, &size) == 2) put_file(client, name, size);
                else send_text(client, "ERR invalid-request\n");
            } else send_text(client, "ERR unknown-command\n");
        }
        close(client);
    }
}
