////////////////////////////////////////////////////////////////////
// File Name    : cli.c                                           //
// Date         : 2017/05/18                                      //
// Os           : Ubuntu 12.04 LTS 64bits                         //
// Author       : Ko Jeong Won                                    //
// Student ID   : 2013722063                                      //
// ______________________________________________________________ //
// Title : System Programing FTP#2           (ftp - server )      //
// Description : Communicate Assignment2 using socket             //
////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>

#define MAX_BUFF 8192

//converting argument to ftp command
int conv_cmd(char* buff, char* cmd_buff);

//print processed result
void process_result(char* rcv_buff);

//exception handling for some signals
void sig_handler(int signum);

int sockfd;

int main(int argc, char** argv)
{
	pid_t cli_pid;
	//signal handling for SIGINT & SIGTERM & SIGUSR1
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGUSR1, sig_handler);
	//if arguments doesn't exist
	if(!argv[1] || !argv[2])
	{
		write(STDERR_FILENO, "Enter IP or PORT#!!\n", strlen("Enter IP or PORT#!!\n"));
		return;
	}
	char buff[MAX_BUFF], cmd_buff[MAX_BUFF], rcv_buff[MAX_BUFF];
	int n;	
	struct sockaddr_in c_addr;
	memset(buff, 0, sizeof(buff));

	//create socket
	if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		write(2, "socket() error!!\n", strlen("socket() error!!\n"));
		exit(1);
	}

	//initiaize sock_addr
	memset(&c_addr, 0, sizeof(c_addr));
	c_addr.sin_family = AF_INET;
	c_addr.sin_addr.s_addr = inet_addr(argv[1]);
	c_addr.sin_port = htons(atoi(argv[2]));

	//connect to server
	if(connect(sockfd, (struct sockaddr*)&c_addr, sizeof(c_addr)) < 0)
	{
		write(2, "connect() error!!\n", strlen("connect() error!!\n"));
		exit(1);
	}

	cli_pid = getpid();
	sprintf(buff, "%d", cli_pid);
	write(sockfd, buff, strlen(buff));

	//INF loop
	for(;;) 
	{

		//initialize char[]
		memset(buff, 0, sizeof(buff));
		memset(cmd_buff, 0, sizeof(cmd_buff));
		memset(rcv_buff, 0, sizeof(rcv_buff));

		write(1, ">> ", strlen(">> "));
		n = read(0, buff, MAX_BUFF);		//user insert buff
		buff[n-1] = '\0';					//check end of string

		//if wrong command
		if(conv_cmd(buff, cmd_buff) < 0) 
		{
			write(STDERR_FILENO, "conv_cmd() error\n", strlen("conv_cmd() error\n"));
			continue;
		}

		n = strlen(cmd_buff);

		//send cmd_buff to server
		if(write(sockfd, cmd_buff, MAX_BUFF) < 0)
		{
			write(STDERR_FILENO, "write() error!!\n", strlen("write() error!!\n"));
			continue;
		}
		//receive data from server
		if((n = read(sockfd, rcv_buff, MAX_BUFF)) > 0)
		{
			rcv_buff[n] = '\0';
			write(STDOUT_FILENO, rcv_buff, strlen(rcv_buff));
		}
		else
			break;
		
	}
	close(sockfd);
	return 0;

}

//if fail to convert than return -1 else return 1
int conv_cmd(char* buff, char* cmd_buff)
{	
	char tempbuff[MAX_BUFF];	
	char multinames[50][MAX_BUFF];
	char* option = NULL; char* arg = NULL; char* temp = NULL;                   
    DIR *dp;
    struct stat statt;                  //For file information
    int count = 0;
    int num = 0;

    if(strcmp(buff, "\0") == 0)
    	return -1;

	for(num ; num<50 ; num++)
		memset(multinames[num], 0, sizeof(multinames[num]));

	num = 0;
    memset(tempbuff, 0, sizeof(tempbuff));
    strcpy(tempbuff, buff);

    option = strtok(tempbuff, " ");

    if(strcmp(option, "ls") == 0) {                //if ls command
    	option = strtok(NULL, " ");
        if(option == NULL)                         //with no path
        {
            strcpy(cmd_buff,"NLST");      		   //send NLST  
            return 1; 
        }
        else if(strcmp(option, "-a") == 0) {       //if a option
        	arg = strtok(NULL, " ");
            if(arg == NULL) {                   //with no path
                strcpy(cmd_buff, "NLST ");
                strcat(cmd_buff, "-a");
                return 1;
            }
            else {                                  //a option with path

                lstat(arg, &statt);
                if(S_ISDIR(statt.st_mode) || S_ISREG(statt.st_mode)) {      //judge if exist
                    strcpy(cmd_buff, "NLST ");
                    strcat(cmd_buff, "-a ");
                    strcat(cmd_buff, arg);
                    return 1;
                }
                else 
                {
                	write(sockfd, buff, strlen(buff));
                    return -1;
                }
            }
        }
        else if(strcmp(option, "-l") == 0) {        //if l option
            arg = strtok(NULL, " ");
            if(arg == NULL) {                   //with no path
                strcpy(cmd_buff, "NLST ");
                strcat(cmd_buff, "-l");
                return 1;
            }
            else {

                lstat(arg, &statt);              //l option with path
                if(S_ISDIR(statt.st_mode) || S_ISREG(statt.st_mode)) {     //judge if exist
                    strcpy(cmd_buff, "NLST ");
                    strcat(cmd_buff, "-l ");
                    strcat(cmd_buff, arg);
                    return 1;
                }
                else 
                {
                	write(sockfd, buff, strlen(buff));
                    return -1;
                }
            }
        }
        else if(strcmp(option, "-al") == 0) {      //if al option
            arg = strtok(NULL, " ");
            if(arg == NULL) {                   //with no path
                strcpy(cmd_buff, "NLST ");
                strcat(cmd_buff, "-al");
                return 1;
            }
            else {

                lstat(arg, &statt);             //al option with path
                if(S_ISDIR(statt.st_mode) || S_ISREG(statt.st_mode)) {      //judge if exist
                    strcpy(cmd_buff, "NLST ");
                    strcat(cmd_buff, "-al ");
                    strcat(cmd_buff, arg);
                    return 1;
                }
                else 
                {
                	write(sockfd, buff, strlen(buff));
                    return -1;
                }
            }
        }
        else if(strcmp(option, "-la") == 0) {      //if al option
            arg = strtok(NULL, " ");
            if(arg == NULL) {                   //with no path
                strcpy(cmd_buff, "NLST ");
                strcat(cmd_buff, "-al");
                return 1;
            }
            else {

                lstat(arg, &statt);             //al option with path
                if(S_ISDIR(statt.st_mode) || S_ISREG(statt.st_mode)) {      //judge if exist
                    strcpy(cmd_buff, "NLST ");
                    strcat(cmd_buff, "-al ");
                    strcat(cmd_buff, arg);
                    return 1;
                }
                else 
                {
                    write(sockfd, buff, strlen(buff));
                    return -1;
                }
            }
        }
        else {                                      //no option & has path
            lstat(option, &statt);
                if(S_ISDIR(statt.st_mode) || S_ISREG(statt.st_mode)) {      //judge if exist
                    strcpy(cmd_buff, "NLST ");
                    strcat(cmd_buff, option);
                    return 1;
                }
                else
                {
                	write(sockfd, buff, strlen(buff));
                    return -1;
                }
        }
    }
    else if(strcmp(option, "dir") == 0) {          //if dir command

        arg = strtok(NULL, " ");
        if(arg == NULL) {                   //with no path
            strcpy(cmd_buff, "LIST");
            return 1;
        }
        else {
            lstat(arg, &statt);             //al option with path
            if(S_ISDIR(statt.st_mode)) {      //judge if exist
                strcpy(cmd_buff, "LIST ");
                strcat(cmd_buff, arg);
                return 1;
            }
            else 
            {
            	write(sockfd, buff, strlen(buff));
                return -1;
            }
        }
    }
    else if(strcmp(option, "pwd") == 0) {          //if pwd command
    	option = strtok(NULL, " ");
        if(option != NULL)                         //if argument exist
        {
        	write(sockfd, buff, strlen(buff));
            return -1;
        }
        else {
            strcpy(cmd_buff, "PWD");
            return 1;
        }
    }
    else if(strcmp(option, "cd") == 0) {           //if cd command
    	option = strtok(NULL, " ");
    	arg = strtok(NULL, " ");
        if(arg != NULL)                         //if there is no argument
        {
        	write(sockfd, buff, strlen(buff));
        	return -1;
        }
        else if(option == NULL)
        {
        	strcpy(cmd_buff,"CWD");
        	return 1;
        }
        else {                                      //if wrong path
            if((dp = opendir(option)) == NULL)
            {
            	write(sockfd, buff, strlen(buff));
                return -1;
            }
            else {
                lstat(option, &statt);             
                if(strcmp(option, "..") == 0)      //if argument is ".."
                {
                    strcpy(cmd_buff, "CDUP");
                	return 1;
                }
                else {                              //if argument is path
                    if(S_ISDIR(statt.st_mode)) {
                        strcpy(cmd_buff,"CWD ");
                        strcat(cmd_buff,option);
                        return 1;
                    } 
                    else                            //if wrong path
                    {
                    	write(sockfd, buff, strlen(buff));
                        return -1;
                    }
                }
            }
        }
    }
    else if(strcmp(option, "mkdir") == 0) {        //if mkdir command
    	option = strtok(NULL, " ");
        if(option == NULL)                         //if there are no arguments
        {
        	write(sockfd, buff, strlen(buff));
            return -1;
        }
        else {            
        	strcpy(multinames[num], option);
        	num++;
            strcpy(cmd_buff,"MKD ");
            while(1)
            {            	
            	temp = strtok(NULL, " ");
            	if(temp == NULL)
            		break;
            	strcpy(multinames[num], temp);
            	num++;
            }
            for(count ; count < num; count++) {
                                  
                strcat(cmd_buff,multinames[count]);   
                strcat(cmd_buff, " ");
            }            
            return 1;          
        }
    }
    else if(strcmp(option, "delete") == 0) {       //if delete command
        option = strtok(NULL, " ");
        if(option == NULL)                         //if there are no arguments
        {
        	write(sockfd, buff, strlen(buff));
            return -1;
        }
        else {            
        	strcpy(multinames[num], option);
        	num++;
            strcpy(cmd_buff,"DELE ");
            while(1)
            {            	
            	temp = strtok(NULL, " ");
            	if(temp == NULL)
            		break;
            	strcpy(multinames[num], temp);
            	num++;
            }
            for(count ; count < num; count++) {
                lstat(multinames[count], &statt);
                if(S_ISDIR(statt.st_mode))         //if new name already exist
                {
                	write(sockfd, buff, strlen(buff));
                    return -1;                
                }
                else {                    
                    strcat(cmd_buff,multinames[count]);   
                    strcat(cmd_buff, " ");
                }
            }            
            return 1;          
        }
    }
    else if(strcmp(option, "rmdir") == 0) {            //if rmdir command
        option = strtok(NULL, " ");
        if(option == NULL)                         //if there are no arguments
        {
        	write(sockfd, buff, strlen(buff));
            return -1;
        }
        else {            
        	strcpy(multinames[num], option);
        	num++;
            strcpy(cmd_buff,"RMD ");
            while(1)
            {            	
            	temp = strtok(NULL, " ");
            	if(temp == NULL)
            		break;
            	strcpy(multinames[num], temp);
            	num++;
            }
            for(count ; count < num; count++) {
                               
                strcat(cmd_buff,multinames[count]);   
                strcat(cmd_buff, " ");
                
            }            
            return 1;          
        }
    }    
    else if(strcmp(option, "rename") == 0) {           //if rename command
    	option = strtok(NULL, " ");
    	arg = strtok(NULL, " ");
    	temp = strtok(NULL, " ");
        if(temp != NULL || arg == NULL || option == NULL)       //wrong arguments
        {
        	write(sockfd, buff, strlen(buff));
            return -1;
        }
        else {
            lstat(option, &statt);
            if(S_ISDIR(statt.st_mode) || S_ISREG(statt.st_mode)) {                //if directory or file
                if(lstat(arg, &statt) < 0) {
                    strcpy(cmd_buff, "RNFR ");
                    strcat(cmd_buff, option);
                    strcat(cmd_buff, " ");
                    strcat(cmd_buff, arg);
                    return 1;
                }
                else {                                  //if new name already exist
                	write(sockfd, buff, strlen(buff));
                    return -1;
                }
            }
            else 
            {
            	write(sockfd, buff, strlen(buff));
                return -1;
            }
        }
    }
    else if(strcmp(option, "quit") == 0) {         //if quit command
    	option = strtok(NULL, " ");
        if(option == NULL) {
            write(sockfd, "QUIT", strlen("QUIT"));
        }
        else {                                      //if argument exist
        	write(sockfd, buff, strlen(buff));
            return -1;
        }
    }
    else {                                          //if unknown command
    	write(sockfd, buff, strlen(buff));
        return -1;
    }
}


//print process
void process_result(char* rcv_buff)
{
	char* temp;
	temp = strtok(rcv_buff, " ");
	write(1, temp, strlen(temp));
	temp = strtok(NULL, " ");
	for(; ; )
	{
		if(!temp)
			return;
		write(1, temp, strlen(temp));
		temp = strtok(NULL, " ");
	}
}


//signal handling
void sig_handler(int signum)
{
	if(signum == SIGINT)       //For SIGINT signal
	{
		write(sockfd, "QUIT", strlen("QUIT"));
		close(sockfd);
		exit(0);
	}
	else if(signum == SIGTERM)     //For SIGTERM siganl
	{
		close(sockfd);
		exit(0);
	}
	else if(signum == SIGUSR1)     //For SIGUSR1 signal
	{
		write(STDOUT_FILENO, "\n", sizeof("\n"));
		close(sockfd);
		exit(0);
	}
}

