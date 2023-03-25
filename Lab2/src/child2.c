#include<unistd.h>
#include <ctype.h>
void *spaces(char* str) {
    int i, x;
    for(i=x=0; str[i]; ++i)
        if(!isspace(str[i]) || (i > 0 && !isspace(str[i-1])))
            str[x++] = str[i];
    str[x] = '\0';
    return str;
}



int main() {
    char c[100];
    while (read(0, &c, 100)) {
        spaces(c);
        size_t k = sizeof(c)/sizeof(c[0]);
        write(1, c, k);
    }
}
