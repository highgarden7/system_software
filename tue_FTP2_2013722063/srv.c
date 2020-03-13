////////////////////////////////////////////////////////////////////
// File Name    : srv.c                                           //
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
#include <time.h>
#include <pwd.h>
#include <grp.h>

//functions for ls
void strlwr(char* str);                     				   //change to lower case alphabet
void changestr(char* str1, char* str2);     				   //swap str1 & str2
void showlist(char* path, char* result_buff);                  //excute ls command
void showlist_a(char* path, char* result_buff);                //excute ls -a command
void showlist_l(char* path, char* result_buff);                //excute ls -l command
void showlist_al(char* path, char* result_buff);               //excute ls -al command
void dir_path(char* path, char* result_buff);                  //excute dir command

//process correct command
int cmd_process(char* buf, char* result_buff);

//print client's infomation
int client_info(struct sockaddr_in* c_addr);

//exception handling for some signals
void sh_alrm(int);
void sh_term(int);
void sh_chld(int);
void sh_int(int);
void sh_usr1(int);



//Using Linked-list
typedef struct Node
{
	pid_t process_ID;
	pid_t cli_pid;
	int port;
	int time;
	int process_count;
	struct Node* p_next;
}Node;

//Linked-list functions
void insert_node(struct sockaddr_in* c_addr, pid_t cur_pid, pid_t cli_pid);
void delete_node(pid_t pid);
void print_node();


//Global variables
Node* p_head;
int server_fd, client_fd;
pid_t parent_pid;

//set MACRO
#define MAX_BUFF 8192   

int main(int argc, char** argv)
{
	p_head = NULL; //set p_head to NULL

	char buff[MAX_BUFF], result_buff[MAX_BUFF];
	int n;
	struct sockaddr_in server_addr, client_addr;
	
	int len;
	int port;

	//create socket
	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		write(STDERR_FILENO, "socket() error!!\n", sizeof("socket() error!!\n"));
		return 0;
	}

	//initialize server's address
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = PF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(atoi(argv[1]));


	//bind socket & address
	if(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
	{
		write(STDERR_FILENO, "bind() error!!\n", sizeof("bind() error!!\n"));
		return 0;;
	}

	//wait 5 connect of clients
	listen(server_fd, 5);

	//INF loop
	while(1)
	{
		//signal actions
        signal(SIGINT, sh_int);
		signal(SIGALRM, sh_alrm);
		signal(SIGCHLD, sh_chld);
		signal(SIGTERM, sh_term);
        signal(SIGUSR1, sh_usr1);

		pid_t pid;
		int temp;
		len = sizeof(client_addr);

		//accept client
		if((client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len)) < 0) 
		{
			write(STDERR_FILENO, "accept() error!!\n", sizeof("accept() error!!\n"));
			return 0;
		}

		memset(buff, 0, sizeof(buff));
		read(client_fd, buff, MAX_BUFF);

		parent_pid = getpid();

		//make child process
		if((pid = fork()) > 0)				//if parant process 
		{
            
			client_info(&client_addr);
			char  temp[100];
			memset(temp, 0, sizeof(temp));
			sprintf(temp, "Child Process ID : %d\n", pid);		
			write(STDOUT_FILENO, temp, strlen(temp));			//print PID
			insert_node(&client_addr, pid, atoi(buff));         //make node
			print_node();			                            //print list
		}
		else if(pid == 0)					//if child process
		{		
			close(server_fd);
			//INF loop
			while(1)
			{				
				memset(buff, 0, sizeof(buff));
				memset(result_buff, 0, sizeof(result_buff));
				//receive data from client
				read(client_fd, buff, MAX_BUFF);
				
				//if data is QUIT
				if(strcmp(buff, "QUIT") == 0)
				{			
					char  temp[100];
					memset(temp, 0, sizeof(temp));
					sprintf(temp, "QUIT : [%d]\n", getpid());		
					write(STDOUT_FILENO, temp, strlen(temp));
					kill(getpid(), SIGTERM);
				}
				write(STDOUT_FILENO, "> ", strlen("> "));

				if(cmd_process(buff, result_buff) < 0)
					continue;

				//send data to client
				write(client_fd, result_buff, strlen(result_buff));
			}
		}
		close(client_fd);
	}
	return 0;
}

//if fail to processing than return -1 else return 1
int cmd_process(char* buf, char* result_buff)
{
	char temp[MAX_BUFF];                         //for copy data
    memset(temp, 0, sizeof(temp));
    strcpy(temp, buf);
    char printpid[10];
    sprintf(printpid, " [%d]\n", getpid());
    char tempstring[100];
    struct passwd* psd;

    //for tokenize
    char* arg1; char* arg2; char* arg3; char* arg4;
    arg1 = strtok(buf, " ");

    if(strcmp(arg1,"NLST") == 0) {          //if NLST command
        arg2 = strtok(NULL, " ");           
        if(arg2 == NULL) {                  //there is no path
            strcat(temp, printpid);
            write(STDOUT_FILENO, temp, strlen(temp));
            showlist(".", result_buff);
            return 1;
        }
        else if(strcmp(arg2, "-a") == 0) {      //if -a option
            arg3 = strtok(NULL, " ");
            if(arg3 == NULL) {                  //with no path
                strcat(temp, printpid);
           		write(STDOUT_FILENO, temp, strlen(temp));
                showlist_a(".", result_buff);
                return 1;
            }
            else {                              //with path
                strcat(temp, printpid);
            	write(STDOUT_FILENO, temp, strlen(temp));
                showlist_a(arg3, result_buff);
                return 1;
            }
        }
        else if(strcmp(arg2, "-l") == 0) {      //if -l option
            arg3 = strtok(NULL, " ");
            if(arg3 == NULL) {                  //with no path
                strcat(temp, printpid);
            	write(STDOUT_FILENO, temp, strlen(temp));
                showlist_l(".", result_buff);
                return 1;
            }
            else {                              //with path
                strcat(temp, printpid);
            	write(STDOUT_FILENO, temp, strlen(temp));
                showlist_l(arg3, result_buff);
                return 1;
            }
        }
        else if(strcmp(arg2, "-al") == 0) {     //if -al option
            arg3 = strtok(NULL, " ");
            if(arg3 == NULL) {                  //with no path
                strcat(temp, printpid);
            	write(STDOUT_FILENO, temp, strlen(temp));
                showlist_al(".", result_buff);
                return 1;
            }
            else {                              //with path
                strcat(temp, printpid);
            	write(STDOUT_FILENO, temp, strlen(temp));
                showlist_al(arg3, result_buff);
                return 1;
            }
        }
        else {                                  //no option & has path
            strcat(temp, printpid);
            write(STDOUT_FILENO, temp, strlen(temp));
            showlist(arg2, result_buff);
            return 1;
        }
    }
    else if(strcmp(arg1,"LIST") == 0) {         //if LIST command
        arg3 = strtok(NULL, " ");
        if(arg3 == NULL) {                  //with no path
                strcat(temp, printpid);
            	write(STDOUT_FILENO, temp, strlen(temp));
                dir_path(".", result_buff);
                return 1;
            }
        else
        {
        	strcat(temp, printpid);
        	write(STDOUT_FILENO, temp, strlen(temp));
            dir_path(arg3, result_buff);
            return 1;
        }
    }
    else if(strcmp(arg1,"PWD") == 0) {          //if PWD command
        char path[200];
        memset(path, 0, sizeof(path));
        getcwd(path, 200);
        strcat(path, "\n");

        strcat(arg1, printpid);
        write(STDOUT_FILENO, arg1, strlen(arg1));
        strcpy(result_buff, path);
        return 1;
    }
    else if(strcmp(arg1,"CWD") == 0) {          //if CWD command

        arg2 = strtok(NULL, " ");

        if(arg2 == NULL)
        {
        	char path[200];
        	psd = getpwuid(getuid());
        	sprintf(path, "/home/%s", psd->pw_name);
        	chdir(path);

        	strcpy(result_buff, "NOW : ");
        	strcat(result_buff, path);
        }
        strcat(temp, printpid);
        write(STDOUT_FILENO, temp, strlen(temp));  

        chdir(arg2);

        char path[200];
        memset(path, 0, sizeof(path));
        getcwd(path, 200);
        strcat(path, "\n");

        strcpy(result_buff, "NOW : ");
        strcat(result_buff, path);

        return 1;
    }
    else if(strcmp(arg1,"CDUP") == 0) {         //if CDUP command

    	chdir("..");	//change directory to parent

        strcat(arg1, printpid);
        write(STDOUT_FILENO, arg1, strlen(arg1));

        char path[200];
        memset(path, 0, sizeof(path));
        getcwd(path, 200);
        strcat(path, "\n");

        strcpy(result_buff, "NOW : ");
        strcat(result_buff, path);

        return 1;
    }
    else if(strcmp(arg1,"MKD") == 0) {          //if MKD command
    	arg2 = strtok(NULL, " ");
        strcat(temp, printpid);
        write(STDOUT_FILENO, temp, strlen(temp));

        if(mkdir(arg2, 0777) < 0)                      //permission is -rwxrwx-w-
        {
        	memset(tempstring, 0, sizeof(tempstring));
        	sprintf(tempstring,"%s is already exist!\n", arg2);
        	strcat(result_buff, tempstring);
        }

        for(arg2 ; arg2 = strtok(NULL, " "); ) 
        {
            if(arg2 == NULL)
                break;
            if(mkdir(arg2, 0777) < 0)
            {
            	memset(tempstring, 0, sizeof(tempstring));
	        	sprintf(tempstring,"%s is already exist!\n", arg2);
	        	strcat(result_buff, tempstring);
            }
        }
        strcat(result_buff, "Make Success\n");
        return 1;
    }
    else if(strcmp(arg1,"DELE") == 0) {         //if DELE command
    	arg2 = strtok(NULL, " ");
        strcat(temp, printpid);
        write(STDOUT_FILENO, temp, sizeof(temp));

        if(unlink(arg2) < 0)
        {
        	memset(tempstring, 0, sizeof(tempstring));
        	sprintf(tempstring,"%s is not exist!\n", arg2);
        	strcat(result_buff, tempstring);
        }

        for(arg2 ; arg2 = strtok(NULL, " "); ) {
            if(arg2 == NULL)
                break;
            if(unlink(arg2) < 0)
            {
            	memset(tempstring, 0, sizeof(tempstring));
            	sprintf(tempstring,"%s is not exist!\n", arg2);
            	strcat(result_buff, tempstring);
            }
        }
        strcat(result_buff, "Files removed!\n");
        return 1;
    }
    else if(strcmp(arg1,"RMD") == 0) {          //if RMD command
    	arg2 = strtok(NULL, " ");    	

        strcat(temp, printpid);
        write(STDOUT_FILENO, temp, strlen(temp));

        if(rmdir(arg2) < 0)
        {
        	memset(tempstring, 0, sizeof(tempstring));
        	sprintf(tempstring,"%s is not empty!\n", arg2);
        	strcat(result_buff, tempstring);
        }
        
        for(arg2 ; arg2 = strtok(NULL, " "); ) 
        {
            if(arg2 == NULL)
                break;
            if(rmdir(arg2) < 0)
            {
            	memset(tempstring, 0, sizeof(tempstring));
            	sprintf(tempstring,"%s is not empty!\n", arg2);
            	strcat(result_buff, tempstring);
            }
        }
        strcat(result_buff, "DIRs removed!\n");
        return 1;
                
    }
    else if(strcmp(arg1,"RNFR") == 0)               //if RNFR command
    {         

        strcat(temp, printpid);
        write(STDOUT_FILENO, temp, strlen(temp));

    	arg2 = strtok(NULL, " ");
        arg3 = strtok(NULL, " ");

        rename(arg2, arg3);
        strcpy(result_buff, "File renamed!\n");
        return 1;
    }
    else 
    {
        char error_buff[200];
        memset(error_buff, 0, sizeof(error_buff));
        sprintf(error_buff, "%s [ERROR!]  %s", temp, printpid);
    	write(STDERR_FILENO, error_buff, strlen(error_buff));
    	return -1;
    }
}

//This function excute like ls system call
void showlist(char* path, char* result_buff) 
{

    //for get information of directory
    DIR *dp;                    
    struct dirent *dirp;        

    int count = 0;
    int count2 = 0;
    int check = 0;              //for checking the number of inode's count       
    char sort[100][50];         //for sorting name

    for(count ; count < 100 ; count ++) 
        memset(sort[count], 0, sizeof(sort[count]));    
    count = 0;

    struct stat statt;          //for get information of file system

    if(strcmp(path, ".") == 0) {        //if current directory
        dp = opendir(".");     //open directory

        while(dirp = readdir(dp)) {     //read directory
            char path[100];
            memset(path, 0, sizeof(path));
            getcwd(path, 100);
            strcat(path, "/");
            if(dirp->d_name[0] == '.')      //if hidden file
                continue;            
            else {
                strcpy(sort[check], dirp->d_name);
                lstat(strcat(path,sort[check]), &statt);
                if(S_ISDIR(statt.st_mode))          //if directory
                    strcat(sort[check], "/");
                check++;
            }
        }
    }
    else {                          //if there is path

        lstat(path, &statt);

        if(S_ISREG(statt.st_mode)) {        //if path is file
        	strcat(path, "\n");
            strcpy(result_buff, path);
            return;
        }

        dp = opendir(path);                 //open directory

        while(dirp = readdir(dp)) {
            char temp[100];
            memset(temp, 0, sizeof(temp));
            strcpy(temp, path);
            strcat(temp, "/");
            if(dirp->d_name[0] == '.')      //if hidden file
                continue;            
            else {
                strcpy(sort[check], dirp->d_name);
                lstat(strcat(temp,sort[check]), &statt);
                if(S_ISDIR(statt.st_mode))          //if directory
                    strcat(sort[check], "/");
                check++;
            }
        }
    }

    

    //Sorting process using bubble sort
    for(count = 0 ; count < check - 1 ; count++ ) {
        for(count2 = 0 ; count2 < check - count - 1 ; count2 ++) {
            char temp1[50];
            char temp2[50];
            strcpy(temp1, sort[count2]);
            strcpy(temp2, sort[count2 + 1]);
            strlwr(temp1);
            strlwr(temp2);
            if(strcmp(temp1, temp2) > 0) 
                changestr(sort[count2], sort[count2 + 1]);            
        }
    }

    count = 0;
    count2 = 0;

    //print sorted files
    for(; count2 < check; ) {
        if(count == 4) {
            strcat(sort[count2], "\n");
            strcat(result_buff, sort[count2]);
            count = 0;
            count2++;
        }
        else {
            strcat(sort[count2], "\t\t");
            strcat(result_buff, sort[count2]);
            count++;
            count2++;
        }
    }
    if(strcmp(result_buff, "") == 0)
    	strcpy(result_buff, "\n");
    if(count != 0)
    	strcat(result_buff, "\n");
    closedir(dp);
    
}

//This function excute like ls -a system call
void showlist_a(char* path, char* result_buff) 
{
    //for get information of directory
    DIR *dp;                    
    struct dirent *dirp;        

    int count = 0;
    int count2 = 0;
    int check = 0;              //for checking the number of inode's count       
    char sort[100][50];         //for sorting name

    for(count ; count < 100 ; count ++) 
        memset(sort[count], 0, sizeof(sort[count]));    
    count = 0;

    struct stat statt;          //for get information of file system

    if(strcmp(path, ".") == 0) {        //if current directory
        dp = opendir(".");     //open directory

        while(dirp = readdir(dp)) {     //read directory
            char path[100];
            memset(path, 0, sizeof(path));
            getcwd(path, 100);
            strcat(path, "/");
            strcpy(sort[check], dirp->d_name);
            lstat(strcat(path,sort[check]), &statt);
            if(S_ISDIR(statt.st_mode))          //if directory
                strcat(sort[check], "/");
            check++;
        
        }
    }
    else {                          //if there is path

        lstat(path, &statt);

        if(S_ISREG(statt.st_mode)) {        //if path is file
        	strcat(path, "\n");
            strcpy(result_buff, path);
            return;
        }

        dp = opendir(path);                 //open directory

        while(dirp = readdir(dp)) {
            char temp[100];
            memset(temp, 0, sizeof(temp));
            strcpy(temp, path);
            strcat(temp, "/");
            strcpy(sort[check], dirp->d_name);
            lstat(strcat(temp,sort[check]), &statt);
            if(S_ISDIR(statt.st_mode))          //if directory
                strcat(sort[check], "/");
            check++;
            
        }
    }    

    //Sorting process using bubble sort
    for(count = 0 ; count < check - 1 ; count++ ) {
        for(count2 = 0 ; count2 < check - count - 1 ; count2 ++) {
            char temp1[50];
            char temp2[50];
            strcpy(temp1, sort[count2]);
            strcpy(temp2, sort[count2 + 1]);
            strlwr(temp1);
            strlwr(temp2);
            if(strcmp(temp1, temp2) > 0) 
                changestr(sort[count2], sort[count2 + 1]);            
        }
    }

    count = 0;
    count2 = 0;

    //print sorted files
    for(; count2 < check; ) {
        if(count == 4) {
            strcat(sort[count2], "\n");
            strcat(result_buff, sort[count2]);
            count = 0;
            count2++;
        }
        else {
            strcat(sort[count2], "\t\t");
            strcat(result_buff, sort[count2]);
            count++;
            count2++;
        }
    }
    if(strcmp(result_buff, "") == 0)
    	strcpy(result_buff, "\n");
    if(count != 0)
    	strcat(result_buff, "\n");
    closedir(dp);
}


//This function excute like ls -l system call
void showlist_l(char* path, char* result_buff) 
{

    //for get information of directory
    DIR *dp;
    struct dirent *dirp;

    int count = 0;
    int count2 = 0;
    int check = 0;              //for checking the number of inodes
    char sort[100][50];         //for sorting
    char cpath[100];
    char* tok;                  //for tokenize
    char data[50];
    struct passwd* pd;          //for get information of user name
    struct group* grp;          //for get information of group name
    struct tm* ttime;           //for get information of time
    char permission[11];        //for print permisstion
    char time[50];
    char buf[100]; 

    memset(cpath, 0, sizeof(cpath));

    for(count ; count < 100 ; count ++) 
        memset(sort[count], 0, sizeof(sort[count]));    
    count = 0;

    struct stat statt;          //for get information of path

    if(strcmp(path, ".") == 0) {            //if current directory
        dp = opendir(".");

        while(dirp = readdir(dp)) {
            memset(cpath, 0, sizeof(cpath));
            getcwd(cpath, 100);
            strcat(cpath, "/");
            if(dirp->d_name[0] == '.')      //if hidden file
                continue;            
            else {
                strcpy(sort[check], dirp->d_name);
                lstat(strcat(cpath,sort[check]), &statt);
                if(S_ISDIR(statt.st_mode))          //if directory
                    strcat(sort[check], "/");
                check++;
            }            
        }
    }
    else {                                      //if has path

        lstat(path, &statt);
        
        if(S_ISREG(statt.st_mode)) {            //if path is file

            tok = strtok(path, "/");
            memset(data, 0, sizeof(data));
            memset(buf, 0, sizeof(buf));
            strcpy(data, tok);
            for(tok ; tok = strtok(NULL, "/") ; ) {
                memset(data, 0, sizeof(data));
                strcpy(data, tok);
            }


            //initialize permisstion process

            permission[0] = '-';

            if((statt.st_mode) & S_IRUSR) permission[1] = 'r'; else permission[1] = '-';
            if((statt.st_mode) & S_IWUSR) permission[2] = 'w'; else permission[2] = '-';
            if((statt.st_mode) & S_IXUSR) permission[3] = 'x'; else permission[3] = '-';
            if((statt.st_mode) & S_IRGRP) permission[4] = 'r'; else permission[4] = '-';
            if((statt.st_mode) & S_IWGRP) permission[5] = 'w'; else permission[5] = '-';
            if((statt.st_mode) & S_IXGRP) permission[6] = 'x'; else permission[6] = '-';
            if((statt.st_mode) & S_IROTH) permission[7] = 'r'; else permission[7] = '-';
            if((statt.st_mode) & S_IWOTH) permission[8] = 'w'; else permission[8] = '-';
            if((statt.st_mode) & S_IXOTH) permission[9] = 'x'; else permission[9] = '-';
        
            //Initialize end 

            pd = getpwuid(statt.st_uid);    //get user name
            grp = getgrgid(statt.st_gid);   //get group name

            ttime = localtime(&statt.st_mtime);        //get time information
            strftime(time, 50, "%b %d %R ", ttime);     //format time for print
      
            //formating buf for print
            sprintf(buf, "%s %3d %6s %6s %9d %s %s\n", 
                        permission, statt.st_nlink, pd->pw_name, grp->gr_name, statt.st_size, time, data);

            strcpy(result_buff, buf);

            if(strcmp(result_buff, "") == 0)
    			strcpy(result_buff, "\n");

    		closedir(dp);
            return;
        }

        dp = opendir(path); 

        while(dirp = readdir(dp)) {
            memset(cpath, 0, sizeof(cpath));
            strcpy(cpath, path);
            strcat(cpath, "/");            
            if(dirp->d_name[0] == '.')          //if hidden file
                continue;            
            else {
                strcpy(sort[check], dirp->d_name);                
                lstat(strcat(cpath,sort[check]), &statt);
                if(S_ISDIR(statt.st_mode))          //if directory
                    strcat(sort[check], "/");
                check++;
            }            
        }
    }

       
    //Bubble sorting process

    for(count = 0 ; count < check - 1 ; count++ ) {
        for(count2 = 0 ; count2 < check - count - 1 ; count2 ++) {
            char temp1[50];
            char temp2[50];
            memset(temp1, 0, sizeof(temp1));
            memset(temp2, 0, sizeof(temp2));

            strcpy(temp1, sort[count2]);
            strcpy(temp2, sort[count2 + 1]);
            strlwr(temp1);
            strlwr(temp2);
            if(strcmp(temp1, temp2) > 0) {                
                changestr(sort[count2], sort[count2 + 1]);            
            }
        }
    }

    //Bubble sorting end

    count = 0;
    count2 = 0;

    
    if(strcmp(path, ".") == 0) {                //if current directory
        for(count = 0 ; count < check ; count++) {
            memset(permission, 0, sizeof(permission));
            memset(cpath, 0, sizeof(cpath));
            memset(time, 0, sizeof(time));
            memset(buf, 0, sizeof(buf));
            getcwd(cpath, 100);
            strcat(cpath, "/");
            strcat(cpath, sort[count]);


            //initialize file type
            lstat(cpath, &statt);
            if(S_ISDIR(statt.st_mode))
               permission[0] = 'd';
            else if(S_ISREG(statt.st_mode))
                permission[0] = '-';
            else if(S_ISCHR(statt.st_mode))
                permission[0] = 'c';
            else if(S_ISBLK(statt.st_mode))
                permission[0] = 'b';
            else if(S_ISFIFO(statt.st_mode))
                permission[0] = 'f';
            else if(S_ISLNK(statt.st_mode))
                permission[0] = 'l';
            else
                permission[0] = 's';

            //initialize permission process
            if((statt.st_mode) & S_IRUSR) permission[1] = 'r'; else permission[1] = '-';
            if((statt.st_mode) & S_IWUSR) permission[2] = 'w'; else permission[2] = '-';
            if((statt.st_mode) & S_IXUSR) permission[3] = 'x'; else permission[3] = '-';
            if((statt.st_mode) & S_IRGRP) permission[4] = 'r'; else permission[4] = '-';
            if((statt.st_mode) & S_IWGRP) permission[5] = 'w'; else permission[5] = '-';
            if((statt.st_mode) & S_IXGRP) permission[6] = 'x'; else permission[6] = '-';
            if((statt.st_mode) & S_IROTH) permission[7] = 'r'; else permission[7] = '-';
            if((statt.st_mode) & S_IWOTH) permission[8] = 'w'; else permission[8] = '-';
            if((statt.st_mode) & S_IXOTH) permission[9] = 'x'; else permission[9] = '-';

            //initialize end
        
            pd = getpwuid(statt.st_uid);        //get user name
            grp = getgrgid(statt.st_gid);       //get group name

            ttime = localtime(&statt.st_mtime);         //get time information
            strftime(time, 50, "%b %d %R ", ttime);     //format time information for print
      
            //format buf for print
            sprintf(buf, "%s %3d %6s %6s %9d %s %s\n", 
                        permission, statt.st_nlink, pd->pw_name, grp->gr_name, statt.st_size, time, sort[count]);

            strcat(result_buff, buf);
        }
    }
    else {                                      //if has path
        for(count = 0 ; count < check ; count++) {
            memset(permission, 0, sizeof(permission));
            memset(cpath, 0, sizeof(cpath));
            memset(time, 0, sizeof(time));
            memset(buf, 0, sizeof(buf));
            strcpy(cpath, path);
            strcat(cpath, "/");
            strcat(cpath, sort[count]);


            //initialize file type
            lstat(cpath, &statt);
            if(S_ISDIR(statt.st_mode))
               permission[0] = 'd';
            else if(S_ISREG(statt.st_mode))
                permission[0] = '-';
            else if(S_ISCHR(statt.st_mode))
                permission[0] = 'c';
            else if(S_ISBLK(statt.st_mode))
                permission[0] = 'b';
            else if(S_ISFIFO(statt.st_mode))
                permission[0] = 'f';
            else if(S_ISLNK(statt.st_mode))
                permission[0] = 'l';
            else
                permission[0] = 's';

            //initialize permisstion process
            if((statt.st_mode) & S_IRUSR) permission[1] = 'r'; else permission[1] = '-';
            if((statt.st_mode) & S_IWUSR) permission[2] = 'w'; else permission[2] = '-';
            if((statt.st_mode) & S_IXUSR) permission[3] = 'x'; else permission[3] = '-';
            if((statt.st_mode) & S_IRGRP) permission[4] = 'r'; else permission[4] = '-';
            if((statt.st_mode) & S_IWGRP) permission[5] = 'w'; else permission[5] = '-';
            if((statt.st_mode) & S_IXGRP) permission[6] = 'x'; else permission[6] = '-';
            if((statt.st_mode) & S_IROTH) permission[7] = 'r'; else permission[7] = '-';
            if((statt.st_mode) & S_IWOTH) permission[8] = 'w'; else permission[8] = '-';
            if((statt.st_mode) & S_IXOTH) permission[9] = 'x'; else permission[9] = '-';
            
            //initialize end

            pd = getpwuid(statt.st_uid);        //get user name
            grp = getgrgid(statt.st_gid);       //get group name

            ttime = localtime(&statt.st_mtime);        //get time information
            strftime(time, 50, "%b %d %R ", ttime);     //format time information for print
      
            //format buf for print
            sprintf(buf, "%s %3d %6s %6s %9d %s %s\n", 
                        permission, statt.st_nlink, pd->pw_name, grp->gr_name, statt.st_size, time, sort[count]);

            strcat(result_buff, buf);
        }
    }

    if(strcmp(result_buff, "") == 0)
    	strcpy(result_buff, "\n");

    closedir(dp);       //close directory

}


//This function excute like ls -al system call
void showlist_al(char* path, char* result_buff) 
{
    //for get information of directory
    DIR *dp;
    struct dirent *dirp;

    int count = 0;
    int count2 = 0;
    int check = 0;              //for checking the number of inodes
    char sort[100][50];         //for sorting
    char cpath[100];
    char* tok;                  //for tokenize
    char data[50];
    struct passwd* pd;          //for get information of user name
    struct group* grp;          //for get information of group name
    struct tm* ttime;           //for get information of time
    char permission[11];        //for print permisstion
    char time[50];
    char buf[100]; 

    memset(cpath, 0, sizeof(cpath));

    for(count ; count < 100 ; count ++) 
        memset(sort[count], 0, sizeof(sort[count]));    
    count = 0;

    struct stat statt;          //for get information of path

    if(strcmp(path, ".") == 0) {            //if current directory
        dp = opendir(".");

        while(dirp = readdir(dp)) {
            memset(cpath, 0, sizeof(cpath));
            getcwd(cpath, 100);
            strcat(cpath, "/");
            strcpy(sort[check], dirp->d_name);
            lstat(strcat(cpath,sort[check]), &statt);
            if(S_ISDIR(statt.st_mode))          //if directory
                strcat(sort[check], "/");
            check++;
                     
        }
    }
    else {                                      //if has path

        lstat(path, &statt);
        
        if(S_ISREG(statt.st_mode)) {            //if path is file

            tok = strtok(path, "/");
            memset(data, 0, sizeof(data));
            memset(buf, 0, sizeof(buf));
            strcpy(data, tok);
            for(tok ; tok = strtok(NULL, "/") ; ) {
                memset(data, 0, sizeof(data));
                strcpy(data, tok);
            }


            //initialize permisstion process

            permission[0] = '-';
            if((statt.st_mode) & S_IRUSR) permission[1] = 'r'; else permission[1] = '-';
            if((statt.st_mode) & S_IWUSR) permission[2] = 'w'; else permission[2] = '-';
            if((statt.st_mode) & S_IXUSR) permission[3] = 'x'; else permission[3] = '-';
            if((statt.st_mode) & S_IRGRP) permission[4] = 'r'; else permission[4] = '-';
            if((statt.st_mode) & S_IWGRP) permission[5] = 'w'; else permission[5] = '-';
            if((statt.st_mode) & S_IXGRP) permission[6] = 'x'; else permission[6] = '-';
            if((statt.st_mode) & S_IROTH) permission[7] = 'r'; else permission[7] = '-';
            if((statt.st_mode) & S_IWOTH) permission[8] = 'w'; else permission[8] = '-';
            if((statt.st_mode) & S_IXOTH) permission[9] = 'x'; else permission[9] = '-';
        
            //Initialize end 

            pd = getpwuid(statt.st_uid);    //get user name
            grp = getgrgid(statt.st_gid);   //get group name

            ttime = localtime(&statt.st_mtime);        //get time information
            strftime(time, 50, "%b %d %R ", ttime);     //format time for print
      
            //formating buf for print
            sprintf(buf, "%s %3d %6s %6s %9d %s %s\n", 
                        permission, statt.st_nlink, pd->pw_name, grp->gr_name, statt.st_size, time, data);

            strcpy(result_buff, buf);

            if(strcmp(result_buff, "") == 0)
    			strcpy(result_buff, "\n");

    		closedir(dp);
            return;
        }

        dp = opendir(path); 

        while(dirp = readdir(dp)) {
            memset(cpath, 0, sizeof(cpath));
            strcpy(cpath, path);
            strcat(cpath, "/"); 
            strcpy(sort[check], dirp->d_name);                
            lstat(strcat(cpath,sort[check]), &statt);
            if(S_ISDIR(statt.st_mode))          //if directory
                strcat(sort[check], "/");
            check++;
                    
        }
    }

       
    //Bubble sorting process

    for(count = 0 ; count < check - 1 ; count++ ) {
        for(count2 = 0 ; count2 < check - count - 1 ; count2 ++) {
            char temp1[50];
            char temp2[50];
            memset(temp1, 0, sizeof(temp1));
            memset(temp2, 0, sizeof(temp2));

            strcpy(temp1, sort[count2]);
            strcpy(temp2, sort[count2 + 1]);
            strlwr(temp1);
            strlwr(temp2);
            if(strcmp(temp1, temp2) > 0) {                
                changestr(sort[count2], sort[count2 + 1]);            
            }
        }
    }

    //Bubble sorting end

    count = 0;
    count2 = 0;

    
    if(strcmp(path, ".") == 0) {                //if current directory
        for(count = 0 ; count < check ; count++) {
            memset(permission, 0, sizeof(permission));
            memset(cpath, 0, sizeof(cpath));
            memset(time, 0, sizeof(time));
            memset(buf, 0, sizeof(buf));
            getcwd(cpath, 100);
            strcat(cpath, "/");
            strcat(cpath, sort[count]);


            //initialize file type
            lstat(cpath, &statt);
            if(S_ISDIR(statt.st_mode))
               permission[0] = 'd';
            else if(S_ISREG(statt.st_mode))
                permission[0] = '-';
            else if(S_ISCHR(statt.st_mode))
                permission[0] = 'c';
            else if(S_ISBLK(statt.st_mode))
                permission[0] = 'b';
            else if(S_ISFIFO(statt.st_mode))
                permission[0] = 'f';
            else if(S_ISLNK(statt.st_mode))
                permission[0] = 'l';
            else
                permission[0] = 's';

            //initialize permission process
            if((statt.st_mode) & S_IRUSR) permission[1] = 'r'; else permission[1] = '-';
            if((statt.st_mode) & S_IWUSR) permission[2] = 'w'; else permission[2] = '-';
            if((statt.st_mode) & S_IXUSR) permission[3] = 'x'; else permission[3] = '-';
            if((statt.st_mode) & S_IRGRP) permission[4] = 'r'; else permission[4] = '-';
            if((statt.st_mode) & S_IWGRP) permission[5] = 'w'; else permission[5] = '-';
            if((statt.st_mode) & S_IXGRP) permission[6] = 'x'; else permission[6] = '-';
            if((statt.st_mode) & S_IROTH) permission[7] = 'r'; else permission[7] = '-';
            if((statt.st_mode) & S_IWOTH) permission[8] = 'w'; else permission[8] = '-';
            if((statt.st_mode) & S_IXOTH) permission[9] = 'x'; else permission[9] = '-';

            //initialize end
        
            pd = getpwuid(statt.st_uid);        //get user name
            grp = getgrgid(statt.st_gid);       //get group name

            ttime = localtime(&statt.st_mtime);         //get time information
            strftime(time, 50, "%b %d %R ", ttime);     //format time information for print
      
            //format buf for print
            sprintf(buf, "%s %3d %6s %6s %9d %s %s\n", 
                        permission, statt.st_nlink, pd->pw_name, grp->gr_name, statt.st_size, time, sort[count]);

            strcat(result_buff, buf);
        }
    }
    else {                                      //if has path
        for(count = 0 ; count < check ; count++) {
            memset(permission, 0, sizeof(permission));
            memset(cpath, 0, sizeof(cpath));
            memset(time, 0, sizeof(time));
            memset(buf, 0, sizeof(buf));
            strcpy(cpath, path);
            strcat(cpath, "/");
            strcat(cpath, sort[count]);


            //initialize file type
            lstat(cpath, &statt);
            if(S_ISDIR(statt.st_mode))
               permission[0] = 'd';
            else if(S_ISREG(statt.st_mode))
                permission[0] = '-';
            else if(S_ISCHR(statt.st_mode))
                permission[0] = 'c';
            else if(S_ISBLK(statt.st_mode))
                permission[0] = 'b';
            else if(S_ISFIFO(statt.st_mode))
                permission[0] = 'f';
            else if(S_ISLNK(statt.st_mode))
                permission[0] = 'l';
            else
                permission[0] = 's';

            //initialize permisstion process
            if((statt.st_mode) & S_IRUSR) permission[1] = 'r'; else permission[1] = '-';
            if((statt.st_mode) & S_IWUSR) permission[2] = 'w'; else permission[2] = '-';
            if((statt.st_mode) & S_IXUSR) permission[3] = 'x'; else permission[3] = '-';
            if((statt.st_mode) & S_IRGRP) permission[4] = 'r'; else permission[4] = '-';
            if((statt.st_mode) & S_IWGRP) permission[5] = 'w'; else permission[5] = '-';
            if((statt.st_mode) & S_IXGRP) permission[6] = 'x'; else permission[6] = '-';
            if((statt.st_mode) & S_IROTH) permission[7] = 'r'; else permission[7] = '-';
            if((statt.st_mode) & S_IWOTH) permission[8] = 'w'; else permission[8] = '-';
            if((statt.st_mode) & S_IXOTH) permission[9] = 'x'; else permission[9] = '-';
            
            //initialize end

            pd = getpwuid(statt.st_uid);        //get user name
            grp = getgrgid(statt.st_gid);       //get group name

            ttime = localtime(&statt.st_mtime);        //get time information
            strftime(time, 50, "%b %d %R ", ttime);     //format time information for print
      
            //format buf for print
            sprintf(buf, "%s %3d %6s %6s %9d %s %s\n", 
                        permission, statt.st_nlink, pd->pw_name, grp->gr_name, statt.st_size, time, sort[count]);

            strcat(result_buff, buf);
        }
    }

    if(strcmp(result_buff, "") == 0)
		strcpy(result_buff, "\n");

    closedir(dp);       //close directory

}


//This fucntion excute like dir -al system call
void dir_path(char* path, char* result_buff) 
{
    //for get information of directory
    DIR *dp;
    struct dirent *dirp;

    int count = 0;
    int count2 = 0;
    int check = 0;              //for checking the number of inodes
    char sort[100][50];         //for sorting
    char cpath[100];
    char* tok;                  //for tokenize
    char data[50];
    struct passwd* pd;          //for get information of user name
    struct group* grp;          //for get information of group name
    struct tm* ttime;           //for get information of time
    char permission[11];        //for print permisstion
    char time[50];
    char buf[100]; 

    for(count ; count < 100 ; count ++) 
        memset(sort[count], 0, sizeof(sort[count]));    
    count = 0;

    struct stat statt; 

    if(strcmp(path, ".") == 0) {        //if corrent directory
        dp = opendir(".");

        while(dirp = readdir(dp)) {
            char path[100];
            memset(path, 0, sizeof(path));
            getcwd(path, 100);
            strcat(path, "/");            
            lstat(strcat(path,sort[check]), &statt);
            if(S_ISDIR(statt.st_mode)){             //if directory
                strcpy(sort[check], dirp->d_name);
                strcat(sort[check], "/");
            }
            check++;
            
        }
    }
    else {                              //if has path

        lstat(path, &statt);

        dp = opendir(path);

        while(dirp = readdir(dp)) {
            char temp[100];
            memset(temp, 0, sizeof(temp));
            strcpy(temp, path);
            strcat(temp, "/");
            lstat(strcat(temp,dirp->d_name), &statt);
            if(S_ISDIR(statt.st_mode)) {            //if directory
                strcpy(sort[check], dirp->d_name);
                strcat(sort[check], "/");
                check++;
            }            
        }
    }

    
    //Bubble sorting process

    for(count = 0 ; count < check - 1 ; count++ ) {
        for(count2 = 0 ; count2 < check - count - 1 ; count2 ++) {
            char temp1[50];
            char temp2[50];
            memset(temp1, 0, sizeof(temp1));
            memset(temp2, 0, sizeof(temp2));

            strcpy(temp1, sort[count2]);
            strcpy(temp2, sort[count2 + 1]);
            strlwr(temp1);
            strlwr(temp2);
            if(strcmp(temp1, temp2) > 0) {                
                changestr(sort[count2], sort[count2 + 1]);            
            }
        }
    }

    //Bubble sorting end

    count = 0;
    count2 = 0;

    
    if(strcmp(path, ".") == 0) {                //if current directory
        for(count = 0 ; count < check ; count++) {
            memset(permission, 0, sizeof(permission));
            memset(cpath, 0, sizeof(cpath));
            memset(time, 0, sizeof(time));
            memset(buf, 0, sizeof(buf));
            getcwd(cpath, 100);
            strcat(cpath, "/");
            strcat(cpath, sort[count]);


            //initialize file type
            lstat(cpath, &statt);
            if(S_ISDIR(statt.st_mode))
               permission[0] = 'd';
            else if(S_ISREG(statt.st_mode))
                permission[0] = '-';
            else if(S_ISCHR(statt.st_mode))
                permission[0] = 'c';
            else if(S_ISBLK(statt.st_mode))
                permission[0] = 'b';
            else if(S_ISFIFO(statt.st_mode))
                permission[0] = 'f';
            else if(S_ISLNK(statt.st_mode))
                permission[0] = 'l';
            else
                permission[0] = 's';

            //initialize permission process
            if((statt.st_mode) & S_IRUSR) permission[1] = 'r'; else permission[1] = '-';
            if((statt.st_mode) & S_IWUSR) permission[2] = 'w'; else permission[2] = '-';
            if((statt.st_mode) & S_IXUSR) permission[3] = 'x'; else permission[3] = '-';
            if((statt.st_mode) & S_IRGRP) permission[4] = 'r'; else permission[4] = '-';
            if((statt.st_mode) & S_IWGRP) permission[5] = 'w'; else permission[5] = '-';
            if((statt.st_mode) & S_IXGRP) permission[6] = 'x'; else permission[6] = '-';
            if((statt.st_mode) & S_IROTH) permission[7] = 'r'; else permission[7] = '-';
            if((statt.st_mode) & S_IWOTH) permission[8] = 'w'; else permission[8] = '-';
            if((statt.st_mode) & S_IXOTH) permission[9] = 'x'; else permission[9] = '-';

            //initialize end
        
            pd = getpwuid(statt.st_uid);        //get user name
            grp = getgrgid(statt.st_gid);       //get group name

            ttime = localtime(&statt.st_mtime);         //get time information
            strftime(time, 50, "%b %d %R ", ttime);     //format time information for print
      
            //format buf for print
            sprintf(buf, "%s %3d %6s %6s %9d %s %s\n", 
                        permission, statt.st_nlink, pd->pw_name, grp->gr_name, statt.st_size, time, sort[count]);

            strcat(result_buff, buf);
        }
    }
    else {                                      //if has path
        for(count = 0 ; count < check ; count++) {
            memset(permission, 0, sizeof(permission));
            memset(cpath, 0, sizeof(cpath));
            memset(time, 0, sizeof(time));
            memset(buf, 0, sizeof(buf));
            strcpy(cpath, path);
            strcat(cpath, "/");
            strcat(cpath, sort[count]);


            //initialize file type
            lstat(cpath, &statt);
            if(S_ISDIR(statt.st_mode))
               permission[0] = 'd';
            else if(S_ISREG(statt.st_mode))
                permission[0] = '-';
            else if(S_ISCHR(statt.st_mode))
                permission[0] = 'c';
            else if(S_ISBLK(statt.st_mode))
                permission[0] = 'b';
            else if(S_ISFIFO(statt.st_mode))
                permission[0] = 'f';
            else if(S_ISLNK(statt.st_mode))
                permission[0] = 'l';
            else
                permission[0] = 's';

            //initialize permisstion process
            if((statt.st_mode) & S_IRUSR) permission[1] = 'r'; else permission[1] = '-';
            if((statt.st_mode) & S_IWUSR) permission[2] = 'w'; else permission[2] = '-';
            if((statt.st_mode) & S_IXUSR) permission[3] = 'x'; else permission[3] = '-';
            if((statt.st_mode) & S_IRGRP) permission[4] = 'r'; else permission[4] = '-';
            if((statt.st_mode) & S_IWGRP) permission[5] = 'w'; else permission[5] = '-';
            if((statt.st_mode) & S_IXGRP) permission[6] = 'x'; else permission[6] = '-';
            if((statt.st_mode) & S_IROTH) permission[7] = 'r'; else permission[7] = '-';
            if((statt.st_mode) & S_IWOTH) permission[8] = 'w'; else permission[8] = '-';
            if((statt.st_mode) & S_IXOTH) permission[9] = 'x'; else permission[9] = '-';
            
            //initialize end

            pd = getpwuid(statt.st_uid);        //get user name
            grp = getgrgid(statt.st_gid);       //get group name

            ttime = localtime(&statt.st_mtime);        //get time information
            strftime(time, 50, "%b %d %R ", ttime);     //format time information for print
      
            //format buf for print
            sprintf(buf, "%s %3d %6s %6s %9d %s %s\n", 
                        permission, statt.st_nlink, pd->pw_name, grp->gr_name, statt.st_size, time, sort[count]);

            strcat(result_buff, buf);
        }
    }

    if(strcmp(result_buff, "") == 0)
		strcpy(result_buff, "\n");

    closedir(dp);       //close directory

}


//This function excute swap str1 & str2
void changestr(char* str1, char* str2) 
{
    
    char temp[50];
    memset(temp, 0, sizeof(temp));

    strcpy(temp, str1);
    memset(str1, 0, sizeof(str1));

    strcpy(str1, str2);
    memset(str2, 0, sizeof(str2));

    strcpy(str2, temp);

    return;
}



//This function excute convert upper case alphabet to lower case alphabet
void strlwr(char* str) 
{
    int i = strlen(str);
    int j;
    for(j = 0; j < i; j++) {
        if(str[j] >= 65 && str[j] <= 90)
            str[j] += 32;
    }
}

//if fail to print than return -1
int client_info(struct sockaddr_in* c_addr)
{

	if(!c_addr)
		return -1;

	char bufffff[100];
	char* addr;
	int port;
	port = ntohs(c_addr->sin_port);
	addr = inet_ntoa(c_addr->sin_addr);
	write(1, "==========Client info==========\n", strlen("==========Client info==========\n"));	
	sprintf(bufffff, "client IP: %s\nclient port: %d\n================================\n", addr, port);
	write(1, bufffff, strlen(bufffff));

	return 1;

}

//sig handler for SIGINT
void sh_int(int signum)
{
	Node* p_temp = p_head;
    
    //write enter
    write(STDOUT_FILENO, "\n", 1);


    //if process is not parent
    if(getpid() != parent_pid)
    {
        kill(parent_pid, SIGUSR1);
        return;
    }

    //if list is empty
	if(p_head == NULL)
        exit(0);

    //kill every child process
	while(1)
	{
		kill(p_temp->process_ID, SIGTERM);
		kill(p_temp->cli_pid, SIGUSR1); 
        p_temp = p_temp->p_next;     
        wait(NULL);

		if(p_temp == NULL)
            break;
	}

    //kill parent process
	kill(getpid(), SIGTERM);
}

//sig handler for SIGALRM
void sh_alrm(int signum)
{
	print_node();		
	return;
}

//sig handler for SIGTERM
void sh_term(int signum)
{
    //if child process
    if(getpid() != parent_pid)
    {        
        char temp[200];
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "Client'(%d) Release\n", getpid());
        write(STDOUT_FILENO, temp, strlen(temp));
    }
	//close client
    else
        write(STDOUT_FILENO, "Server Terminated!\n", sizeof("Server Terminated!\n"));

	exit(0);
}

//sig handler for SIGCHLD
void sh_chld(int signum)
{
    //get child's resource
	pid_t child_id;
	child_id = waitpid(-1, NULL, WNOHANG);

    if(child_id != 0)
	   delete_node(child_id);      //delete node
}

//sig handler for SIGUSR1
void sh_usr1(int signum)
{
    Node* p_temp = p_head;

    write(STDOUT_FILENO, "\n", 1);

    //if list is empty
    if(p_head == NULL)
        exit(0);

    //kill every child process
    while(1)
    {
        kill(p_temp->process_ID, SIGTERM);
        kill(p_temp->cli_pid, SIGUSR1); 
        p_temp = p_temp->p_next;     
        wait(NULL);

        if(p_temp == NULL)
            break;
    }

    //kill parent process
    kill(getpid(), SIGTERM);
}

//make node processing
void insert_node(struct sockaddr_in* c_addr, pid_t cur_pid, pid_t cli_pid)
{
    //get current time
	struct timeval curtime;
	gettimeofday(&curtime, NULL);

	Node* p_temp = p_head;

    //if list is empty than make head
	if(p_temp == NULL)
	{
		p_temp = (Node*)malloc(sizeof(Node));

		p_temp->process_ID = cur_pid;
		p_temp->cli_pid = cli_pid;
		p_temp->port = ntohs(c_addr->sin_port);
		p_temp->time = curtime.tv_sec;
		p_temp->p_next = NULL;
		p_temp->process_count = 1;
		p_head = p_temp;
	}
	else 
	{
        //move p_temp to last node
		while(p_temp->p_next != NULL)
			p_temp = p_temp->p_next;

		p_temp->p_next = (Node*)malloc(sizeof(Node));
		p_temp = p_temp->p_next;

		p_temp->process_ID = cur_pid;
		p_temp->cli_pid = cli_pid;
		p_temp->port = ntohs(c_addr->sin_port);
		p_temp->time = curtime.tv_sec;
		p_head->process_count += 1;
		p_temp->p_next = NULL;
	}
}

//delete node processing
void delete_node(pid_t pid)
{

	Node* p_temp = p_head;
	Node* p_prev = p_temp;

    //find node
	while(p_temp->p_next != NULL)
	{
		if(p_temp->process_ID == pid)
			break;
		p_prev = p_temp;
		p_temp = p_temp->p_next;
	}

    //if finded node is p_head
	if(p_temp == p_head)
	{
        //only head exists
		if(p_head->p_next == NULL)
		{
			free(p_head);
			p_head = NULL;
		}
		else  //other nodes exist
		{
			p_head = p_temp->p_next;
			p_head->process_count = p_temp->process_count;
			free(p_temp);
			p_head->process_count -= 1;
		}
		return;
	}

    //replace node
	p_prev->p_next = p_temp->p_next;
	free(p_temp);
	p_head->process_count -= 1;
	return;
}

//print list process
void print_node()
{
    //if list is empty
	if(p_head == NULL)
	{
		alarm(0);     //turn off alarm
		return;
	}

	alarm(10);	        //set alarm 10 sec

	Node* p_temp = p_head;
	struct timeval curtime;            //get current time
	gettimeofday(&curtime, NULL);
    char string_buff[200];

    //if only head exists
	if(p_temp->p_next == NULL)
	{
        memset(string_buff, 0 ,sizeof(string_buff));
        sprintf(string_buff, "Current Number of Client : %d \n   PID     PORT     TIME\n%6d   %6d   %6d\n"
            ,p_head->process_count, p_temp->process_ID, p_temp->port, (curtime.tv_sec - p_temp->time));
        write(STDOUT_FILENO, string_buff, strlen(string_buff));
	}
	else
	{
        memset(string_buff, 0 ,sizeof(string_buff));
        sprintf(string_buff, "Current Number of Client : %d \n   PID     PORT     TIME\n", p_head->process_count);
        write(STDOUT_FILENO, string_buff, strlen(string_buff));

        //print every node's information
		while(p_temp->p_next != NULL) 
		{		
            memset(string_buff, 0 ,sizeof(string_buff));
            sprintf(string_buff, "%6d   %6d   %6d\n", p_temp->process_ID, p_temp->port, (curtime.tv_sec - p_temp->time));
            write(STDOUT_FILENO, string_buff, strlen(string_buff));
			p_temp = p_temp->p_next;
		}
        memset(string_buff, 0 ,sizeof(string_buff));
        sprintf(string_buff, "%6d   %6d   %6d\n", p_temp->process_ID, p_temp->port, (curtime.tv_sec - p_temp->time));
        write(STDOUT_FILENO, string_buff, strlen(string_buff));
	}		
}

