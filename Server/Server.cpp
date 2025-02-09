// Очередь
queue<string> clients;

// Заглушка для пула потоков
pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER;

// Постоянная пустой очереди
pthread_cond_t pool_cond = PTHREAD_COND_INITIALIZER;

// Постоянная заполненой очереди
pthread_cond_t full_pool = PTHREAD_COND_INITIALIZER;

// Карта клиентов
map<int, int> unsatisfied_clients; 

// Заглушка для карты
pthread_mutex_t uc_mutex = PTHREAD_MUTEX_INITIALIZER;

// Карта заглушек
map<int, pthread_mutex_t> socketmap;

// Заглушка сокета
pthread_mutex_t socket_mutex = PTHREAD_MUTEX_INITIALIZER;

// Начальные переменные
int thread_pool_size;
int queue_size;
int block_size;
int PORT;

int main(int argc, char **argv){

    // Считывание вводных аргументов
    if (argc != 9)
    {
        std::cerr << "Wrong arguments.\nFormat: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size> -b <block_size>\n";
        exit(1);
    }
    // Интерпретация опции
    int cc;
    while ((cc = getopt(argc, argv, "p:s:q:b:"))!=-1)
    {
        switch(cc){
            // Порт
            case 'p':
                PORT = atoi(optarg);
                break;
            // Размер пула потоков
            case 's':
                thread_pool_size = atoi(optarg);
                break;
            // Размер очереди
            case 'q':
                queue_size = atoi(optarg);
                break;
            // Размер блока данных
            case 'b':
                block_size = atoi(optarg);
                break;
            // Несуществующая опция
            default:
                std::cerr << "Wrong arguments.\nFormat: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size> -b <block_size>\n";
                exit(1);
        }
    }

    // Вывод сообщения о считаных аргуметах
    std::cout<< "\nServer's parameters are:\n";
    std::cout<< "port: "<< PORT <<"\n";
    std::cout<< "thread_pool_size: "<< thread_pool_size <<"\n";
    std::cout<< "queue_size: "<< queue_size <<"\n";
    std::cout<< "block_size: "<< block_size <<"\n";

    // Задание пула потоков 
    pthread_t thread_pool[thread_pool_size];
    for(int i=0;i< thread_pool_size;i++)
    {
        // Задание рабочего потока
        pthread_create(&thread_pool[i], NULL, worker_thread, NULL);
    }

    // Создание сокета
    int socketFP = errorcheck(socket(AF_INET, SOCK_STREAM, 0 ), "creating socket");

    // Вывод сообщения об успешной инициализации сервера
    bind_port(socketFP, PORT);
    std::cout << "Server was successfully initialised...\n";
    
    // Ожидание подключения к сокету
    errorcheck(listen(socketFP, BACKLOG), "listen");
    std::cout << "Listening for connections on port "<< PORT << "\n";

    // Цикл работы сервера
    while(1)
    {
        // Подключение
        struct sockaddr_in addr;
        socklen_t addr_len;
        int connectFP;
        errorcheck(connectFP = accept(socketFP, (SA*) &addr, &addr_len), "accept connection");            
        std::cout << "Accepted connection from " << inet_ntoa(addr.sin_addr) << "\n\n";

        int* clientSocket = new int;
        *clientSocket = connectFP;

        // Активация заглушки
        pthread_mutex_lock(&socket_mutex);

        // Задание новой заглушки, если остальные активны
        if(socketmap.find(connectFP)==socketmap.end())
        {
            pthread_mutex_t new_socket_mutex = PTHREAD_MUTEX_INITIALIZER;
            socketmap.insert(std::pair<int, pthread_mutex_t>(connectFP, new_socket_mutex));
        }
        pthread_mutex_unlock(&socket_mutex);

        // Взаимодействие потока с сокетом
        pthread_t com_thread;
        pthread_create(&com_thread, NULL, get_client_request, clientSocket);
        pthread_detach(com_thread);
        
    }
    
    return 0; 
}
