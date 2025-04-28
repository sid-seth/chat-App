To compile the code you need to run following commands which makes the compiler aware to add these ws2tcpip library 

first run the server command and then the client command
1) "run the executable .exe of updatedserver"
2) "run the executable .exe of updatedclient"


In case you want to make changes and compile the code you need following commands to compile the cpp file
1) "g++ updatedserver.cpp -o updatedserver.exe -lws2_32"
2) "g++ updatedclient.cpp -o updatedclient.exe -lws2_32"
