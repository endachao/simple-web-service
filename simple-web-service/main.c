//
//  main.c
//  simple-web-service
//
//  Created by yuanchao on 2016/9/28.
//  Copyright © 2016年 yuanchao. All rights reserved.


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

void sendData(void *sock,char * filename);
void catHTML(void *sock,char * filename);
void catJPEG(void *sock,char * filename);
void sendError(void *sock);
void requestHandling(void *sock);
void errorHandling(char * message);

int main(int argc, const char * argv[]) {
    // insert code here...
    int service_sock;   // 保存创建的服务器套接字
    int clent_sock;     // 保存客户端套接字
    
    
    struct sockaddr_in service_addr;    // 保存服务器套接字地址信息
    struct sockaddr_in clent_addr;      // 保存客户端套字地址信息
    socklen_t clent_addr_size;          // 客户端套字节地址变量大小
    
    
    // 创建一个服务器套接字
    
    service_sock = socket(PF_INET, SOCK_STREAM, 0);
    
    if(service_sock == -1){
        errorHandling("socket() error");
    }
    
    // 配置套接字IP和端口信息
    memset(&service_addr, 0, sizeof(service_addr));
    service_addr.sin_family = AF_INET;
    service_addr.sin_addr.s_addr = htonl(INADDR_ANY);   // htonl ()用来将参数指定的32 位hostlong 转换成网络字符顺序.
    service_addr.sin_port = htons(PORT);                // htons()用来将参数指定的16 位hostshort 转换成网络字符顺序.
    
    // 绑定服务器套接字
    if(bind(service_sock, (struct sockaddr*)&service_addr, sizeof(service_addr)) == -1){
        errorHandling("bind() error");
    }
    
    
    // 监听服务器套接字
    if(listen(service_sock, 5) == -1){
        errorHandling("listen() error");
    }
    
    while (1) {
        // 接收客户端的请求
        clent_addr_size = sizeof(clent_addr);
        
        clent_sock = accept(service_sock, (struct sockaddr *)&clent_addr, &clent_addr_size);
        if(clent_sock == -1){
            errorHandling("accept() error");
        }
        
        requestHandling((void *) &clent_sock);
    }
    
    // 关闭套接字
    close(service_sock);
    
    
    return 0;
}

void errorHandling(char * message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void requestHandling(void *sock){
    
    int clent_sock = *((int *) sock);
    char buf[1024];     // 缓冲区
    char method[10];    // 保存请求方法
    char filename[20];  // 保存文件名
    
    // 读取游览器请求内容
    read(clent_sock, buf, sizeof(buf)-1);
    
    if(strstr(buf, "HTTP/") == NULL){
        sendError(sock);
        close(clent_sock);
        return ;
    }
    
    // 提取请求方法至 method 数组中
    strcpy(method, strtok(buf, " /"));
    
    // 提取请求的文件名至 filename 数组中
    strcpy(filename, strtok(NULL, " /"));
    
    // 判断请求方法是否是 GET，不是 GET 则发送错误
    if(strcmp(method, "GET") != 0){
        sendError(sock);
        close(clent_sock);
        return ;
    }
    
    // 访问请求文件
    sendData(sock,filename);
    
}


/**
 * 处理游览器请求的文件
 */
void sendData(void *sock,char * filename){
    
    int clent_sock = *((int *) sock);
    char buf[20];
    char ext[10];
    
    strcpy(buf, filename);
    
    // 判断文件类型
    strtok(buf, ".");
    strcpy(ext, strtok(NULL, "."));
    
    if(strcmp(ext, "html") == 0){
        // html 文件
        catHTML(sock, filename);
    }else if (strcmp(ext, "jpg") == 0){
        // jpg
        catJPEG(sock, filename);
    }else if (strcmp(ext, "php") == 0){
        //php
    }else{
        sendError(sock);
        close(clent_sock);
        return ;
    }
    
}

/**
 * 读取 HTML 文件
 */
void catHTML(void * sock,char * filename){
    
    int clent_sock = *((int *) sock);
    char buf[1024];
    FILE *fp;
    
    char status[] = "HTTP/1.0 200 OK\r\n";
    char header[] = "Server: A Simple Web Server\r\nContent-Type: text/html\r\n\r\n";
    
    // 发送响应报文
    write(clent_sock, status, strlen(status));
    
    // 发送报文头
    write(clent_sock, header, strlen(header));
    
    fp = fopen(filename, "r");
    
    if(fp == NULL){
        sendError(sock);
        close(clent_sock);
        errorHandling("open file failed");
        return ;
    }
    
    // 读取文件内容并发送
    fgets(buf, sizeof(buf), fp);
    
    while (!feof(fp)) {
        write(clent_sock, buf, strlen(buf));
        fgets(buf, sizeof(buf), fp);
    }
    
    fclose(fp);
    close(clent_sock);
    
}


void catJPEG(void * sock,char * filename){
    
    int clent_sock = *((int *) sock);
    
    char buf[1024];
    
    FILE *fp;
    FILE *fw;
    
    char status[] = "HTTP/1.0 200 OK\r\n";
    char header[] = "Server: A Simple Web Server\r\nContent-Type: image/jpeg\r\n\r\n";
    
    // 发送响应报文
    write(clent_sock, status, strlen(status));
    
    // 发送报文头
    write(clent_sock, header, strlen(header));

    
    // 图片以二进制格式打开
    fp = fopen(filename, "rb");
    if(fp == NULL){
        sendError(sock);
        close(clent_sock);
        errorHandling("open file failed");
        return ;
    }
    
    // 在套接字上打开一个文件句柄
    fw = fdopen(clent_sock, "w");
    
    fread(buf, 1,sizeof(buf), fp);
    
    while (!feof(fp)) {
        fwrite(buf, 1, sizeof(buf), fw);
        fread(buf, 1,sizeof(buf), fp);
    }
    
    fclose(fw);
    fclose(fp);
    close(clent_sock);
}




void sendError(void *sock){
    int clent_sock = *((int *) sock);
    char status[] = "HTTP/1.0 400 Bad Request\r\n";
    char header[] = "Server: A Simple Web Server\r\nContent-Type: text/html\r\n\r\n";
    char body[] = "<html><head><title>Bad Request</title></head><body><p>请求出错，请检查！</p></body></html>";
    // 向客户端套接字发送信息
    write(clent_sock, status, sizeof(status));
    write(clent_sock, header, sizeof(header));
    write(clent_sock, body, sizeof(body));
}




























