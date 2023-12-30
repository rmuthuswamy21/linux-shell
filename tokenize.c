#include <stdio.h>
#include <stdbool.h>

int main(int argc, char **argv) {
    char c;
    char q;
    bool spaceWasPrevCharacter = false;
    bool inQuotes = false;

    while ((c = getchar()) != EOF) {
        if (c == '/') {
            if ((q = getchar()) != EOF) {
                if (q == '"') {
                    putchar(q);
                    if (inQuotes) {
                        putchar('\n');
                    }
                    inQuotes = !inQuotes;
                }
                else {
                    putchar(c);
		    if(q == ' ' || q == '\t') {
			if (!spaceWasPrevCharacter && !inQuotes) {
			    putchar('\n');
			}
			spaceWasPrevCharacter = true;
		    }
                    putchar(q);
                }
            }
            else {
                putchar(c);
            }
        } else if (c == '"') {
            inQuotes = !inQuotes;
        }  else if (c == ' ' || c == '\t') {
            if (!spaceWasPrevCharacter && !inQuotes) {
                putchar('\n');
            }
	    if (inQuotes) {
		putchar(' ');
	    }
            spaceWasPrevCharacter = true;
        } else if (c == '(' || c == ')' || c == '<' || c == '>' || c == ';' || c == '|'){
            if (!spaceWasPrevCharacter && !inQuotes) {
                putchar('\n');
            }
            putchar(c);
            spaceWasPrevCharacter = false;
        } else {
            putchar(c);
            spaceWasPrevCharacter = false;
        }
    }

    if (!spaceWasPrevCharacter) {
        putchar('\n');
    }

    return 0;
}
