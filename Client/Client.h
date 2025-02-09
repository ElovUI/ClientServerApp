/* Библиотека клиента */

// Подключаемые библиотеки
#include <cstring>     // Си-образные строки
#include <string>      // строки
#include <cstdio>      // методы ввода/вывода
#include <fcntl.h>     // библиотека файлового дескриптора
#include <sys/stat.h>  // библиотека считывания состояния файла
#include <sys/types.h> // библиотека базовых системных переменных
#include <errno.h>     // библиотека обработки ошибок
#include <sys/wait.h>  // библиотека задержек
#include <thread>      // потоки
#include <dirent.h>    // библиотека для работы с директориями
#include <libgen.h>    // библиотека для работы с путями к директориям
#include <sys/socket.h>// билиотека сокета
#include <pthread.h>   // библиотека потоков
#include <sys/ioctl.h> // библиотека управления базовыми параметрами устройства
#include <netinet/in.h>// библиотека интернет адресов
#include <arpa/inet.h> // билиотека интернет операций
#include <netdb.h>     // библиотека сетевых бах данных

// Обработка ошибок
int errorcheck(int i, const char * message);

// Для подключения сокета
#define SA struct sockaddr
