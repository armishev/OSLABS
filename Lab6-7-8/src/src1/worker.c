#include "log/log.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <czmq.h>


static char *WORKER_EXEC_PATH = "worker";


/* Create worker node functional */


// Создание дочернего процесса - вычислительного узла
// @argument worker_id - id создаваемого вычислительного узла
// @argument manager_endpoint - endpoint сокета, которому созданный узел отправит сгенерированный endpoint
// @note аналог функции __create_worker_process в менеджере
pid_t create_worker(int worker_id, char *manager_endpoint) {
    pid_t id = fork();
    if (id != 0) return id;

    char worker_id_str[256];
    sprintf(worker_id_str, "%d", worker_id);

    char *args[] = {WORKER_EXEC_PATH, manager_endpoint, worker_id_str, NULL};
    execv(WORKER_EXEC_PATH, args);

    exit(1);
}


/* Timer functional */


// Структура таймера
typedef struct {
    unsigned long elapsed_time; // сколько времени насчитал таймер
    unsigned long last_timemark; // последняя временная точка старта таймера
    bool is_running; // запущен ли сейчас таймер?
} Timer;


// Получение текущего времени в миллисекундах
unsigned long __get_current_time() {
    struct timeval timemark;
    gettimeofday(&timemark, NULL);
    return timemark.tv_sec * 1000LL + timemark.tv_usec / 1000;
}


// Конструктор таймера
// @argument timer - ссылка на структуру таймера
// @note при повторном вызове конструктора обновляет таймер
void timer_init(Timer *timer) {
    timer->elapsed_time = 0;
    timer->last_timemark = __get_current_time();
    timer->is_running = false;
}


// Запускает таймер
// @argument timer - ссылка на таймер
void timer_start(Timer *timer) {
    if (timer->is_running)
        return;

    timer->last_timemark = __get_current_time();
    timer->is_running = true;
}


// Приостанавливает таймер
// @argument timer - ссылка на таймер
void timer_stop(Timer *timer) {
    if (!timer->is_running)
        return;

    unsigned long current_time = __get_current_time();
    timer->elapsed_time += current_time - timer->last_timemark;
    timer->is_running = false;
}


// Получение количества миллисекунд, которые насчитал таймер
// @argument timer - ссылка на таймер
unsigned long timer_time(Timer *timer) {
    if (timer->is_running) {
        unsigned long current_time = __get_current_time();
        return timer->elapsed_time + (current_time - timer->last_timemark);
    }

    return timer->elapsed_time;
}


/* Heartbeat functional */


// Тип данных функции, которая будет вызываться при отправке
// сообщения "я жив"
typedef void (*CallbackFunction)(void);   //???


// Структура HEARTBEAT
typedef struct {
    long polling_interval; // интервал, с которым вызывается callback функция
    pthread_t polling_thread; // поток отправки сообщений "я жив"
    CallbackFunction callback; // ссылка на callback функцию
} Heartbeat;


// Поток исполнения "засыпает" на msec миллисекунд
// @note Вспомогательная функция. Аналог sleep только для миллисекунд
int msleep(long msec) {
    struct timespec ts;
    int res;

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do res = nanosleep(&ts, &ts);
    while (res && errno == EINTR);

    return res;
}


// Функция потока отправки сообщений "я жив"
void *__polling_thread_func(void *_hb) {
    Heartbeat *hb = (Heartbeat *) _hb;

    while (1) {
        // при -1 останавливаем поток
        if (hb->polling_interval == -1)
            break;

        msleep(hb->polling_interval);
        hb->callback();
    }

    return NULL;
}


// Конструктор HEARTBEAT
// @argument hb - ссылка на структуру HEARTBEAT
// @argument callback - ссылка на callback функцию
void heartbeat_init(Heartbeat *hb, CallbackFunction callback) {
    hb->polling_interval = -1; // маркер -1 означает, что поток отправки сообщений еще не создан
    hb->callback = callback;
}


// Начало отправки сообщений "я жив"
// @argument hb - ссылка на структуру HEARTBEAT
// @argument interval - интервал (мс), с которым отправляем сообщения
void heartbeat_start_polling(Heartbeat *hb, long interval) {
    // если поток был создан - останавливаем его
    if (hb->polling_interval != -1) {
        hb->polling_interval = -1;
        pthread_join(hb->polling_thread, NULL);
    }

    hb->polling_interval = interval;
    pthread_create(&hb->polling_thread, NULL, __polling_thread_func, (void *) hb);
}


// Деструктор HEARTBEAT
// @argument hb - ссылка на структуру HEARTBEAT
void heartbeat_remove(Heartbeat *hb) {
    hb->polling_interval = -1;
    pthread_join(hb->polling_thread, NULL);
}


/* Main functional */


static zsock_t *input_socket; // сокет для прослушивания входных сообщений и отправки ответов
static zsock_t *heartbeat_socket = NULL; // сокет, на который узел отправляет сообщения "я жив"
int id; // id вычислительного узла
static Heartbeat hb; // объект HEARTBEAT
static Timer timer; // таймер вычислительного узла


// Отправляет сообщение "я жив" через ZMQ сокет менеджеру
void send_alive_message() {
    zstr_sendf(heartbeat_socket, "%d", id);
    free(zstr_recv(heartbeat_socket));
}


// Отправляет результат инициализации вычислительного узла менеджеру
// @argument manager_endpoint - endpoint менеджера
// @argument worker_endpoint - endpoint сокета, по которому узел будет получать сообщения
void send_response_to_manager(char *manager_endpoint, const char *worker_endpoint) {
    zsock_t *manager_socket = zsock_new_req(manager_endpoint);

    zstr_send(manager_socket, worker_endpoint);
    free(zstr_recv(manager_socket));

    zsock_destroy(&manager_socket);
}


int main(int argc, char *argv[]) {
    id = atoi(argv[2]);

    input_socket = zsock_new_rep("tcp://0.0.0.0:*");
    send_response_to_manager(argv[1], zsock_endpoint(input_socket));

    heartbeat_init(&hb, send_alive_message);
    timer_init(&timer);

    while (1) {
        char *request_type = zstr_recv(input_socket);

        if (!strcmp(request_type, "create")) {
            char *manager_endpoint = zstr_recv(input_socket);
            char *worker_id_arg = zstr_recv(input_socket);
            pid_t pid = create_worker(atoi(worker_id_arg), manager_endpoint);

            zstr_sendf(input_socket, "%d", pid);

            zstr_free(&manager_endpoint);
            zstr_free(&worker_id_arg);
        } else if (!strcmp(request_type, "heartbeat")) {
            char *interval_arg, *heartbeat_endpoint;
            long interval;

            interval_arg = zstr_recv(input_socket);
            heartbeat_endpoint = zstr_recv(input_socket);
            zstr_send(input_socket, "OK");

            sscanf(interval_arg, "%ld", &interval);

            // Подключаемся к HEARTBEAT сокету менеджера
            // Если ранее были подключены к другому сокету - разрываем соединение
            if (heartbeat_socket != NULL)
                zsock_destroy(&heartbeat_socket);
            heartbeat_socket = zsock_new_req(heartbeat_endpoint);

            heartbeat_start_polling(&hb, interval);

            zstr_free(&interval_arg);
            zstr_free(&heartbeat_endpoint);
        } else if (!strcmp(request_type, "exit")) {
            zstr_send(input_socket, "OK");
            break;
        } else if (!strcmp(request_type, "timer-start")) {
            timer_start(&timer);
            zstr_send(input_socket, "OK");
        } else if (!strcmp(request_type, "timer-stop")) {
            timer_stop(&timer);
            zstr_send(input_socket, "OK");
        } else if (!strcmp(request_type, "timer-time")) {
            unsigned long time = timer_time(&timer);
            zstr_sendf(input_socket, "%lu", time);
        }

        zstr_free(&request_type);
    }

    heartbeat_remove(&hb);
    zsock_destroy(&input_socket);
    zsock_destroy(&heartbeat_socket);

    return 0;
}
