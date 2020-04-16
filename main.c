//  C L I E N T 
#include <sys/types.h>
#include <sys/socket.h>
#include "message.h"
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define BUF 4095
#undef signal

// Global variables
char name[32];
int socketFD = 0;
pthread_t thread[2];
volatile sig_atomic_t active = 1; //exit-Flag

void tippeEtwas() 
{
    printf("%s", "> ");
    fflush(stdout);
}
// entfernt das \n
void compressMe(char *arr, int length)
{
    int i;
    for(i=0; i<length; i++)
    {
        if(arr[i] == '\n')
        {
            arr[i] = '\0';
            break;
        }
    }
}
// Der Client will sich beenden
void exitControl(int sig) 
{
    write( socketFD, "4", 10);
    write( socketFD, "exit", 4);
    sleep(1);
    close(socketFD);
    exit(0);
}
// Zeitstempel
char *getStamp()
{
    time_t ltime;       //Kalender-Zeit 
    ltime=time(NULL);   // Aktuelle Zeit
    char *x = asctime( localtime( &ltime ));
    compressMe(x, strlen(x));
    return x;
}
char *getln() 
{
    char *text = NULL, *tmp = NULL;
    ssize_t size = 0, index = 0;
    int letter = EOF;

    while ( letter ) 
    {
        letter = getc(stdin);

        // Eingabe beendet?
        if (letter == '\n' || letter == EOF)
            letter = 0;

        // Mehr Speicher notwendig?
        if (size <= index) 
        {
            size += sizeof (char) * 4;
            tmp = realloc(text, size);
            if ( !tmp ) 
            {
                free(text);
                text = NULL;
                break;
            }
            text = tmp;
        }
        // Hänge Buchstabe an Eingabestring
        text[index++] = letter;
    }
    return text;
}
void client_writer() 
{
    char *message = malloc( sizeof( char ));
    char textLaenge[10];
    int msg_len =sizeof( char ), status =0, aktiv =1;
    
    while( aktiv )
    {
        bzero(message, msg_len);
        bzero( textLaenge, 10);
        
        tippeEtwas();
        message = getln();
        msg_len = strlen(message);
        compressMe(message, msg_len);
        
        if( strcmp(message, "exit" ) == 0 )
            exitControl(2);
        sprintf(textLaenge, "%d\n", msg_len);
            
        status = write( socketFD, textLaenge, 10);
        status = write( socketFD, message, msg_len);
    }
    return;
}

void broadcast_reader() 
{
    MSG *msg = (MSG *)malloc( sizeof( MSG ));
    char textLaenge[10];
    int status =0, aktiv =1;

    while( aktiv )
    {
        bzero( msg->text, sizeof( msg->text ));
        bzero( msg->name, 32);
        bzero(textLaenge, 10);
         
        status = read( socketFD, msg->name, 32);
        status = read( socketFD, textLaenge, 10);
        msg->len = ( uint16_t )atoi( textLaenge );
        status = read( socketFD, msg->text, msg->len );
        
        if(status > 0)
        {
            if( strcmp( msg->text, "exit") == 0)
            {
                printf("%s hat den chat verlassen\n", msg->name);
                continue;
            }
            if( strcmp( msg->text, "welcome") ==0)
            {
                printf("%s ist dem Chat beigetreten\n",msg->name);
                continue;
                
            }
            if( strcmp( msg->text, "byebye") ==0 && strcmp( msg->name, "Server") ==0)
            {
                printf("%s hat seinen Dienst beendet.\n",msg->name);
                exit(0);
            }
            // Ab in die Message Queue
            else if( strlen( msg->text ) > 0)
            {
                compressMe( msg->text, msg->len );
                printf("%s:\t\t\t[%s]:\n\t%s\n", msg->name, getStamp(), msg->text );
                tippeEtwas();
                continue;
            }
        }
        else if(strcmp( textLaenge, "0\n") == 0) 
        {
            strcpy( msg->text, "\n");
            printf("%s: \n", msg->name);
            continue;
        }
        else
            aktiv =0;
    }
}

int main(int argc, char **argv)
{
    char vergeben [3];
    char *ip = "127.0.0.1";
    const in_port_t port = 8111;
    struct sockaddr_in server_addr;
    signal(SIGINT, exitControl);    //Signalhandler für STRG+C
    
    printf("Bitte Namen eingeben: ");
    strcpy(name, getln());
    compressMe(name, strlen(name));

    if (strlen(name) > 32 || strlen(name) < 2)
    {
	printf("Name Sollte zwischen 2 und 32 Zeichen sein\n");
	return EXIT_FAILURE;
    }

    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port        = htons(port);


  // Connect to Server
    int err = connect(socketFD, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err == -1) 
    {
        printf("Fehler beim Verbinden\n");
        return EXIT_FAILURE;
    }
    
    // Sendet name
    write(socketFD, name, 32);
    // prüft, ob name vergeben ist
    read(socketFD, vergeben ,3);
    
    if( strcmp( vergeben, "0") == 0){
        printf("-> %s <-  ist schon im Chat\n",name);
        return EXIT_FAILURE; 
    }
    printf("=== Herzlich Willkommen im Chat ===\n");

    if( pthread_create( &thread[0], NULL, (void *) client_writer, NULL) != 0 )
    {
	printf("Fehler beim Thread-erzeugen\n");
        return EXIT_FAILURE;
    }

    if( pthread_create( &thread[1], NULL, (void *) broadcast_reader, NULL ) != 0 )
    {
        printf("Fehler beim Thread-erzeugen\n");
	return EXIT_FAILURE;
    }

    while (1)
    {
	if( ! active )
            break;
    }
    pthread_exit( (void *)thread[1] );
    pthread_exit( (void *)thread[0] );
    close( socketFD );
    return EXIT_SUCCESS;
}