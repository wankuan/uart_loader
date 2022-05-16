// reference from github.com/rifotu/Serial-Port-Programming-on-Linux/
/*====================================================================================================*/
/* Serial Port Programming in C (Serial Port Write)                                                   */
/* Non Cannonical mode                                                                                */
/*----------------------------------------------------------------------------------------------------*/
/* Program writes a character to the serial port at 9600 bps 8N1 format                               */
/* Baudrate - 9600                                                                                    */
/* Stop bits -1                                                                                       */
/* No Parity                                                                                          */
/*----------------------------------------------------------------------------------------------------*/
/* Compiler/IDE  : gcc 4.6.3                                                                          */
/* Library       :                                                                                    */
/* Commands      : gcc -o serialport_write serialport_write.c                                         */
/* OS            : Linux(x86) (Linux Mint 13 Maya)(Linux Kernel 3.x.x)                                */
/* Programmer    : Rahul.S                                                                            */
/* Date	         : 21-December-2014                                                                   */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>


int fd; /*File Descriptor*/

void *test_write_thread(void *args)
{
    printf("\n +----------------------------------+");
    printf("\n |        Serial Port WRITE         |");
    printf("\n +----------------------------------+");
    printf("\n");

    char write_buffer[512] = "Wankuan Huang";
    write_buffer[strlen(write_buffer)] = '\n';
    write_buffer[strlen(write_buffer) + 1] = '\0';
    int  bytes_write = strlen(write_buffer) + 2;
    printf("write_buffer string size:%d\n", bytes_write);
    while (1) {
        bytes_write = write(fd, write_buffer, bytes_write);
        printf("[WRITE MONITOR] msg:%s actual_write_size:%d\n", write_buffer, bytes_write);

        sleep(1);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    // args ./a.out /dev/ttyxx 921600
    if (argc < 3) {
        printf("usages:....\n");
        printf("      ./a.out /dev/ttyxx 921600\n");
        printf("no input args..\n");
        return -1;
    }
    uint32_t baudrate = atoi(argv[2]);
    char *   name     = argv[1];
    if (baudrate > 921600) {
        printf("baudrate:%d over max:%d..exit\n", baudrate, 921600);
        return -1;
    }

    printf("\n +----------------------------------+");
    printf("\n |        Serial Port Read          |");
    printf("\n +----------------------------------+");

    /*------------------------------- Opening the Serial Port -------------------------------*/

    /* Change /dev/ttyUSB0 to the one corresponding to your system */

    fd = open(name, O_RDWR | O_NOCTTY); /* ttyUSB0 is the FT232 based USB2SERIAL Converter   */
                                        /* O_RDWR   - Read/Write access to serial port       */
                                        /* O_NOCTTY - No terminal will control the process   */
                                        /* Open in blocking mode,read will wait              */

    if (fd == -1) /* Error Checking */
        printf("\n  Error! in Opening ttyUSB0  ");
    else
        printf("\n  ttyUSB0 Opened Successfully ");

    /*---------- Setting the Attributes of the serial port using termios structure --------- */

    struct termios SerialPortSettings; /* Create the structure                          */

    tcgetattr(fd, &SerialPortSettings); /* Get the current attributes of the Serial port */

    /* Setting the Baud rate */
    cfsetispeed(&SerialPortSettings, baudrate); /* Set Read  Speed as 9600                       */
    cfsetospeed(&SerialPortSettings, baudrate); /* Set Write Speed as 9600                       */

    /* 8N1 Mode */
    SerialPortSettings.c_cflag &= ~PARENB; /* Disables the Parity Enable bit(PARENB),So No Parity   */
    SerialPortSettings.c_cflag &= ~CSTOPB; /* CSTOPB = 2 Stop bits,here it is cleared so 1 Stop bit */
    SerialPortSettings.c_cflag &= ~CSIZE;  /* Clears the mask for setting the data size             */
    SerialPortSettings.c_cflag |= CS8;     /* Set the data bits = 8                                 */

    SerialPortSettings.c_cflag &= ~CRTSCTS;       /* No Hardware flow Control                         */
    SerialPortSettings.c_cflag |= CREAD | CLOCAL; /* Enable receiver,Ignore Modem Control lines       */

    SerialPortSettings.c_iflag &= ~(IXON | IXOFF | IXANY);         /* Disable XON/XOFF flow control both i/p and o/p */
    SerialPortSettings.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG); /* Non Cannonical mode                            */

    SerialPortSettings.c_oflag &= ~OPOST; /*No Output Processing*/

    /* Setting Time outs */
    SerialPortSettings.c_cc[VMIN]  = 10; /* Read at least 10 characters */
    SerialPortSettings.c_cc[VTIME] = 1;  /* Wait indefinetly  unit:100ms */

    if ((tcsetattr(fd, TCSANOW, &SerialPortSettings)) != 0) /* Set the attributes to the termios structure*/
        printf("\n  ERROR ! in Setting attributes");
    else
        printf("\n  BaudRate = %d \n  StopBits = 1 \n  Parity   = none", baudrate);

    /*------------------------------- Read data from serial port -----------------------------*/
     int  err;
     pthread_t ntid;
     err = pthread_create(&ntid, NULL, test_write_thread, NULL);
     if  (err != 0)
         printf ( "can't create thread: %s\n" ,  strerror (err));


    tcflush(fd, TCIFLUSH); /* Discards old data in the rx buffer            */

    char read_buffer[32]; /* Buffer to store the data received              */
    int  bytes_read = 0;  /* Number of bytes read by the read() system call */
    int  i          = 0;
    while (1) {
        bytes_read = read(fd, read_buffer, 200); /* Read the data                   */

        printf("[MONITOR]  Bytes Rxed -%d \n", bytes_read); /* Print the number of bytes read */
        printf("[HEX]      ");
        for (i = 0; i < bytes_read; i++)         /*printing only the received characters*/
        {
            printf("%.2x ", read_buffer[i]);
        }
        printf("\n");
        read_buffer[bytes_read] = '\0';
        printf("[ASCII]   %s", read_buffer);
    }

    printf("\n +----------------------------------+\n\n\n");

    close(fd); /* Close the serial port */
    return 0;
}