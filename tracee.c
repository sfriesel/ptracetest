#include <stdio.h>

int main() {
	FILE *fp;
	fp = fopen("/nonexistant", "r");
	if (!fp) {
		fputs("failed to open file, expected\n", stderr);
	} else {
		fclose(fp);
		fputs("unexpectedly succeeded opening file\n", stderr);
		return 1;
	}
	fp = fopen("/dev/random", "r");
	if (fp) {
		fputs("opened file, expected\n", stderr);
		fclose(fp);
	} else {
		fputs("unexpectedly failed to open file\n", stderr);
	}
	fp = fopen("/tmp/optional", "r");
	if (fp) {
		fputs("successfully opened optional file\n", stderr);
		fclose(fp);
	} else {
		fputs("failed to open optional file\n", stderr);
	}
	return 0;
}
