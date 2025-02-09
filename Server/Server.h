/* Библиотека Сервера */

// Подключаемые библиотеки
#include <map>           // карта
#include <unistd.h>      // библиотека стандартных постоянных и типов
#include <fcntl.h>       // библиотека файлового дескриптора
#include <cstring>       // Си-образные строки
#include <iterator>      // иттератор
#include <sys/stat.h>    // библиотека считывания состояния файла
#include <sys/types.h>   // библиотека базовых системных переменных
#include <errno.h>       // библиотека обработки ошибок
#include <sys/wait.h>    // библиотека задержек
#include <queue>         // очередь
#include <thread>        // потоки
#include <dirent.h>      // библиотека для работы с директориями
#include <sys/ioctl.h>   // библиотека управления базовыми параметрами устройства
#include <sys/socket.h>  // библиотека сокетов
#include <pthread.h>     // библиотека потоков
#include <netinet/in.h>  // библиотека интернет адресов
#include <arpa/inet.h>   // билиотека интернет операций
#include <netdb.h>       // Библиотека сетевых бах данных

// Максимальная длина очереди
#define BACKLOG 1280

// Для подключения сокета
#define SA struct sockaddr

using std::queue;        // пространство имен очередь
using std::string;       // пространство имен строка
using std::map;          // пространство имен карта

// Очередь
extern queue<string> clients; 

// Карта клиентов
extern map<int, int> unsatisfied_clients; 

// Заглушка для карты
extern pthread_mutex_t uc_mutex;

// Заглушка для пула потоков
extern pthread_mutex_t pool_mutex;

// Переменная для пустой очереди
extern pthread_cond_t pool_cond;

// Переменная для полной очереди
extern pthread_cond_t full_pool;

// Начальные переменные
extern int block_size;
extern int thread_pool_size;
extern int queue_size;

// Карта заглушек
extern map<int, pthread_mutex_t> socketmap;

// Заглушка сокета
extern pthread_mutex_t socket_mutex;
