#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <czmq.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <zmq.h>
#include <sys/wait.h>
#include <pthread.h>

static const int MAX_LIFE_POINTS = 4; // максимальное количество очков жизни
static char* WORKER_EXEC_PATH = "worker"; // путь к исполняемому файлу вычислительного узла
static const long START_HEARTBEAT_INTERVAL = 1000;
static const long MAX_TIMEOUT = 1000; // таймаут (в мс) для принятия / отправки сообщений

static const int OK = 0; // операция выполнено успешно
static const int ALREADY_EXISTS = 1; // узел уже существует
static const int NOT_EXISTS = 2; // узла не существуте
static const int UNACTIVE = 3; // узел неактивен


/* Topology functional */

// структура элемента вычислительного узла
typedef struct __WorkerNode {
	struct __WorkerNode* next; // указатель на следующий элемент (NULL если элемент последний)
	int id; // идентификатор вычислительного узла
	zsock_t* socket; // сокет для отправки запросов на узел
	int life_points; // очки жизни узла
} WorkerNode;


// структура корневого элемента
// @note такие элементы нужны для связки нескольких список из рабочих узлов
typedef struct __RootNode {
	struct __RootNode* next; // ссылка на следующий корневой элемент (NULL если элемент последний)
	WorkerNode* root_worker; // ссылка на первый элемент в списке вычислительных узлов
} RootNode;


// структура топологии
typedef struct {
	RootNode* root; // первый элемент списка корневых элементов
} Topology;


// структура итератора по топологии
// @note нужен, чтобы упростить обход всех вычислительных узлов в топологии
typedef struct {
	RootNode* root_node; // текущий корневой элемент
	WorkerNode* worker_node; // текущий вычислительный узел
} TopologyIterator;


// Конструктор элемента вычислительного узла
// @argument node - ссылка на структуру элемента
// @argument id - id вычислительного узла
// @argument socket - сокет для общения с вычислительным узлом
void worker_node_init(WorkerNode* node, int id, zsock_t* socket)
{
	node->id = id;
	node->socket = socket;
	node->next = NULL;
	node->life_points = MAX_LIFE_POINTS;
}


// Деструктор элемента вычислительного узла
void worker_node_remove(WorkerNode* node)
{
	zstr_send(node->socket, "exit");
	zsock_destroy(&node->socket);
	node->next = NULL;
}


// Конструктор итератора по топологии
void topology_iter_init(TopologyIterator* iter, Topology* tp)
{
	iter->root_node = tp->root;
	iter->worker_node = (tp->root != NULL) ? tp->root->root_worker : NULL;
}


// Получение текущего вычислительного узла, на который указывает итератор
// @argument iter - ссылка на итератор
// @note возвращает NULL, если все элементы были пройдены
WorkerNode* topology_iter_curr(TopologyIterator* iter)
{
	return iter->worker_node;
}


// Переход к следующему вычислительному узлу
// @argument iter - ссылка на итератор
WorkerNode* topology_iter_next(TopologyIterator* iter)
{
	iter->worker_node = iter->worker_node->next;

	// если мы дошли до конца в списке и есть следующий список...
	if(iter->worker_node == NULL && iter->root_node->next != NULL)
	{
		iter->root_node = iter->root_node->next;
		iter->worker_node = iter->root_node->root_worker;
	}

	return topology_iter_curr(iter);
}


// Инициализация структуры топологии
// @argument tp - ссылка на структуру топологии
void topology_init(Topology* tp)
{
	tp->root = NULL;
}


// Удаление вычислительного узла из списка вместе со всеми дочерними элементами
// @note под дочерними понимаются все те элементы, которые идут в списке после текущего
// @argument root - ссылка на текущий элемент списка
// @argument id - id вычислительного удаляемого узла (при -1 удаляются все элементы)
WorkerNode* __remove_worker(WorkerNode* root, int id)
{
	if(root == NULL)
		return NULL;

	// если текущий элемент - тот, который надо удалить...
	if(root->id == id || id == -1)
	{
		free(zstr_recv(root->socket));
		__remove_worker(root->next, -1);
		worker_node_remove(root);
		free(root);
		return NULL;
	}

	root->next = __remove_worker(root->next, id);
	return root;
}


// Удаление вычислительного узла из списка, находящихся среди тех, на которые
// указывают корневые элементы
// @argument root - ссылка на текущий корневой элемент
// @argument id - id вычислительного удаляемого узла (при -1 удаляются все элементы)
RootNode* __remove_worker_from_root(RootNode* root, int id)
{
	if(root == NULL)
		return NULL;

	root->root_worker = __remove_worker(root->root_worker, id);

	// если корневой элемент ссылается на пустой список, то он больше не нужен
	if(root->root_worker == NULL)
	{
		RootNode* ret = root->next;
		free(root);
		return ret;
	}

	root->next = __remove_worker_from_root(root->next, id);
	return root;
}


// Добавление корневого элемента, у которого в списке 1 элемент - node
// @argument tp - ссылка на топологии, куда добавляем
// @argument node - узел, который будет в списке создаваемого корневого элемента
void __add_to_root_node(Topology* tp, WorkerNode* node)
{
	RootNode* new_root_node = (RootNode*) malloc(sizeof(RootNode));
	new_root_node->root_worker = node;

	new_root_node->next = tp->root;
	tp->root = new_root_node;
}


// Поиск вычислительного узла по id в топологии
// @argument tp - ссылка на топологию
// @argument id - id вычислительного узла, который ищем
// @return ссылку на найденный узел или NULL если узла нет в топологии
WorkerNode* topology_search(Topology* tp, int id)
{
	TopologyIterator iter;
	topology_iter_init(&iter, tp);

	for(WorkerNode* node = topology_iter_curr(&iter); node != NULL; node = topology_iter_next(&iter))
		if(node->id == id)
			return node;

	return NULL;
}


// Добавление вычислительного узла в топологию
// @argument tp - ссылка на топологию, в которую добавляем узел
// @argument id - id создаваемого вычислительного узла
// @argument parent_id - id родительского вычислительного узла (0, если родительский узел - управляющий)
// @return OK - элемент успешно добавлен
// @return ALREADY_EXISTS - узел с таким id уже существует
// @return NOT_EXISTS - узла с id parent_id не существует
int topology_add(Topology* tp, int id, int parent_id)
{
	if(topology_search(tp, id) != NULL)
		return ALREADY_EXISTS;

	WorkerNode* new_node = (WorkerNode*) malloc(sizeof(WorkerNode));
	worker_node_init(new_node, id, NULL);

	if(parent_id == 0)
	{
		__add_to_root_node(tp, new_node);
		return OK;
	}

	WorkerNode* parent = topology_search(tp, parent_id);

	if(parent == NULL)
	{
		free(new_node);
		return NOT_EXISTS;
	}

	new_node->next = parent->next;
	parent->next = new_node;

	return OK;
}


// Удаление вычилсительного узла из топологии
// @argument tp - ссылка на топологию, в которой будем удалять
// @argument id - id удаляемого узла
// @return NOT_EXISTS - такого узла не существует
// @return OK - успешно удален
int topology_remove(Topology* tp, int id)
{
	if(topology_search(tp, id) == NULL)
		return NOT_EXISTS;
	tp->root = __remove_worker_from_root(tp->root, id);
	return OK;
}


// Деструктор топологии
// @argument tp - ссылка на топологию, которую удаляем
void topology_destroy(Topology* tp)
{
	while(tp->root != NULL)
		topology_remove(tp, tp->root->root_worker->id);
}


/* Manager state */


static Topology topology;

static zsock_t* heartbeat_socket; // сокет для получения HEARTBEAT сообщений от вычислительных узлов
long heartbeat_interval = START_HEARTBEAT_INTERVAL; // интервал, с которым вычислительные узлы отправляют сообщения "я жив"

static zsock_t* init_socket; // сокет, на который вычислительные узлы отправляют их endpoint после инициализации

pthread_t decrement_thread; // поток декремента очков жизни вычислительных узлов
pthread_t heartbeat_listen_thread; // поток прослушивания сообщений "я жив" от вычислительных узлов


/* Heartbeat functional */


// Поток исполнения "засыпает" на msec миллисекунд
// @note Вспомогательная функция. Аналог sleep только для миллисекунд

int msleep(long msec)
{
    struct timespec ts;
    int res;

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do res = nanosleep(&ts, &ts);
    while (res && errno == EINTR);

    return res;
}


// Код для потока прослушивания вычислительного узла
void* heartbeat_listen_thread_func()
{
	while(1)
	{
		char* active_worker_id = zstr_recv(heartbeat_socket);
		if(active_worker_id == NULL)
			break;

		zstr_send(heartbeat_socket, "OK");

		WorkerNode* worker_node = topology_search(&topology, atoi(active_worker_id));
		if(worker_node != NULL) worker_node->life_points = 4;

		zstr_free(&active_worker_id);
	}

	return NULL;
}


// Код для потока уменьшения очков жизни у вычислительных узлов
void* heartbeat_decrement_thread_func()
{
	TopologyIterator iter;

	while(1)
	{
		msleep(heartbeat_interval);
		topology_iter_init(&iter, &topology);

		for(WorkerNode* node = topology_iter_curr(&iter); node != NULL; node = topology_iter_next(&iter))
		{
			if(node->life_points <= 0)
				continue;
			if(--node->life_points == 0)
				printf("Heartbeat: node %d is unavailable now\n", node->id);
		}
	}

	return NULL;
}


/* MAIN FUNCTIONAL */


// Создание дочернего процесса - вычислительного узла
// @argument worker_id - id создаваемого вычислительного узла
pid_t __create_worker_process(int worker_id)
{
	pid_t id = fork();
	if(id != 0) return id;

	char endpoint_arg[256], worker_id_arg[256];
	const char* manager_endpoint = zsock_endpoint(init_socket);

	strcpy(endpoint_arg, manager_endpoint);
	sprintf(worker_id_arg, "%d", worker_id);

	char* args[] = {WORKER_EXEC_PATH, endpoint_arg, worker_id_arg, NULL};

	execv(WORKER_EXEC_PATH, args);
	exit(1); // если запустить код узла не получилось - аварийно выходим
}


/* Usecases */


int create_worker(int worker_id, int parent_id, pid_t* created_worker_pid)
{
	pid_t pid;

	int res = topology_add(&topology, worker_id, parent_id);
	if(res != OK) return res;

	// создаем новый процесс вычислительного узла
	if(parent_id == 0)
		pid = __create_worker_process(worker_id);

	else
	{
		char worker_id_arg[256];
		WorkerNode* parent_node;
		char* reply;

		parent_node = topology_search(&topology, parent_id);
		sprintf(worker_id_arg, "%d", worker_id);

		zstr_sendx(parent_node->socket, "create", zsock_endpoint(init_socket), worker_id_arg, NULL);
		reply = zstr_recv(parent_node->socket);

		if(reply == NULL)
		{
			topology_remove(&topology, worker_id);
			return UNACTIVE;
		}

		sscanf(reply, "%d", &pid);
		zstr_free(&reply);
	}

	// Инициализируем сокет для общения с вычислительным узлом
	char* worker_endpoint = zstr_recv(init_socket);
	zstr_send(init_socket, "OK");

	WorkerNode* worker_node = topology_search(&topology, worker_id);
	worker_node->socket = zsock_new_req(worker_endpoint);
	zsock_set_rcvtimeo(worker_node->socket, MAX_TIMEOUT);
	zsock_set_sndtimeo(worker_node->socket, MAX_TIMEOUT);

	// Кидаем ему запрос, чтобы он присылал сообщения "я жив"
	char interval_arg[256];

	sprintf(interval_arg, "%ld", heartbeat_interval);
	zstr_sendx(worker_node->socket, "heartbeat", interval_arg , zsock_endpoint(heartbeat_socket), NULL);
	free(zstr_recv(worker_node->socket));

	*created_worker_pid = pid;
	return 0;
}


int set_heartbeat_interval(long interval)
{
	char interval_arg[256];
	TopologyIterator iter;

	sprintf(interval_arg, "%ld", interval);
	topology_iter_init(&iter, &topology);
	heartbeat_interval = interval;

	for(WorkerNode* node = topology_iter_curr(&iter); node != NULL; node = topology_iter_next(&iter))
	{
		zstr_sendx(node->socket, "heartbeat", interval_arg , zsock_endpoint(heartbeat_socket), NULL);
		free(zstr_recv(node->socket));
	}

	return OK;
}


void print_node_states()
{
	TopologyIterator iter;
	topology_iter_init(&iter, &topology);

	for(WorkerNode* node = topology_iter_curr(&iter); node != NULL; node = topology_iter_next(&iter))
		printf("%d\t|\t%s\n", node->id, (node->life_points) ? "online" : "offline");
}


int main()
{
	// Инициализация сокетов
	// используем wildcard, чтобы ОС сама определила свободный порт
	init_socket = zsock_new_rep("tcp://0.0.0.0:*");
	heartbeat_socket = zsock_new_rep("tcp://0.0.0.0:*");

	zsock_set_rcvtimeo(init_socket, MAX_TIMEOUT);
	zsock_set_sndtimeo(init_socket, MAX_TIMEOUT);

	topology_init(&topology);

	pthread_create(&heartbeat_listen_thread, NULL, heartbeat_listen_thread_func, NULL);
	pthread_create(&decrement_thread, NULL, heartbeat_decrement_thread_func, NULL);

	while(1)
	{
		char command_type[256];
		printf(">>> ");
		scanf("%s", command_type);

		if(!strcmp(command_type, "create"))
		{
			int worker_id, parent_id, res;
			pid_t created_worker_pid;

			scanf("%d %d", &worker_id, &parent_id);
			res = create_worker(worker_id, parent_id, &created_worker_pid);

			if(res == OK)
				printf("Ok: %d\n", created_worker_pid);
			if(res == ALREADY_EXISTS)
				printf("Error: Already exists\n");
			if(res == NOT_EXISTS)
				printf("Error: Parent not exists\n");
			if(res == UNACTIVE)
				printf("Error: Parent is unavailable\n");
		}

		else if(!strcmp(command_type, "remove"))
		{
			int worker_id, result;

			scanf("%d", &worker_id);
			result = topology_remove(&topology, worker_id);

			if(result == NOT_EXISTS)
				printf("Error: Not found\n");
			if(result == OK)
				printf("Ok\n");
		}

		else if(!strcmp(command_type, "heartbeat"))
		{
			long interval;
			scanf("%ld", &interval);

			set_heartbeat_interval(interval);
		}

		else if(!strcmp(command_type, "exec"))
		{
			int worker_id;
			char method[256];
			WorkerNode *node;

			scanf("%d %s", &worker_id, method);
			node = topology_search(&topology, worker_id);

			if(node == NULL)
			{
				printf("Error:%d: Not found\n", worker_id);
				continue;
			}

			if(!strcmp(method, "start"))
			{
				zstr_send(node->socket, "timer-start");
				char* reply = zstr_recv(node->socket);

				if(reply == NULL)
					printf("Error:%d: Node is unavailable\n", worker_id);
				else
					printf("Ok:%d\n", worker_id);
			}

			else if(!strcmp(method, "stop"))
			{
				zstr_send(node->socket, "timer-stop");
				char* reply = zstr_recv(node->socket);

				if(reply == NULL)
					printf("Error:%d: Node is unavailable\n", worker_id);
				else
					printf("Ok:%d\n", worker_id);
			}

			else if(!strcmp(method, "time"))
			{
				zstr_send(node->socket, "timer-time");
				char* reply = zstr_recv(node->socket);

				if(reply == NULL)
					printf("Error:%d: Node is unavailable\n", worker_id);
				else
					printf("Ok:%d: %s\n", worker_id, reply);
			}

			else
				printf("Error:%d: Unknown method", worker_id);
		}

		else if(!strcmp(command_type, "heartbeat-info"))
			print_node_states();

		else if(!strcmp(command_type, "exit"))
			break;

		else
			printf("Error: Unknown command\n");
	}

	pthread_cancel(heartbeat_listen_thread);
	pthread_cancel(decrement_thread);

	topology_destroy(&topology);

	zsock_destroy(&init_socket);
	zsock_destroy(&heartbeat_socket);

	return 0;
}
