#include <stdio.h>
#include <stdlib.h>

#define MAXWORD 200
char word[MAXWORD];

char * nextword(char * str) {

  int c;
  int i = 0;

  while ((c = str[i]) != '\0') {
	if(c == ' ' || c == '\t' || c == '\n' || c == '\r'){
		if (i > 0){
			word[i] = '\0';
			return word;
		}
	} else {
		word[i] = c;
		i++;
	}
  }

  return NULL;
}
