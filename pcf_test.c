#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "pcf.h"

int main() {
	error err = NULL;
	pf pf;
	pf.init = 0;
	pcf_dump_ascii(&pf, "wenquanyi_12pt.pcf", 0x521d, &err); // \u521d = 初
	if (err) printf("%s\n", err);
	pcf_dump_ascii(&pf, "wenquanyi_12pt.pcf", 0x59cb, &err); // \u59cb = 始
	if (err) printf("%s\n", err);
}
