// Обработка ошибок
int errorcheck(int i, const char * message);

// Привязка сокета
int bind_port(int socketFD, short port);

// Отправка метаданных клиенту
void* handle_client_request(string client_request);

// Обработка клиентского запроса
void* get_client_request(void* conFD);

// Назначение запроса из очереди потоку
void* worker_thread(void* argi);

// Каталогизация
void сatalog(string dirpath, int connectFD);

// Подсчёт файлов
void count_files(string dirpath, int connectFD);
