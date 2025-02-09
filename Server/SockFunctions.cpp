// Обработка ошибок
int errorcheck(int i, const char * message)
{    
    // Если возвращаемое значение -1
    if (i ==-1)
    {
        // Вывод сообщения об ошибке и выход из программы
        std::cerr << "Error Encountered with: " << message << std::endl;
        std::cerr << "Errno: " << errno << std::endl;
        _exit(1);
    }

    // Иначе возращает значение
    return i;
}

// Привязка сокета
int bind_port(int socketFP, short port)
{
    // Адресная структура
    struct sockaddr_in server;

    // Задание сетевого домена AF INET
    server.sin_family = AF_INET;
    
    // Задание алпеса
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    // Номер TCP порта 
    server.sin_port = htons(port);

    // Привязка сокета к адресу 
    return errorcheck(bind(socketFP, (SA *) &server, sizeof(server)), "binding socket");
}

// Обработка клиентского запроса
void* get_client_request(void* conFP)
{
    int connectFP = *((int*)conFP);
    delete (int*)conFP;

    int buffer=4096;
    
    // Задание буффера
    size_t bytes_read;
    char buff[buffer];
    memset(buff, '\0', sizeof(char)*buffer);
    string dirpath = "";

    // Чтение
    while((bytes_read = read(connectFP, &buff, sizeof(char)*buffer))>0)
    {
        dirpath +=buff;
        memset(buff, '\0', sizeof(char)*buffer);
        if (bytes_read < buffer) break;
    }

    // Проверка успешности
    errorcheck(bytes_read, "read from socket");

    // Вывод сообщения
    std::cout << "[Thread: "<< std::hash<std::thread::id>()(std::this_thread::get_id()) << "]: About to scan directory "<<  dirpath << std::endl;

    // Число файлов
    count_files(dirpath, connectFP);

    // Отправка размера буффера данных
    uint32_t server_block_size = htonl(block_size);
    write(connectFP, &server_block_size, sizeof(uint32_t));

    // Отправка числа файлов
    int counter= unsatisfied_clients[connectFP];
    uint32_t count = htonl(counter);
    write(connectFP, &count, sizeof(uint32_t));

    // Задание каталога
    catalog(dirpath, connectFP);

    return NULL;
}

// Подсчёт файлов в дериктории
void count_files(string dirpath, int connectFP)
{    
    if(dirpath.back()=='\0')
    {
        dirpath.pop_back();
    }

    if(dirpath.back()!='/')
    {
        dirpath+="/";
    }
    string pathie = dirpath;
    dirpath+='\0';

    char directory[dirpath.length()];
    strcpy(directory, dirpath.c_str());
    
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(directory)) != NULL) 
    {
        // Для всех директорий
        while ((ent = readdir(dir)) != NULL) 
        {
            string pathfile = pathie;
            pathfile += ent->d_name;
            pathfile += '\0';
            char pathname[pathfile.length()];
            strcpy(pathname, pathfile.c_str());
            
            // Если это не текущая или родительская директория
            if (!( !strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..") ))
            {
                struct stat pathINFO;
                stat(pathname, &pathINFO);
                if( S_ISREG(pathINFO.st_mode))
                {
                    pthread_mutex_lock(&uc_mutex);

                    // Если нет в карте, добавить
                    if(unsatisfied_clients.find(connectFP)==unsatisfied_clients.end())
                    {
                        unsatisfied_clients.insert(std::pair<int, int>(connectFP, 1));
                    }
                    else
                    {   
                        // иначе увеличить счётчик
                        unsatisfied_clients[connectFP]+=1;
                    }
                    pthread_mutex_unlock(&uc_mutex);

                }
                else if( S_ISDIR(pathINFO.st_mode))
                {
                    // Если это дериктория, рекурсивно вызвать функцию подсчёта файлов для неё
                    pathfile.pop_back();
                    count_files(pathfile, connectFP);
                }
            }
        }
        closedir(dir);
    } 
    else 
    {
        // если указанная директория не была найдена
        errorcheck(-1, "opening directory");
    }

}

// Каталогизация
void catalog(string dirpath, int connectFP)
{
    if(dirpath.back()=='\0')
    {
        dirpath.pop_back();
    }
    if(dirpath.back()!='/')
    {
        dirpath+="/";
    }

    string pathie = dirpath;
    dirpath+='\0';
    char directory[dirpath.length()];
    strcpy(directory, dirpath.c_str());
    
    // Открыть директорию
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(directory)) != NULL) 
    {
        // Для всех директорий
        while ((ent = readdir(dir)) != NULL) 
        {
            string pathfile = pathie;
            pathfile += ent->d_name;
            pathfile += '\0';
            char pathname[pathfile.length()];
            strcpy(pathname, pathfile.c_str());
            
            // Если это не текущая или родительская директория
            if (!( !strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..") ))
            {
                struct stat pathINFO;
                stat(pathname, &pathINFO);
                
                if( S_ISREG(pathINFO.st_mode))
                {
                    // Сформулировать запрос (ключ + путь)
                    string client_and_file = std::to_string(connectFP);
                    client_and_file += " ";
                    client_and_file += pathfile;
                    
                    pthread_mutex_lock(&pool_mutex);

                    // Пока очередь заполнена, ожидать
                    while(clients.size()>=queue_size)
                        pthread_cond_wait(&full_pool, &pool_mutex);
                    
                    // Вывести сообщение
                    std::cout << "[Thread: "<< std::hash<std::thread::id>()(std::this_thread::get_id()) << "]: Adding file "<< pathfile<<" to the queue..."<< std::endl;

                    // Добавить запрос в очередь
                    clients.push(client_and_file);
                    pthread_cond_signal(&pool_cond);                   
                    pthread_mutex_unlock(&pool_mutex);

                }
                else if( S_ISDIR(pathINFO.st_mode))
                {
                    // Убрать окончание строки
                    pathfile.pop_back();
                    
                    // Рекурсивный вызов каталогизации
                    catalog(pathfile, connectFP);
                }
            }
        }
        closedir(dir);
    } 
    else 
    {
        // При ошибке открытия
        errorcheck(-1, "opening directory");
    }

}

// Назначение запроса из очереди потоку
void* worker_thread(void* argi)
{
    while(1)
    {
        // Активация заглушки
        pthread_mutex_lock(&pool_mutex);

        // Ожидать, пока очередь пуста
        while(clients.empty())
            pthread_cond_wait(&pool_cond, &pool_mutex);   
            
        // Извлечь запрос из очереди
        string clientrequest = clients.front();
        clients.pop();

        // Вывод сообщения
        std::cout << "[Thread: "<< std::hash<std::thread::id>()(std::this_thread::get_id()) << "]: Received task: <"<<clientrequest<<">"<< std::endl;
        
        // Ожидание при заполнении пула
        pthread_cond_signal(&full_pool);
        pthread_mutex_unlock(&pool_mutex);

        // Вызов функции для отправки метаданных клиенту
        handle_client_request(clientrequest);
    }

    return NULL;

}

// Отправка метаданных клиенту
void* handle_client_request(string client_request)
{
    // Получение сокета
    string CFP = "";
    int i=0;
    while (client_request[i]!=' ')
    {
        CFP+=client_request[i];
        i++;
    }
    int connectFP = stoi(CFP);
    i++;

    // Получение пути к файлу
    string file_in_question = "";
    while (client_request[i]!='\0')
    {
        file_in_question+=client_request[i];
        i++;
    }
    file_in_question+='\0';
    
    // Нахождение нужного сокета на карте
    pthread_mutex_lock(&socket_mutex);
    if(socketmap.find(connectFP)==socketmap.end())
    {
        errorcheck(-1, "mutex for socket unavailable");
    }
    pthread_mutex_unlock(&socket_mutex);

    // Активация заглушки
    pthread_mutex_lock(&socketmap[connectFP]);

    std::cout << "[Thread: "<< std::hash<std::thread::id>()(std::this_thread::get_id()) << "]: About to read file "<<file_in_question << std::endl;

    // Отправляемый файл
    char filepath[file_in_question.length()];
    strcpy(filepath, file_in_question.c_str());

    // Открытие файла
    int FPR = errorcheck(open(filepath, O_RDONLY), "open");

    // Отправить размер полного имени файла
    uint32_t name_size = htonl(file_in_question.length());
    write(connectFP, &name_size, sizeof(uint32_t));
    
    // Отправить путь до файла
    write(connectFP, &filepath, file_in_question.length());

    // Узнать размер файла
    struct stat metadata;
    stat(filepath, &metadata);

    // Преобразование в uint32_t т.к. работаю на Linux
    uint32_t file_size = htonl(metadata.st_size);

    // Отправка размера файла
    write(connectFP, &file_size, sizeof(uint32_t));

    // Пакетная отправка файла
    size_t bytes_read;
    char buff[block_size];
    memset(buff, '\0', sizeof(char)*block_size);
    while((bytes_read = read(FPR, &buff, block_size))>0)
    {
        write(connectFP, buff, bytes_read);
        memset(buff, '\0', sizeof(char)*block_size);
    }

    // Закрыть файл
    close(FPR);

    // Активация заглушки
    pthread_mutex_lock(&uc_mutex);

    // Проверка карты
    if(unsatisfied_clients.find(connectFP)==unsatisfied_clients.end())
    {
        errorcheck(-1, "map unsatisfied_clients doesn't have this key.");
    }
    else if(unsatisfied_clients[connectFP]>1)
    {
        unsatisfied_clients[connectFP]-=1;
    }
    else 
    {
        unsatisfied_clients.erase(unsatisfied_clients.find(connectFP));
        close(connectFP);
    }
    pthread_mutex_unlock(&uc_mutex);

    // Снять заглушку
    pthread_mutex_unlock(&socketmap[connectFP]);

    return NULL;

}
