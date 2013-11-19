#include "seatest.h"

void chgrp_tests(void);
void chmod_tests(void);
void chown_tests(void);
void cp_tests(void);
void ln_tests(void);
void mkdir_tests(void);
void mkfile_tests(void);
void mv_tests(void);
void rmdir_tests(void);
void rmfile_tests(void);

void
all_tests(void)
{
	chgrp_tests();
	chmod_tests();
	chown_tests();
	cp_tests();
	ln_tests();
	mkdir_tests();
	mkfile_tests();
	mv_tests();
	rmdir_tests();
	rmfile_tests();
}

int
main(void)
{
	return run_tests(&all_tests) == 0;
}

/* vim: set tabstop=2 softtabstop=2 shiftwidth=2 noexpandtab cinoptions-=(0 : */
/* vim: set cinoptions+=t0 : */
