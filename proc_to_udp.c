#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <getopt.h>

#define MAX_LINE 1024

volatile int running = 1;  // Флаг для выхода из цикла
char *total_line; // Буфер для строк
int total_size_all_line; // Длина считываемой информации из файла
int verbose = 0;  // Вывод дополнительных сообщений

struct cpu_stats { // Структура для хранения одного набора счётчиков
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
};

// Вычисление загрузки в процентах между двумя наборами счётчиков
float calculate_usage(const struct cpu_stats *prev, const struct cpu_stats *curr) {
    unsigned long long prev_total = prev->user + prev->nice + prev->system +
                                    prev->idle + prev->iowait + prev->irq +
                                    prev->softirq + prev->steal + prev->guest + prev->guest_nice;
    unsigned long long curr_total = curr->user + curr->nice + curr->system +
                                    curr->idle + curr->iowait + curr->irq +
                                    curr->softirq + curr->steal + curr->guest + curr->guest_nice;

    unsigned long long prev_idle = prev->idle + prev->iowait + prev->guest + prev->guest_nice;
    unsigned long long curr_idle = curr->idle + curr->iowait + curr->guest + curr->guest_nice;

    unsigned long long total_delta = curr_total - prev_total;
    unsigned long long idle_delta = curr_idle - prev_idle;

    // printf("total_delta %ld\n", total_delta);

    if (total_delta <= 0) return 0.0;
    return (100.0 * (total_delta - idle_delta)) / total_delta;
}

// Получить число ядер (один раз при старте)
int get_cpu_count() {
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    if (n < 1) n = 1;
    return (int)n;
}

void handle_signal(int signal) {
    running = 0;  // Устанавливаем флаг выхода из цикла
}

// Парсинг одной строки (пропускает "cpu" или "cpuN")
int parse_cpu_line(char *line, struct cpu_stats *stats) {
    // const char *p = line;
    while (*line && *line != ' ') line++; // переходим к первому пробелу после имени
    if (sscanf(line, "%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
               &stats->user, &stats->nice, &stats->system,
               &stats->idle, &stats->iowait, &stats->irq,
               &stats->softirq, &stats->steal, &stats->guest, &stats->guest_nice) != 10)
        return -1;
    return 0;
}

// Чтение суммарной статистики и массивов для каждого ядра
// total – указатель на структуру для суммарной статистики
// per_cpu – массив из cpu_count элементов для хранения статистики по ядрам
// Возвращает 0 при успехе, -1 при ошибке
int read_all_cpu_stats(struct cpu_stats *total,
                       struct cpu_stats *per_cpu,
                       int cpu_count) {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return -1;

    char *line;
    int cpu_idx = 0;
    int total_read = 0;

    size_t bytes_read = fread(total_line, 1, total_size_all_line - 1, f);
    if (bytes_read == 0) {
        if (feof(f))
            fprintf(stderr, "Warning: /proc/stat is empty\n");
        if (ferror(f))
            fprintf(stderr, "Error reading /proc/stat");
        fclose(f);
        return -1;
    }
    total_line[bytes_read] = '\0';

    int line_ind = 0;
    // int current_cpu = 0;
    line = total_line;
    while (line_ind < bytes_read && cpu_idx < cpu_count) {
        if (strncmp(line, "cpu", 3) != 0) break; // конец секции CPU

        if (line[3] == ' ' || line[3] == '\t') {
            // Суммарная строка
            if (parse_cpu_line(line, total) == 0)
                total_read = 1;
        }
        else if (line[3] >= '0' && line[3] <= '9') {
            // Строка очередного ядра
            if (cpu_idx < cpu_count) {
                if (parse_cpu_line(line, &per_cpu[cpu_idx]) == 0)
                    cpu_idx++;
            }
        }
        while (*line && *line != '\n') line++; // переходим к символу переноса строки
        line++; // переходим к символу после переноса строки
        if (*line == '\0') break;
    }
    fclose(f);

    if (!total_read || cpu_idx != cpu_count) return -1;
    return 0;
}

int main(int argc, char *argv[])
{
    int opt;
    // Опция: короткая 'v' - вывод дополнительных сообщений
    while ((opt = getopt(argc, argv, "v")) != -1)
        if (opt == 'v') verbose = 1;

    signal(SIGINT, handle_signal);  // Регистрируем обработчик сигнала установки выхода из цикла
    printf("Begin!!!!!\n");
    uint16_t cpu_count = get_cpu_count();
    if (cpu_count < 1) {
        perror("Error cpu_count < 1");
        return 1;
    }
    printf("Обнаружено ядер: %d\n", cpu_count);

    double *cpu_usage = malloc(cpu_count * sizeof(double));
    // Массивы для предыдущего и текущего состояний
    struct cpu_stats prev_total, curr_total;
    struct cpu_stats *prev_cpu = malloc(cpu_count * sizeof(struct cpu_stats));
    struct cpu_stats *curr_cpu = malloc(cpu_count * sizeof(struct cpu_stats));
    if (!prev_cpu || !curr_cpu) {
        fprintf(stderr, "Memory allocation failed\n");
        free(prev_cpu);
        free(curr_cpu);
        return 2;
    }

    total_size_all_line = (cpu_count + 1) * MAX_LINE; // Длина считываемой информации из файла
    total_line = malloc(total_size_all_line); // Буфер для строк
    if (!total_line) {
        perror("Error malloc total_line");
        return 3;
    }

    // Первое чтение (просто заполняем предыдущие значения)
    if (read_all_cpu_stats(&prev_total, prev_cpu, cpu_count) != 0) {
        fprintf(stderr, "Failed to read /proc/stat\n");
        free(prev_cpu);
        free(curr_cpu);
        free(total_line);
        return 4;
    }

    // Работа с UDP
    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    // Создание UDP сокета
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        free(prev_cpu);
        free(curr_cpu);
        free(total_line);
        return 5;
    }
    // Структура адреса получателя
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1234);          // порт получателя
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // localhost
    size_t message_len = 8 + 4 + 2 + (cpu_count + 1) * 5;
    // 8(BEGIN END) 4(uint32_t message_counter) 2(uint16_t cpu_count) 5 (bool float)
    char *message;
    message = (char*)malloc(message_len);
    if (message == NULL) {
        perror("malloc failed");
        close(sockfd);
        free(prev_cpu);
        free(curr_cpu);
        free(total_line);
        return 6;
    }
    uint32_t message_counter = 0;
    strncpy(message, "BEGIN", 5);
    strncpy(message + (message_len - 3), "END", 3);

    struct timeval myTime; // Текущее время
    long myPause;  // Пауза для выравнивания времени выполнения действия
    // double delta_usage_summ = 0; // Для коррекции показателей загрузки ядер
    while (running) {
        gettimeofday(&myTime, NULL);
        myPause = 1000000 - myTime.tv_usec; // Расчёт длительности паузы до начала сканирования
        if (verbose) printf("Pause %ld mks\n", myPause);
        usleep(myPause);

        if (read_all_cpu_stats(&curr_total, curr_cpu, cpu_count) != 0) {
            perror("read_all_cpu_stats");
            continue;
        }

        // Суммарная загрузка
        float total_usage = calculate_usage(&prev_total, &curr_total);

        // Загрузка по каждому ядру
        float summ_usage = 0;
        for (int i = 0; i < cpu_count; i++) {
            cpu_usage[i] = calculate_usage(&prev_cpu[i], &curr_cpu[i]);
            if (verbose) printf("  CPU%d: %.3f%%\n", i, cpu_usage[i] );
            summ_usage += cpu_usage[i];
        }
        summ_usage  = summ_usage / 4.;
        // float k = total_usage / summ_usage; // Для коррекции показателей загрузки ядер
        if (verbose) printf("Total CPU usage: summ_usage: %.3f%%   summ_usage: %.3f%%\n", total_usage, summ_usage);

        // // Коррекция показателей загрузки ядер
        // float delta_usage = summ_usage - total_usage;
        // delta_usage_summ += delta_usage;
        // printf("Delta usage: %.3f %.3f%% %.3f%%\n", k, delta_usage, delta_usage_summ);
        // summ_usage = 0;
        // for (int i = 0; i < cpu_count; i++) {
        //     cpu_usage[i] = cpu_usage[i] * k;
        //     if (cpu_usage[i] > 100.) pu_usage[i] = 100.
        //     printf("  CPU%d: %.3f%%\n", i, cpu_usage[i] );
        //     summ_usage += cpu_usage[i];
        // }
        // summ_usage  = summ_usage / 4.;
        // delta_usage = summ_usage - total_usage;
        // printf("Delta korr usage: %.3f%%\n", delta_usage);

        // Заполнение сообщения
        *(uint32_t*)(&message[5]) = message_counter; // 4 байта Счётчик
        *(uint16_t*)(&message[9]) = cpu_count; // 2 байта Число ядер
        *(uint8_t*)(&message[11]) = 1; // 1 байт Наличие информации об общей загрузки
        *(float*)(&message[12]) = total_usage; // 4 байта Общая загрузка
        for (int i = 0; i < cpu_count; i++) {
            *(uint8_t*)(&message[16 + i * 5]) = 1; // 1 байт Наличие информации загрузке ядра
            *(float*)(&message[17 + i * 5]) = cpu_usage[i]; // 4 байта Загрузка ядра
        }
        // Отправка сообщения
        ssize_t sent = sendto(sockfd, message, message_len, 0,
                              (struct sockaddr*)&server_addr, addr_len);
        if (sent < 0) {
            perror("sendto failed");
        } else {
            if (verbose) printf("Sent %ld bytes to %s:%d\n", sent,
                   inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
        }

        // Подготовка к следующей итерации
        prev_total = curr_total;
        for (int i = 0; i < cpu_count; i++)
            prev_cpu[i] = curr_cpu[i];

        fflush(stdout);
        message_counter++;
    }

    printf("End!!!!!!\n");
    close(sockfd);
    free(message);
    free(total_line);
    free(prev_cpu);
    free(curr_cpu);
    return 0;
}
