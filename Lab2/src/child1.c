#include<unistd.h>
#include <ctype.h>
int main() {
    char c[100];
    int n;
    while(n=read(0, &c, 100)) {
        for (int i = 0; i <= n; ++i) {
            if (c[i] != '\n' && c[i] != '\0') {
                c[i] = toupper(c[i]);
            } else {
                break;
            }
        }
        write(1, &c, n);
    }
    return 0;
}
