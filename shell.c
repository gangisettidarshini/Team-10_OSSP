#include <stdio.h>
#include<string.h>
int main(){
char input[100];
while(1){
printf("myshell>");
fflush(stdout);
if(fgets(input,sizeof(input),stdin)==NULL){
printf("\nExiting...\n");
break;
}
input[strcspn(input, "\n")]='\0';
if(strcmp(input,"exit")==0){
printf("Exiting shell...\n");
break;
}
