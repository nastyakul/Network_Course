# Проект по курсу "Сетевые технологии"
В этом проекте мы попробовали реализовать взаимодейтсвие "клиент-сервер" и рассмотреть такую практическую задачу, 
как удаленный доступ к командной строке. Действительно, системному администратору может быть необходимо запустить какую-либо команду
на удалённом компьютере, не вставая с рабочего места. Это, к примеру, пригодится для установки программ или утилит, 
изменения настроек на рабочих компьютерах сети.
## Установка
Проект состоит из следующих файлов: [client](https://github.com/nastyakul/Network_Course/blob/master/Client/client.cpp) и
[server с header-ами](https://github.com/nastyakul/Network_Course/tree/master/Server). Перед запуском необходимо прописать IPv4 адрес 
сети в переменную HOST клиента и сервера. Кроме того, необходимо отключить Барндмауэр Windows.
## Запуск
По мере запуска выводятся этапы работы приложения-сервера:
```
Intializing Winsock...
Setting up server...
Creating server socket...
Binding socket...
Listening...
```
А также приложения-клиента:
```
Starting Client...
Connecting...
Successfully Connected
```
Выводятся и сообщения об ошибках подключения.
## Тесты
Далее можно работать с комнадной строкой удаленного компьютера, попробовав простейшие команды:
```
dir 
cd
help
```
Shortcuts в текущей версии проекта не реализованы.
## Компиляция

* [Visual Studio 2015 Professional](https://www.microsoft.com/ru-ru/SoftMicrosoft/vs2015professional.aspx)

## Авторы

* Самал Кубентаева — server
* [Анастасия Кулакова](https://github.com/nastyakul) — server
* [Елизавета Рыбка](https://github.com/EliseRybka) — client
