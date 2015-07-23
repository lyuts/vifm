#include <stic.h>

#include <sys/stat.h> /* chmod() */

#include <sys/types.h> /* stat */
#include <sys/stat.h> /* stat */
#include <unistd.h> /* lstat() */

#include "../../src/compat/fs_limits.h"
#include "../../src/compat/os.h"
#include "../../src/io/iop.h"
#include "../../src/utils/fs.h"
#include "../../src/utils/utils.h"

#include "utils.h"

static void file_is_copied(const char original[]);

static int not_windows(void);

TEST(dir_is_not_copied)
{
	io_args_t args = {
		.arg1.src = TEST_DATA_PATH "/existing-files",
		.arg2.dst = SANDBOX_PATH "/existing-files",
	};
	ioe_errlst_init(&args.result.errors);

	assert_failure(iop_cp(&args));

	assert_true(args.result.errors.error_count != 0);
	ioe_errlst_free(&args.result.errors);
}

TEST(empty_file_is_copied)
{
	create_test_file(SANDBOX_PATH "/empty");

	{
		io_args_t args = {
			.arg1.src = SANDBOX_PATH "/empty",
			.arg2.dst = SANDBOX_PATH "/empty-copy",
		};
		ioe_errlst_init(&args.result.errors);

		assert_success(iop_cp(&args));

		assert_int_equal(0, args.result.errors.error_count);
	}

	assert_true(files_are_identical(SANDBOX_PATH "/empty",
				SANDBOX_PATH "/empty-copy"));

	delete_test_file(SANDBOX_PATH "/empty");
	delete_test_file(SANDBOX_PATH "/empty-copy");
}

TEST(file_is_not_overwritten_if_not_asked)
{
	create_test_file(SANDBOX_PATH "/empty");
	clone_test_file(SANDBOX_PATH "/empty", SANDBOX_PATH "/empty-copy");

	{
		io_args_t args = {
			.arg1.src = TEST_DATA_PATH "/read/two-lines",
			.arg2.dst = SANDBOX_PATH "/empty-copy",
			.arg3.crs = IO_CRS_FAIL,
		};
		ioe_errlst_init(&args.result.errors);

		assert_failure(iop_cp(&args));

		assert_true(args.result.errors.error_count != 0);
		ioe_errlst_free(&args.result.errors);
	}

	assert_true(files_are_identical(SANDBOX_PATH "/empty",
				SANDBOX_PATH "/empty-copy"));
	assert_false(files_are_identical(SANDBOX_PATH "/read/two-lines",
				SANDBOX_PATH "/empty-copy"));

	delete_test_file(SANDBOX_PATH "/empty");
	delete_test_file(SANDBOX_PATH "/empty-copy");
}

TEST(file_is_overwritten_if_asked)
{
	{
		io_args_t args = {
			.arg1.src = TEST_DATA_PATH "/read/two-lines",
			.arg2.dst = SANDBOX_PATH "/two-lines",
			.arg3.crs = IO_CRS_FAIL,
		};
		ioe_errlst_init(&args.result.errors);

		assert_success(iop_cp(&args));

		assert_int_equal(0, args.result.errors.error_count);
	}

	assert_false(files_are_identical(TEST_DATA_PATH "/read/binary-data",
				SANDBOX_PATH "/two-lines"));

	{
		io_args_t args = {
			.arg1.src = TEST_DATA_PATH "/read/binary-data",
			.arg2.dst = SANDBOX_PATH "/two-lines",
			.arg3.crs = IO_CRS_REPLACE_FILES,
		};
		ioe_errlst_init(&args.result.errors);

		assert_success(iop_cp(&args));

		assert_int_equal(0, args.result.errors.error_count);
	}

	assert_true(files_are_identical(TEST_DATA_PATH "/read/binary-data",
				SANDBOX_PATH "/two-lines"));

	delete_test_file(SANDBOX_PATH "/two-lines");
}

TEST(block_size_file_is_copied)
{
	file_is_copied(TEST_DATA_PATH "/various-sizes/block-size-file");
}

TEST(block_size_minus_one_file_is_copied)
{
	file_is_copied(TEST_DATA_PATH "/various-sizes/block-size-minus-one-file");
}

TEST(block_size_plus_one_file_is_copied)
{
	file_is_copied(TEST_DATA_PATH "/various-sizes/block-size-plus-one-file");
}

TEST(double_block_size_file_is_copied)
{
	file_is_copied(TEST_DATA_PATH "/various-sizes/double-block-size-file");
}

TEST(double_block_size_minus_one_file_is_copied)
{
	file_is_copied(TEST_DATA_PATH
			"/various-sizes/double-block-size-minus-one-file");
}

TEST(double_block_size_plus_one_file_is_copied)
{
	file_is_copied(TEST_DATA_PATH
			"/various-sizes/double-block-size-plus-one-file");
}

static void
file_is_copied(const char original[])
{
	{
		io_args_t args = {
			.arg1.src = original,
			.arg2.dst = SANDBOX_PATH "/copy",
		};
		ioe_errlst_init(&args.result.errors);

		assert_success(iop_cp(&args));

		assert_int_equal(0, args.result.errors.error_count);
	}

	assert_true(files_are_identical(SANDBOX_PATH "/copy", original));

	delete_test_file(SANDBOX_PATH "/copy");
}

TEST(appending_works_for_files)
{
	uint64_t size;

	clone_test_file(TEST_DATA_PATH "/various-sizes/block-size-minus-one-file",
			SANDBOX_PATH "/appending");
	assert_success(chmod(SANDBOX_PATH "/appending", 0700));

	size = get_file_size(SANDBOX_PATH "/appending");

	{
		io_args_t args = {
			.arg1.src = TEST_DATA_PATH "/various-sizes/block-size-file",
			.arg2.dst = SANDBOX_PATH "/appending",
			.arg3.crs = IO_CRS_APPEND_TO_FILES,
		};
		ioe_errlst_init(&args.result.errors);

		assert_success(iop_cp(&args));

		assert_int_equal(0, args.result.errors.error_count);
	}

	assert_int_equal(size + 1, get_file_size(SANDBOX_PATH "/appending"));
	assert_success(chmod(SANDBOX_PATH "/appending", 0700));

	{
		io_args_t args = {
			.arg1.src = TEST_DATA_PATH "/various-sizes/block-size-plus-one-file",
			.arg2.dst = SANDBOX_PATH "/appending",
			.arg3.crs = IO_CRS_APPEND_TO_FILES,
		};
		ioe_errlst_init(&args.result.errors);

		assert_success(iop_cp(&args));

		assert_int_equal(0, args.result.errors.error_count);
	}

	assert_int_equal(size + 2, get_file_size(SANDBOX_PATH "/appending"));

	delete_test_file(SANDBOX_PATH "/appending");
}

TEST(appending_does_not_shrink_files)
{
	uint64_t size;

	clone_test_file(TEST_DATA_PATH "/read/two-lines", SANDBOX_PATH "/two-lines");
	assert_success(chmod(SANDBOX_PATH "/two-lines", 0700));

	size = get_file_size(SANDBOX_PATH "/two-lines");

	{
		io_args_t args = {
			.arg1.src = TEST_DATA_PATH "/existing-files/a",
			.arg2.dst = SANDBOX_PATH "/two-lines",
			.arg3.crs = IO_CRS_APPEND_TO_FILES,
		};
		ioe_errlst_init(&args.result.errors);

		assert_success(iop_cp(&args));

		assert_int_equal(0, args.result.errors.error_count);
	}

	assert_int_equal(size, get_file_size(SANDBOX_PATH "/two-lines"));

	{
		io_args_t args = {
			.arg1.path = SANDBOX_PATH "/two-lines",
		};
		ioe_errlst_init(&args.result.errors);

		assert_success(iop_rmfile(&args));

		assert_int_equal(0, args.result.errors.error_count);
	}
}

/* Windows doesn't support Unix-style permissions. */
TEST(file_permissions_are_preserved, IF(not_windows))
{
	struct stat src;
	struct stat dst;

	create_test_file(SANDBOX_PATH "/file");

	assert_int_equal(0, chmod(SANDBOX_PATH "/file", 0200));

	assert_int_equal(0, lstat(SANDBOX_PATH "/file", &src));
	assert_false((src.st_mode & 0777) == 0600);

	assert_int_equal(0, chmod(SANDBOX_PATH "/file", 0600));

	{
		io_args_t args = {
			.arg1.src = SANDBOX_PATH "/file",
			.arg2.dst = SANDBOX_PATH "/file-copy",
		};
		ioe_errlst_init(&args.result.errors);

		assert_success(iop_cp(&args));

		assert_int_equal(0, args.result.errors.error_count);
	}

	assert_int_equal(0, lstat(SANDBOX_PATH "/file", &src));
	assert_int_equal(0, lstat(SANDBOX_PATH "/file-copy", &dst));
	assert_int_equal(src.st_mode & 0777, dst.st_mode & 0777);

	delete_test_file(SANDBOX_PATH "/file");
	delete_test_file(SANDBOX_PATH "/file-copy");
}

/* Creating symbolic links on Windows requires administrator rights. */
TEST(file_symlink_copy_is_symlink, IF(not_windows))
{
	char old_target[PATH_MAX];
	char new_target[PATH_MAX];

	{
		io_args_t args = {
			.arg1.path = TEST_DATA_PATH "/read/two-lines",
			.arg2.target = SANDBOX_PATH "/sym-link",
		};
		ioe_errlst_init(&args.result.errors);

		assert_success(iop_ln(&args));

		assert_int_equal(0, args.result.errors.error_count);
	}

	assert_true(is_symlink(SANDBOX_PATH "/sym-link"));

	{
		io_args_t args = {
			.arg1.src = SANDBOX_PATH "/sym-link",
			.arg2.dst = SANDBOX_PATH "/sym-link-copy",
		};
		ioe_errlst_init(&args.result.errors);

		assert_success(iop_cp(&args));

		assert_int_equal(0, args.result.errors.error_count);
	}

	assert_true(is_symlink(SANDBOX_PATH "/sym-link"));
	assert_true(is_symlink(SANDBOX_PATH "/sym-link-copy"));

	assert_success(get_link_target(SANDBOX_PATH "/sym-link", old_target,
				sizeof(old_target)));
	assert_success(get_link_target(SANDBOX_PATH "/sym-link-copy", new_target,
				sizeof(new_target)));

	assert_string_equal(new_target, old_target);

	delete_test_file(SANDBOX_PATH "/sym-link");
	delete_test_file(SANDBOX_PATH "/sym-link-copy");
}

/* Creating symbolic links on Windows requires administrator rights. */
TEST(dir_symlink_copy_is_symlink, IF(not_windows))
{
	os_mkdir(SANDBOX_PATH "/dir", 0700);
	assert_true(is_dir(SANDBOX_PATH "/dir"));

	{
		io_args_t args = {
			.arg1.path = "dir",
			.arg2.target = SANDBOX_PATH "/dir-sym-link",
		};
		ioe_errlst_init(&args.result.errors);

		assert_success(iop_ln(&args));

		assert_int_equal(0, args.result.errors.error_count);
	}

	assert_true(is_symlink(SANDBOX_PATH "/dir-sym-link"));
	assert_true(is_dir(SANDBOX_PATH "/dir-sym-link"));

	{
		io_args_t args = {
			.arg1.src = SANDBOX_PATH "/dir-sym-link",
			.arg2.dst = SANDBOX_PATH "/dir-sym-link-copy",
		};
		ioe_errlst_init(&args.result.errors);

		assert_success(iop_cp(&args));

		assert_int_equal(0, args.result.errors.error_count);
	}

	assert_true(is_symlink(SANDBOX_PATH "/dir-sym-link"));
	assert_true(is_dir(SANDBOX_PATH "/dir-sym-link"));
	assert_true(is_symlink(SANDBOX_PATH "/dir-sym-link-copy"));
	assert_true(is_dir(SANDBOX_PATH "/dir-sym-link-copy"));

	delete_test_file(SANDBOX_PATH "/dir-sym-link");
	delete_test_file(SANDBOX_PATH "/dir-sym-link-copy");

	{
		io_args_t args = {
			.arg1.path = SANDBOX_PATH "/dir",
		};
		ioe_errlst_init(&args.result.errors);

		assert_success(iop_rmdir(&args));

		assert_int_equal(0, args.result.errors.error_count);
	}
}

/* Windows lacks definitions some declarations. */
#ifndef _WIN32

/* No named fifo in file systems on Windows. */
TEST(fifo_is_copied, IF(not_windows))
{
	struct stat st;

	io_args_t args = {
		.arg1.src = SANDBOX_PATH "/fifo-src",
		.arg2.dst = SANDBOX_PATH "/fifo-dst",

		.result.errors = IOE_ERRLST_INIT,
	};

	assert_success(mkfifo(args.arg1.src, 0755));

	assert_success(iop_cp(&args));
	assert_int_equal(0, args.result.errors.error_count);

	assert_success(lstat(SANDBOX_PATH "/fifo-dst", &st));
	assert_true(S_ISFIFO(st.st_mode));

	args.arg1.path = SANDBOX_PATH "/fifo-src";
	assert_success(iop_rmfile(&args));
	assert_int_equal(0, args.result.errors.error_count);

	args.arg1.path = SANDBOX_PATH "/fifo-dst";
	assert_success(iop_rmfile(&args));
	assert_int_equal(0, args.result.errors.error_count);
}

/* No named sockets in file systems on Windows. */
TEST(socket_is_copied, IF(not_windows))
{
	struct stat st;

	io_args_t args = {
		.arg1.src = SANDBOX_PATH "/sock-src",
		.arg2.dst = SANDBOX_PATH "/sock-dst",

		.result.errors = IOE_ERRLST_INIT,
	};

	assert_success(mknod(args.arg1.src, S_IFSOCK | 0755, 0));

	assert_success(iop_cp(&args));
	assert_int_equal(0, args.result.errors.error_count);

	assert_success(lstat(SANDBOX_PATH "/sock-dst", &st));
	assert_true(S_ISSOCK(st.st_mode));

	args.arg1.path = SANDBOX_PATH "/sock-src";
	assert_success(iop_rmfile(&args));
	assert_int_equal(0, args.result.errors.error_count);

	args.arg1.path = SANDBOX_PATH "/sock-dst";
	assert_success(iop_rmfile(&args));
	assert_int_equal(0, args.result.errors.error_count);
}

#endif

static int
not_windows(void)
{
	return get_env_type() != ET_WIN;
}

/* vim: set tabstop=2 softtabstop=2 shiftwidth=2 noexpandtab cinoptions-=(0 : */
/* vim: set cinoptions+=t0 : */
