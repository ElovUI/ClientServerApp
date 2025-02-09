/* Клиент */

int main(int argc, char *argv[])
{
    /* Задание переменных */
    // Порт клиента
    int port;
    // Сокет клиента
    int sockFP;     
    // ip сервера
    struct hostent *server_ip;
    // Адрес директории
    char *directory;

    // Считывание вводных аргументов
    if (argc != 7)
    {
        std::cerr << "Wrong arguments.\nFormat: ./remoteClient -i <server_ip> -p <server_port> -d <directory>\n";
        exit(1);
    }
    // Интерпретация опции
    int cc;
    while ((cc = getopt(argc, argv, "i:p:d:"))!=-1)
    {
        switch(cc)
        {
            // ip сервера
            case 'i':
                if ((server_ip = gethostbyname(optarg)) == NULL)
                {
                    herror(" gethostbyname ");
                    exit(1);
                }
                break;
            // Порт
            case 'p':
                port = atoi(optarg);
                break;
            // Адрес директории
            case 'd':
                directory = optarg;
                break;
            // Несуществующая опция
            default:
                std::cerr << "Wrong arguments.\nFormat: ./remoteClient -i <server_ip> -p <server_port> -d <directory>\n";
                exit(1);
        }
    }

    // Вывод сообщения о считаных аргуметах
    std::cout<< "\nClient's parameters are:\n";
    std::cout<< "serverIP: "<< server_ip->h_name <<"\n";
    std::cout<< "port: "<< port <<"\n";
    std::cout<< "directory: "<< directory <<"\n";

    struct sockaddr_in server;

    // Создание сокета
    errorcheck(sockFP = socket(AF_INET, SOCK_STREAM, 0), "socket");

    // Задание сетевого домена AF INET
    server.sin_family = AF_INET;
    memcpy(&server.sin_addr, server_ip->h_addr, server_ip->h_length);

    // Задание TCP порта
    server.sin_port = htons(port);

    // Соединение с сервером
    errorcheck(connect(sockFP, (SA *) &server, sizeof(server)), "connect");

    // Вывод сообщения об успешном соединении
    std::cout << "Established Connection to: " << inet_ntoa(server.sin_addr) << " on port: " << port << "\n\n";
    
    // Перевод адреса директории в строку
    char folder[strlen(directory)+1];
    for(int i=0;i<strlen(directory);i++)
    {
        folder[i]=directory[i];
    }
    folder[strlen(directory)]='\0';

    // Запись адреса директории в сокет
    errorcheck(write(sockFP, folder, strlen(directory)+1), "write path");

    // Считывание размера буффера считываемых данных
    size_t bytes_read = 0;
    uint32_t server_block_size;
    bytes_read = read(sockFP, &server_block_size, sizeof(uint32_t));
    if(bytes_read!=sizeof(uint32_t)) errorcheck(-1, "read");
    uint32_t block_size = ntohl(server_block_size);

    // Считывание числа считываемых файлов
    uint32_t folder_file_count;
    bytes_read = read(sockFP, &folder_file_count, sizeof(uint32_t));
    if(bytes_read!=sizeof(uint32_t)) errorcheck(-1, "read");
    uint32_t file_count = ntohl(folder_file_count);

    char buff[block_size];
    // Для каждого файла
    for(int i =0 ; i<file_count;i++)
    {

        // Считывание размера имени файла
        uint32_t name_size;
        bytes_read = read(sockFP, &name_size, sizeof(uint32_t));
        if(bytes_read!=sizeof(uint32_t)) errorcheck(-1, "read");
        uint32_t filepath_size = ntohl(name_size);

        // Считывание полного имени файла
        bytes_read = read(sockFP, &buff, filepath_size);
        if(bytes_read!=filepath_size) errorcheck(-1, "read");

        char filename[filepath_size];
        strncpy(filename, buff, filepath_size);

        // Проверка существования директории
        char *directory = dirname(buff);
        struct stat dirStat;
        if(!(stat(directory, &dirStat)==0 && S_ISDIR(dirStat.st_mode)))
        {

            // Если указаная директория не существует, создаём её
            char st[strlen(directory)+10];
            sprintf(st, "mkdir -p %s", directory);
            system(st);
        }

        // Проверка существования файла
        struct stat fileStat;
        if(stat(buff, &fileStat)==0 && S_ISREG(fileStat.st_mode))
        {    
            // Если указаный файл существует, удаляем её
            char st[filepath_size+4];
            sprintf(st, "rm %s", buff);
            system(st);
        }

        // Создание файла
        int fp = errorcheck(open(filename, O_WRONLY | O_CREAT, 0644), "open");

        // Считывание размера файла
        uint32_t file_sizeH;
        bytes_read = read(sockFP, &file_sizeH, sizeof(uint32_t));
        if(bytes_read!=sizeof(uint32_t)) errorcheck(-1, "read");
        size_t file_size = ntohl(file_sizeH);
        
        // Очистка буфера
        memset(buff, '\0', sizeof(char)*block_size);
        
        // Пока не считан файл
        while(file_size !=0 )
        {
            // Если осталось считать больше емкости буффера
            if(file_size>block_size)
            { 
                bytes_read = read(sockFP, &buff, block_size);
                if(bytes_read!=block_size) errorcheck(-1, "read");
                file_size-=bytes_read;
            }
            else
            {
                bytes_read = read(sockFP, &buff, file_size);
                if(bytes_read!=file_size) errorcheck(-1, "read");
                file_size-=bytes_read;
            }

            // Запись считаного в файл
            int dd = write(fp, buff, bytes_read);

            // Проверка успешности записи
            if (dd != bytes_read) 
                errorcheck(-1, "write to file");

            // Очистка буффера
            memset(buff, '\0', sizeof(char)*block_size);
        }
        
         // Закрытие файла
        close(fp);

        // Вывод сообщения о считывании файла
        std::cout << "Received: " << filename << "\n";
    }
    close(sockFP);
}

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
