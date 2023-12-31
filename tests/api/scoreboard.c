/*
 * ProFTPD - FTP server testsuite
 * Copyright (c) 2008-2020 The ProFTPD Project team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA.
 *
 * As a special exemption, The ProFTPD Project team and other respective
 * copyright holders give permission to link this program with OpenSSL, and
 * distribute the resulting executable, without including the source code for
 * OpenSSL in the source distribution.
 */

/* Scoreboard API tests */

#include "tests.h"

static pool *p = NULL;

static const char *test_dir = "/tmp/prt-scoreboard/";
static const char *test_file = "/tmp/prt-scoreboard/test.dat";
static const char *test_mutex = "/tmp/prt-scoreboard/test.dat.lck";
static const char *test_file2 = "/tmp/prt-scoreboard-mutex.dat";

static void set_up(void) {
  (void) unlink(test_file);
  (void) unlink(test_file2);
  (void) unlink(test_mutex);
  (void) rmdir(test_dir);

  if (p == NULL) {
    p = permanent_pool = make_sub_pool(NULL);
  }

  ServerType = SERVER_STANDALONE;
  init_netaddr();

  if (getenv("TEST_VERBOSE") != NULL) {
    pr_trace_set_levels("lock", 1, 20);
    pr_trace_set_levels("scoreboard", 1, 20);
  }
}

static void tear_down(void) {
  (void) unlink(test_file);
  (void) unlink(test_file2);
  (void) unlink(test_mutex);
  (void) rmdir(test_dir);

  if (getenv("TEST_VERBOSE") != NULL) {
    pr_trace_set_levels("lock", 0, 0);
    pr_trace_set_levels("scoreboard", 0, 0);
  }

  if (p) {
    destroy_pool(p);
    p = permanent_pool = NULL;
  }
}

START_TEST (scoreboard_get_test) {
  const char *ok, *res;

  ok = PR_RUN_DIR "/proftpd.scoreboard";

  res = pr_get_scoreboard();
  ck_assert_msg(res != NULL, "Failed to get scoreboard path: %s",
    strerror(errno));
  ck_assert_msg(strcmp(res, ok) == 0,
    "Expected scoreboard path '%s', got '%s'", ok, res);

  ok = PR_RUN_DIR "/proftpd.scoreboard.lck";

  res = pr_get_scoreboard_mutex();
  ck_assert_msg(res != NULL, "Failed to get scoreboard mutex path: %s",
    strerror(errno));
  ck_assert_msg(strcmp(res, ok) == 0,
    "Expected scoreboard mutex path '%s', got '%s'", ok, res);
}
END_TEST

START_TEST (scoreboard_set_test) {
  int fd, res;
  const char *path;

  mark_point();
  res = pr_set_scoreboard(NULL);
  ck_assert_msg(res < 0, "Failed to handle null argument");
  ck_assert_msg(errno == EINVAL, "Failed to set errno to EINVAL");

  mark_point();
  res = pr_set_scoreboard("foo");
  ck_assert_msg(res < 0, "Failed to handle non-path argument");
  ck_assert_msg(errno == EINVAL, "Failed to set errno to EINVAL (got %d)",
    errno);

  mark_point();
  res = pr_set_scoreboard("foo/");
  ck_assert_msg(res < 0, "Failed to handle relative path argument");
  ck_assert_msg(errno == EINVAL, "Failed to set errno to EINVAL (got %d)",
    errno);

  mark_point();
  res = pr_set_scoreboard("/foo");
  ck_assert_msg(res < 0, "Failed to handle nonexistent path argument");
  ck_assert_msg(errno == EINVAL, "Failed to set errno to EINVAL (got %d)",
    errno);

  mark_point();
  res = pr_set_scoreboard("/tmp");
  ck_assert_msg(res < 0, "Failed to handle nonexistent path argument");
  ck_assert_msg(errno == EINVAL, "Failed to set errno to EINVAL (got %d)",
    errno);

  mark_point();
  res = pr_set_scoreboard("/tmp/");
  ck_assert_msg(res < 0, "Failed to handle nonexistent path argument");
  ck_assert_msg(errno == EINVAL, "Failed to set errno to EINVAL (got %d)",
    errno);

  mark_point();
  res = pr_set_scoreboard("/tmp/foo/bar");
  ck_assert_msg(res < 0, "Failed to handle nonexistent path argument");
  ck_assert_msg(errno == ENOENT, "Expected ENOENT (%d), got %s (%d)", ENOENT,
    strerror(errno), errno);

  res = mkdir(test_dir, 0777);
  ck_assert_msg(res == 0,
    "Failed to create tmp directory '%s': %s", test_dir, strerror(errno));
  res = chmod(test_dir, 0777);
  ck_assert_msg(res == 0,
    "Failed to create set 0777 perms on '%s': %s", test_dir, strerror(errno));

  mark_point();
  fd = open("/tmp/foo", O_CREAT|O_RDONLY, 0400);
  if (fd < 0) {
    return;
  }
  (void) close(fd);

  res = pr_set_scoreboard("/tmp/foo/bar");
  ck_assert_msg(res < 0, "Failed to handle non-directory path argument");
  ck_assert_msg(errno == ENOTDIR, "Expected ENOTDIR (%d), got %s (%d)", ENOTDIR,
    strerror(errno), errno);
  (void) unlink("/tmp/foo");

  mark_point();
  res = pr_set_scoreboard(test_dir);
  ck_assert_msg(res < 0, "Failed to handle nonexistent file argument");
  ck_assert_msg(errno == EINVAL, "Failed to set errno to EINVAL");

  mark_point();
  res = pr_set_scoreboard("/tmp/prt-scoreboard/bar");
  ck_assert_msg(res < 0, "Failed to handle world-writable path argument");
  ck_assert_msg(errno == EPERM, "Failed to set errno to EPERM");

  res = chmod(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to set 0775 perms on '%s': %s", test_dir,
    strerror(errno));

  mark_point();
  res = pr_set_scoreboard("/tmp/prt-scoreboard/bar");
  ck_assert_msg(res == 0, "Failed to set scoreboard: %s", strerror(errno));
  (void) rmdir(test_dir);

  path = pr_get_scoreboard();
  ck_assert_msg(path != NULL, "Failed to get scoreboard path: %s",
    strerror(errno));
  ck_assert_msg(strcmp("/tmp/prt-scoreboard/bar", path) == 0,
    "Expected '%s', got '%s'", "/tmp/prt-scoreboard/bar", path);

  (void) rmdir(test_dir);
}
END_TEST

START_TEST (scoreboard_set_mutex_test) {
  int res;
  const char *path;

  res = pr_set_scoreboard_mutex(NULL);
  ck_assert_msg(res == -1, "Failed to handle null argument");
  ck_assert_msg(errno == EINVAL, "Failed to set errno to EINVAL");

  res = pr_set_scoreboard_mutex("/tmp");
  ck_assert_msg(res == 0, "Failed to set scoreboard mutex: %s",
    strerror(errno));

  path = pr_get_scoreboard_mutex();
  ck_assert_msg(path != NULL, "Failed to get scoreboard mutex path: %s",
    strerror(errno));
  ck_assert_msg(strcmp("/tmp", path) == 0,
    "Expected '%s', got '%s'", "/tmp", path);
}
END_TEST

START_TEST (scoreboard_open_close_test) {
  int res;
  const char *symlink_path = "/tmp/prt-scoreboard/symlink";

  res = mkdir(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to create directory '%s': %s", test_dir,
    strerror(errno));

  res = chmod(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to set perms on '%s': %s", test_dir,
    strerror(errno));

  res = pr_set_scoreboard(test_file);
  ck_assert_msg(res == 0, "Failed to set scoreboard to '%s': %s", test_file,
    strerror(errno));

  if (symlink(symlink_path, test_file) == 0) {

    res = pr_open_scoreboard(O_RDWR);
    if (res == 0) {
      (void) unlink(symlink_path);

      ck_abort_msg("Unexpectedly opened symlink scoreboard");
    }

    if (errno != EPERM) {
      int xerrno = errno;

      (void) unlink(symlink_path);

      ck_abort_msg("Failed to set errno to EPERM (got %d)", xerrno);
    }

    (void) unlink(symlink_path);

    res = pr_set_scoreboard(test_file);
    ck_assert_msg(res == 0, "Failed to set scoreboard to '%s': %s", test_file,
      strerror(errno));
  }

  res = pr_open_scoreboard(O_RDONLY);
  ck_assert_msg(res < 0, "Unexpectedly opened scoreboard using O_RDONLY");

  if (errno != EINVAL) {
    int xerrno = errno;

    (void) unlink(symlink_path);

    ck_abort_msg("Failed to set errno to EINVAL (got %d)", xerrno);
  }

  (void) unlink(test_mutex);
  (void) unlink(test_file);
  res = pr_open_scoreboard(O_RDWR);
  ck_assert_msg(res == 0, "Failed to open scoreboard: %s", strerror(errno));

  /* Try opening the scoreboard again; it should be OK, since we are the
   * opener.
   */
  res = pr_open_scoreboard(O_RDWR);
  ck_assert_msg(res == 0, "Failed to open scoreboard again: %s", strerror(errno));

  /* Now that we have a scoreboard, try opening it again using O_RDONLY. */
  mark_point();
  pr_close_scoreboard(FALSE);

  res = pr_open_scoreboard(O_RDONLY);
  ck_assert_msg(res < 0, "Unexpectedly opened scoreboard using O_RDONLY");
  ck_assert_msg(errno == EINVAL, "Expected EINVAL (%d), got %s (%d)", EINVAL,
    strerror(errno), errno);

  mark_point();
  res = pr_open_scoreboard(O_RDWR);
  ck_assert_msg(res == 0, "Failed to open scoreboard: %s", strerror(errno));

  res = pr_close_scoreboard(FALSE);
  ck_assert_msg(res == 0, "Failed to close scoreboard: %s", strerror(errno));

  /* Close the already-closed scoreboard, to assert that it is a no-op. */
  mark_point();
  res = pr_close_scoreboard(FALSE);
  ck_assert_msg(res == 0, "Failed to close scoreboard: %s", strerror(errno));

  (void) unlink(test_mutex);
  (void) unlink(test_file);
  (void) rmdir(test_dir);
}
END_TEST

START_TEST (scoreboard_open_invalid_header_test) {
  int fd, res;
  pr_scoreboard_header_t sch;

  res = mkdir(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to create directory '%s': %s", test_dir,
    strerror(errno));

  res = chmod(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to set perms on '%s': %s", test_dir,
    strerror(errno));

  (void) unlink(test_file);
  res = pr_set_scoreboard(test_file);
  ck_assert_msg(res == 0, "Failed to set scoreboard to '%s': %s", test_file,
    strerror(errno));

  /* Bad magic */
  mark_point();
  fd = open(test_file, O_CREAT|O_RDWR, PR_SCOREBOARD_MODE);
  if (fd < 0) {
    return;
  }

  memset(&sch, 0, sizeof(sch));
  (void) write(fd, &sch, sizeof(sch));
  (void) close(fd);
  res = pr_open_scoreboard(O_RDWR);
  ck_assert_msg(res == PR_SCORE_ERR_BAD_MAGIC,
    "Failed to handle invalid header magic; expected %d, got %d",
    PR_SCORE_ERR_BAD_MAGIC, res);
  (void) unlink(test_file);

  /* Version too old */
  mark_point();
  fd = open(test_file, O_CREAT|O_RDWR, PR_SCOREBOARD_MODE);
  if (fd < 0) {
    return;
  }

  memset(&sch, 0, sizeof(sch));
  sch.sch_magic = PR_SCOREBOARD_MAGIC;
  sch.sch_version = 1;
  (void) write(fd, &sch, sizeof(sch));
  (void) close(fd);
  res = pr_open_scoreboard(O_RDWR);
  ck_assert_msg(res == PR_SCORE_ERR_OLDER_VERSION,
    "Failed to handle too-old header version; expected %d, got %d",
    PR_SCORE_ERR_OLDER_VERSION, res);
  (void) unlink(test_file);

  /* Version too new */
  mark_point();
  fd = open(test_file, O_CREAT|O_RDWR, PR_SCOREBOARD_MODE);
  if (fd < 0) {
    return;
  }

  memset(&sch, 0, sizeof(sch));
  sch.sch_magic = PR_SCOREBOARD_MAGIC;
  sch.sch_version = PR_SCOREBOARD_VERSION + 1;
  (void) write(fd, &sch, sizeof(sch));
  (void) close(fd);
  res = pr_open_scoreboard(O_RDWR);
  ck_assert_msg(res == PR_SCORE_ERR_NEWER_VERSION,
    "Failed to handle too-new header version; expected %d, got %d",
    PR_SCORE_ERR_NEWER_VERSION, res);
  (void) unlink(test_file);

  (void) unlink(test_mutex);
  (void) rmdir(test_dir);
}
END_TEST

START_TEST (scoreboard_lock_test) {
  int fd = -1, lock_type = -1, res;

  res = pr_lock_scoreboard(fd, lock_type);
  ck_assert_msg(res < 0, "Failed to handle bad lock type");
  ck_assert_msg(errno == EINVAL, "Expected EINVAL (%d), got %s (%d)", EINVAL,
    strerror(errno), errno);

  lock_type = F_RDLCK;
  res = pr_lock_scoreboard(fd, lock_type);
  ck_assert_msg(res < 0, "Failed to handle bad file descriptor");
  ck_assert_msg(errno == EBADF, "Expected EBADF (%d), got %s (%d)", EBADF,
    strerror(errno), errno);

  fd = open(test_file2, O_CREAT|O_EXCL|O_RDWR, S_IRUSR|S_IWUSR);
  ck_assert_msg(fd >= 0, "Failed to open '%s': %s", test_file2, strerror(errno));

  res = pr_lock_scoreboard(fd, lock_type);
  ck_assert_msg(res == 0, "Failed to lock fd %d: %s", fd, strerror(errno));

  lock_type = F_WRLCK;
  res = pr_lock_scoreboard(fd, lock_type);
  ck_assert_msg(res == 0, "Failed to lock fd %d: %s", fd, strerror(errno));

  lock_type = F_UNLCK;
  res = pr_lock_scoreboard(fd, lock_type);
  ck_assert_msg(res == 0, "Failed to lock fd %d: %s", fd, strerror(errno));

  lock_type = F_WRLCK;
  res = pr_lock_scoreboard(fd, lock_type);
  ck_assert_msg(res == 0, "Failed to lock fd %d: %s", fd, strerror(errno));

  /* Note: apparently attempt to lock (again) a file on which a lock
   * (of the same type) is already held will succeed.  Huh.
   */
  res = pr_lock_scoreboard(fd, lock_type);
  ck_assert_msg(res == 0, "Failed to lock fd %d: %s", fd, strerror(errno));

  lock_type = F_RDLCK;
  res = pr_lock_scoreboard(fd, lock_type);
  ck_assert_msg(res == 0, "Failed to lock fd %d: %s", fd, strerror(errno));

  lock_type = F_UNLCK;
  res = pr_lock_scoreboard(fd, lock_type);
  ck_assert_msg(res == 0, "Failed to lock fd %d: %s", fd, strerror(errno));

  (void) unlink(test_file2);
}
END_TEST

START_TEST (scoreboard_delete_test) {
  int res;
  struct stat st;

  res = mkdir(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to create directory '%s': %s", test_dir,
    strerror(errno));

  res = chmod(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to set perms on '%s' to 0775': %s", test_dir,
    strerror(errno));

  res = pr_set_scoreboard(test_file);
  ck_assert_msg(res == 0, "Failed to set scoreboard to '%s': %s", test_file,
    strerror(errno));

  res = pr_open_scoreboard(O_RDWR);
  ck_assert_msg(res == 0, "Failed to open scoreboard: %s", strerror(errno));

  res = stat(pr_get_scoreboard(), &st);
  ck_assert_msg(res == 0, "Failed to stat scoreboard: %s", strerror(errno));

  pr_delete_scoreboard();

  res = stat(pr_get_scoreboard(), &st);
  ck_assert_msg(res < 0, "Unexpectedly found deleted scoreboard");

  res = stat(pr_get_scoreboard_mutex(), &st);
  ck_assert_msg(res < 0, "Unexpectedly found deleted scoreboard mutex");

  (void) unlink(test_mutex);
  (void) unlink(test_file);
  (void) rmdir(test_dir);
}
END_TEST

START_TEST (scoreboard_restore_test) {
  int res;
  const char *path;

  res = mkdir(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to create directory '%s': %s", test_dir,
    strerror(errno));

  res = chmod(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to set perms on '%s' to 0775': %s", test_dir,
    strerror(errno));

  res = pr_set_scoreboard(test_file);
  ck_assert_msg(res == 0, "Failed to set scoreboard to '%s': %s", test_file,
    strerror(errno));

  mark_point();
  res = pr_restore_scoreboard();
  ck_assert_msg(res < 0, "Unexpectedly restored scoreboard");
  ck_assert_msg(errno == EINVAL, "Expected EINVAL (%d), got %s (%d)", EINVAL,
    strerror(errno), errno);

  res = pr_open_scoreboard(O_RDWR);
  ck_assert_msg(res == 0, "Failed to open scoreboard: %s", strerror(errno));

  mark_point();
  res = pr_restore_scoreboard();
  ck_assert_msg(res < 0,
    "Restoring scoreboard before rewind succeeded unexpectedly");

  mark_point();
  res = pr_rewind_scoreboard();
  ck_assert_msg(res == 0, "Failed to rewind scoreboard: %s", strerror(errno));
  res = pr_restore_scoreboard();
  ck_assert_msg(res == 0, "Failed to restore scoreboard: %s", strerror(errno));

  mark_point();
  path = pr_get_scoreboard();
  res = pr_set_scoreboard("none");
  ck_assert_msg(res == 0, "Failed to disable scoreboarding: %s", strerror(errno));
  res = pr_restore_scoreboard();
  ck_assert_msg(res == 0, "Failed to restore scoreboard: %s", strerror(errno));
  res = pr_set_scoreboard(path);
  ck_assert_msg(res == 0, "Failed to enable scoreboarding: %s", strerror(errno));

  (void) unlink(test_mutex);
  (void) unlink(test_file);
  (void) rmdir(test_dir);
}
END_TEST

START_TEST (scoreboard_rewind_test) {
  int res;
  const char *path;

  res = mkdir(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to create directory '%s': %s", test_dir,
    strerror(errno));

  res = chmod(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to set perms on '%s' to 0775': %s", test_dir,
    strerror(errno));

  mark_point();
  res = pr_set_scoreboard(test_file);
  ck_assert_msg(res == 0, "Failed to set scoreboard to '%s': %s", test_file,
    strerror(errno));

  res = pr_rewind_scoreboard();
  ck_assert_msg(res < 0, "Unexpectedly rewound scoreboard");
  ck_assert_msg(errno == EINVAL, "Expected EINVAL (%d), got %s (%d)", EINVAL,
    strerror(errno), errno);

  mark_point();
  res = pr_open_scoreboard(O_RDWR);
  ck_assert_msg(res == 0, "Failed to open scoreboard: %s", strerror(errno));
  res = pr_rewind_scoreboard();
  ck_assert_msg(res == 0, "Failed to rewind scoreboard: %s", strerror(errno));

  mark_point();
  path = pr_get_scoreboard();
  res = pr_set_scoreboard("none");
  ck_assert_msg(res == 0, "Failed to disable scoreboarding: %s", strerror(errno));
  res = pr_rewind_scoreboard();
  ck_assert_msg(res == 0, "Failed to rewind scoreboard: %s", strerror(errno));
  res = pr_set_scoreboard(path);
  ck_assert_msg(res == 0, "Failed to enable scoreboarding: %s", strerror(errno));

  (void) unlink(test_mutex);
  (void) unlink(test_file);
  (void) rmdir(test_dir);
}
END_TEST

START_TEST (scoreboard_scrub_test) {
  uid_t euid;
  int res;

  res = mkdir(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to create directory '%s': %s", test_dir,
    strerror(errno));

  res = chmod(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to set perms on '%s' to 0775': %s", test_dir,
    strerror(errno));

  res = pr_set_scoreboard(test_file);
  ck_assert_msg(res == 0, "Failed to set scoreboard to '%s': %s", test_file,
    strerror(errno));

  res = pr_scoreboard_scrub();
  ck_assert_msg(res < 0, "Unexpectedly scrubbed scoreboard");

  euid = geteuid();
  if (euid != 0) {
    if (errno != EPERM &&
        errno != ENOENT) {
      ck_abort_msg("Failed to set errno to EPERM/ENOENT, got %d [%s] (euid = %lu)",
        errno, strerror(errno), (unsigned long) euid);
    }

  } else {
    if (errno != ENOENT) {
      ck_abort_msg("Failed to set errno to ENOENT, got %d [%s] (euid = %lu)", errno,
        strerror(errno), (unsigned long) euid);
    }
  }

  res = pr_open_scoreboard(O_RDWR);
  ck_assert_msg(res == 0, "Failed to open scoreboard: %s", strerror(errno));

  res = pr_scoreboard_scrub();
  ck_assert_msg(res == 0, "Failed to scrub scoreboard: %s", strerror(errno));

  (void) unlink(test_mutex);
  (void) unlink(test_file);
  (void) rmdir(test_dir);
}
END_TEST

START_TEST (scoreboard_get_daemon_pid_test) {
  int res;
  pid_t daemon_pid;

  res = mkdir(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to create directory '%s': %s", test_dir,
    strerror(errno));

  res = chmod(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to set perms on '%s' to 0775': %s", test_dir,
    strerror(errno));

  res = pr_set_scoreboard(test_file);
  ck_assert_msg(res == 0, "Failed to set scoreboard to '%s': %s", test_file,
    strerror(errno));

  res = pr_open_scoreboard(O_RDWR);
  ck_assert_msg(res == 0, "Failed to open scoreboard: %s", strerror(errno));

  daemon_pid = pr_scoreboard_get_daemon_pid();
  if (daemon_pid != getpid()) {
    ck_abort_msg("Expected %lu, got %lu", (unsigned long) getpid(),
      (unsigned long) daemon_pid);
  }

  pr_delete_scoreboard();

  ServerType = SERVER_INETD;

  res = pr_open_scoreboard(O_RDWR);
  ck_assert_msg(res == 0, "Failed to open scoreboard: %s", strerror(errno));

  daemon_pid = pr_scoreboard_get_daemon_pid();
  if (daemon_pid != 0) {
    ck_abort_msg("Expected %lu, got %lu", (unsigned long) 0,
      (unsigned long) daemon_pid);
  }

  (void) unlink(test_mutex);
  (void) unlink(test_file);
  (void) rmdir(test_dir);
}
END_TEST

START_TEST (scoreboard_get_daemon_uptime_test) {
  int res;
  time_t daemon_uptime, now;

  res = mkdir(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to create directory '%s': %s", test_dir,
    strerror(errno));

  res = chmod(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to set perms on '%s' to 0775': %s", test_dir,
    strerror(errno));

  res = pr_set_scoreboard(test_file);
  ck_assert_msg(res == 0, "Failed to set scoreboard to '%s': %s", test_file,
    strerror(errno));

  res = pr_open_scoreboard(O_RDWR);
  ck_assert_msg(res == 0, "Failed to open scoreboard: %s", strerror(errno));

  daemon_uptime = pr_scoreboard_get_daemon_uptime();
  now = time(NULL);

  if (daemon_uptime > now) {
    ck_abort_msg("Expected %lu, got %lu", (unsigned long) now,
      (unsigned long) daemon_uptime);
  }

  pr_delete_scoreboard();

  ServerType = SERVER_INETD;

  res = pr_open_scoreboard(O_RDWR);
  ck_assert_msg(res == 0, "Failed to open scoreboard: %s", strerror(errno));

  daemon_uptime = pr_scoreboard_get_daemon_uptime();
  if (daemon_uptime != 0) {
    ck_abort_msg("Expected %lu, got %lu", (unsigned long) 0,
      (unsigned long) daemon_uptime);
  }

  (void) unlink(test_mutex);
  (void) unlink(test_file);
  (void) rmdir(test_dir);
}
END_TEST

START_TEST (scoreboard_entry_add_test) {
  int res;

  res = mkdir(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to create directory '%s': %s", test_dir,
    strerror(errno));

  res = chmod(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to set perms on '%s' to 0775': %s", test_dir,
    strerror(errno));

  res = pr_set_scoreboard(test_file);
  ck_assert_msg(res == 0, "Failed to set scoreboard to '%s': %s", test_file,
    strerror(errno));

  res = pr_scoreboard_entry_add();
  ck_assert_msg(res < 0, "Unexpectedly added entry to scoreboard");
  ck_assert_msg(errno == EINVAL, "Expected EINVAL (%d), got %s (%d)", EINVAL,
    strerror(errno), errno);

  res = pr_open_scoreboard(O_RDWR);
  ck_assert_msg(res == 0, "Failed to open scoreboard: %s", strerror(errno));

  res = pr_scoreboard_entry_add();
  ck_assert_msg(res == 0, "Failed to add entry to scoreboard: %s",
    strerror(errno));

  res = pr_scoreboard_entry_add();
  ck_assert_msg(res < 0, "Unexpectedly added entry to scoreboard");
  ck_assert_msg(errno == EPERM, "Expected EPERM (%d), got %s (%d)", EPERM,
    strerror(errno), errno);

  (void) unlink(test_mutex);
  (void) unlink(test_file);
  (void) rmdir(test_dir);
}
END_TEST

START_TEST (scoreboard_entry_del_test) {
  int res;

  res = mkdir(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to create directory '%s': %s", test_dir,
    strerror(errno));

  res = chmod(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to set perms on '%s' to 0775': %s", test_dir,
    strerror(errno));

  res = pr_set_scoreboard(test_file);
  ck_assert_msg(res == 0, "Failed to set scoreboard to '%s': %s", test_file,
    strerror(errno));

  res = pr_scoreboard_entry_del(FALSE);
  ck_assert_msg(res < 0, "Unexpectedly deleted entry from scoreboard");
  ck_assert_msg(errno == EINVAL, "Expected EINVAL (%d), got %s (%d)", EINVAL,
    strerror(errno), errno);

  res = pr_open_scoreboard(O_RDWR);
  ck_assert_msg(res == 0, "Failed to open scoreboard: %s", strerror(errno));

  res = pr_scoreboard_entry_del(FALSE);
  ck_assert_msg(res < 0, "Unexpectedly deleted entry from scoreboard");
  ck_assert_msg(errno == ENOENT, "Expected ENOENT (%d), got %s (%d)", ENOENT,
    strerror(errno), errno);

  res = pr_scoreboard_entry_add();
  ck_assert_msg(res == 0, "Failed to add entry to scoreboard: %s",
    strerror(errno));

  res = pr_scoreboard_entry_del(FALSE);
  ck_assert_msg(res == 0, "Failed to delete entry from scoreboard: %s",
    strerror(errno));

  res = pr_scoreboard_entry_del(FALSE);
  ck_assert_msg(res < 0, "Unexpectedly deleted entry from scoreboard");
  ck_assert_msg(errno == ENOENT, "Expected ENOENT (%d), got %s (%d)", ENOENT,
    strerror(errno), errno);

  (void) unlink(test_mutex);
  (void) unlink(test_file);
  (void) rmdir(test_dir);
}
END_TEST

START_TEST (scoreboard_entry_read_test) {
  int res;
  pr_scoreboard_entry_t *score;

  res = mkdir(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to create directory '%s': %s", test_dir,
    strerror(errno));

  res = chmod(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to set perms on '%s' to 0775': %s", test_dir,
    strerror(errno));

  res = pr_set_scoreboard(test_file);
  ck_assert_msg(res == 0, "Failed to set scoreboard to '%s': %s", test_file,
    strerror(errno));

  score = pr_scoreboard_entry_read();
  ck_assert_msg(score == NULL, "Unexpectedly read scoreboard entry");
  ck_assert_msg(errno == EINVAL, "Expected EINVAL (%d), got %s (%d)", EINVAL,
    strerror(errno), errno);

  res = pr_open_scoreboard(O_RDWR);
  ck_assert_msg(res == 0, "Failed to open scoreboard: %s", strerror(errno));

  /* We expect NULL here because the scoreboard file should be empty. */
  score = pr_scoreboard_entry_read();
  ck_assert_msg(score == NULL, "Unexpectedly read scoreboard entry");

  res = pr_scoreboard_entry_add();
  ck_assert_msg(res == 0, "Failed to add entry to scoreboard: %s",
    strerror(errno));

  score = pr_scoreboard_entry_read();
  ck_assert_msg(score != NULL, "Failed to read scoreboard entry: %s",
    strerror(errno));

  if (score->sce_pid != getpid()) {
    ck_abort_msg("Failed to read expected scoreboard entry (expected PID %lu, got %lu)",
      (unsigned long) getpid(), (unsigned long) score->sce_pid);
  }

  score = pr_scoreboard_entry_read();
  ck_assert_msg(score == NULL, "Unexpectedly read scoreboard entry");

  (void) unlink(test_mutex);
  (void) unlink(test_file);
  (void) rmdir(test_dir);
}
END_TEST

START_TEST (scoreboard_entry_get_test) {
  register unsigned int i;
  int res;
  const char *val;
  int scoreboard_fields[] = {
    PR_SCORE_USER,
    PR_SCORE_CLIENT_ADDR,
    PR_SCORE_CLIENT_NAME,
    PR_SCORE_CLASS,
    PR_SCORE_CWD,
    PR_SCORE_CMD,
    PR_SCORE_CMD_ARG,
    PR_SCORE_PROTOCOL,
    -1
  };

  res = mkdir(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to create directory '%s': %s", test_dir,
    strerror(errno));

  res = chmod(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to set perms on '%s' to 0775': %s", test_dir,
    strerror(errno));

  res = pr_set_scoreboard(test_file);
  ck_assert_msg(res == 0, "Failed to set scoreboard to '%s': %s", test_file,
    strerror(errno));

  val = pr_scoreboard_entry_get(-1);
  ck_assert_msg(val == NULL, "Unexpectedly read value from scoreboard entry");
  ck_assert_msg(errno == EINVAL, "Expected EINVAL (%d), got %s (%d)", EINVAL,
    strerror(errno), errno);

  res = pr_open_scoreboard(O_RDWR);
  ck_assert_msg(res == 0, "Failed to open scoreboard: %s", strerror(errno));

  val = pr_scoreboard_entry_get(-1);
  ck_assert_msg(val == NULL, "Unexpectedly read value from scoreboard entry");
  ck_assert_msg(errno == EPERM, "Expected EPERM (%d), got %s (%d)", EPERM,
    strerror(errno), errno);

  res = pr_scoreboard_entry_add();
  ck_assert_msg(res == 0, "Failed to add entry to scoreboard: %s",
    strerror(errno));

  val = pr_scoreboard_entry_get(-1);
  ck_assert_msg(val == NULL, "Unexpectedly read value from scoreboard entry");
  ck_assert_msg(errno == ENOENT, "Expected ENOENT (%d), got %s (%d)", ENOENT,
    strerror(errno), errno);

  for (i = 0; scoreboard_fields[i] != -1; i++) {
    val = pr_scoreboard_entry_get(scoreboard_fields[i]);
    ck_assert_msg(val != NULL, "Failed to read scoreboard field %d: %s",
      scoreboard_fields[i], strerror(errno));
  }

  (void) unlink(test_mutex);
  (void) unlink(test_file);
  (void) rmdir(test_dir);
}
END_TEST

START_TEST (scoreboard_entry_update_test) {
  int num, res;
  const char *val;
  pid_t pid = getpid();
  const pr_netaddr_t *addr;
  time_t now;
  off_t len;
  unsigned long elapsed;

  res = mkdir(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to create directory '%s': %s", test_dir,
    strerror(errno));

  res = chmod(test_dir, 0775);
  ck_assert_msg(res == 0, "Failed to set perms on '%s' to 0775': %s", test_dir,
    strerror(errno));

  res = pr_set_scoreboard(test_file);
  ck_assert_msg(res == 0, "Failed to set scoreboard to '%s': %s", test_file,
    strerror(errno));

  res = pr_scoreboard_entry_update(pid, 0);
  ck_assert_msg(res < 0, "Unexpectedly updated scoreboard entry");
  ck_assert_msg(errno == EINVAL, "Expected EINVAL (%d), got %s (%d)", EINVAL,
    strerror(errno), errno);

  res = pr_open_scoreboard(O_RDWR);
  ck_assert_msg(res == 0, "Failed to open scoreboard: %s", strerror(errno));

  res = pr_scoreboard_entry_update(pid, 0);
  ck_assert_msg(res < 0, "Unexpectedly updated scoreboard entry");
  ck_assert_msg(errno == EPERM, "Expected EPERM (%d), got %s (%d)", EPERM,
    strerror(errno), errno);

  res = pr_scoreboard_entry_add();
  ck_assert_msg(res == 0, "Failed to add entry to scoreboard: %s",
    strerror(errno));

  res = pr_scoreboard_entry_update(pid, -1);
  ck_assert_msg(res < 0, "Unexpectedly updated scoreboard entry");
  ck_assert_msg(errno == ENOENT, "Expected ENOENT (%d), got %s (%d)", ENOENT,
    strerror(errno), errno);

  val = "cwd";
  res = pr_scoreboard_entry_update(pid, PR_SCORE_CWD, val, NULL);
  ck_assert_msg(res == 0, "Failed to update PR_SCORE_CWD: %s", strerror(errno));

  val = pr_scoreboard_entry_get(PR_SCORE_CWD);
  ck_assert_msg(val != NULL, "Failed to get entry PR_SCORE_CWD: %s",
    strerror(errno));
  ck_assert_msg(strcmp(val, "cwd") == 0, "Expected 'cwd', got '%s'", val);

  val = "user";
  res = pr_scoreboard_entry_update(pid, PR_SCORE_USER, val, NULL);
  ck_assert_msg(res == 0, "Failed to update PR_SCORE_USER: %s", strerror(errno));

  addr = pr_netaddr_get_addr(p, "127.0.0.1", NULL);
  ck_assert_msg(addr != NULL, "Failed to resolve '127.0.0.1': %s",
    strerror(errno));

  res = pr_scoreboard_entry_update(pid, PR_SCORE_CLIENT_ADDR, addr, NULL);
  ck_assert_msg(res == 0, "Failed to update PR_SCORE_CLIENT_ADDR: %s",
    strerror(errno));

  val = "remote_name";
  res = pr_scoreboard_entry_update(pid, PR_SCORE_CLIENT_NAME, val, NULL);
  ck_assert_msg(res == 0, "Failed to update PR_SCORE_CLIENT_NAME: %s",
    strerror(errno));

  val = "session_class";
  res = pr_scoreboard_entry_update(pid, PR_SCORE_CLASS, val, NULL);
  ck_assert_msg(res == 0, "Failed to update PR_SCORE_CLASS: %s", strerror(errno));

  val = "USER";
  res = pr_scoreboard_entry_update(pid, PR_SCORE_CMD, "%s", val, NULL, NULL);
  ck_assert_msg(res == 0, "Failed to update PR_SCORE_CMD: %s", strerror(errno));

  val = "foo bar";
  res = pr_scoreboard_entry_update(pid, PR_SCORE_CMD_ARG, "%s", val, NULL,
    NULL);
  ck_assert_msg(res == 0, "Failed to update PR_SCORE_CMD_ARG: %s",
    strerror(errno));

  num = 77;
  res = pr_scoreboard_entry_update(pid, PR_SCORE_SERVER_PORT, num, NULL);
  ck_assert_msg(res == 0, "Failed to update PR_SCORE_SERVER_PORT: %s",
    strerror(errno));

  res = pr_scoreboard_entry_update(pid, PR_SCORE_SERVER_ADDR, addr, num, NULL);
  ck_assert_msg(res == 0, "Failed to update PR_SCORE_SERVER_ADDR: %s",
    strerror(errno));

  val = "label";
  res = pr_scoreboard_entry_update(pid, PR_SCORE_SERVER_LABEL, val, NULL);
  ck_assert_msg(res == 0, "Failed to update PR_SCORE_SERVER_LABEL: %s",
    strerror(errno));

  now = 1;
  res = pr_scoreboard_entry_update(pid, PR_SCORE_BEGIN_IDLE, now, NULL);
  ck_assert_msg(res == 0, "Failed to update PR_SCORE_BEGIN_IDLE: %s",
    strerror(errno));

  now = 2;
  res = pr_scoreboard_entry_update(pid, PR_SCORE_BEGIN_SESSION, now, NULL);
  ck_assert_msg(res == 0, "Failed to update PR_SCORE_BEGIN_SESSION: %s",
    strerror(errno));

  len = 7;
  res = pr_scoreboard_entry_update(pid, PR_SCORE_XFER_DONE, len, NULL);
  ck_assert_msg(res == 0, "Failed to update PR_SCORE_XFER_DONE: %s",
    strerror(errno));

  len = 8;
  res = pr_scoreboard_entry_update(pid, PR_SCORE_XFER_SIZE, len, NULL);
  ck_assert_msg(res == 0, "Failed to update PR_SCORE_XFER_SIZE: %s",
    strerror(errno));

  len = 9;
  res = pr_scoreboard_entry_update(pid, PR_SCORE_XFER_LEN, len, NULL);
  ck_assert_msg(res == 0, "Failed to update PR_SCORE_XFER_LEN: %s",
    strerror(errno));

  elapsed = 1;
  res = pr_scoreboard_entry_update(pid, PR_SCORE_XFER_ELAPSED, elapsed, NULL);
  ck_assert_msg(res == 0, "Failed to update PR_SCORE_XFER_ELAPSED: %s",
    strerror(errno));

  val = "protocol";
  res = pr_scoreboard_entry_update(pid, PR_SCORE_PROTOCOL, val, NULL);
  ck_assert_msg(res == 0, "Failed to update PR_SCORE_PROTOCOL: %s",
    strerror(errno));

  (void) unlink(test_mutex);
  (void) unlink(test_file);
  (void) rmdir(test_dir);
}
END_TEST

START_TEST (scoreboard_entry_kill_test) {
  int res;
  pr_scoreboard_entry_t sce;

  mark_point();
  res = pr_scoreboard_entry_kill(NULL, 0);
  ck_assert_msg(res < 0, "Failed to handle null arguments");
  ck_assert_msg(errno == EINVAL, "Expected EINVAL (%d), got %s (%d)", EINVAL,
    strerror(errno), errno);

  mark_point();
  sce.sce_pid = 1;
  res = pr_scoreboard_entry_kill(&sce, 0);
  ck_assert_msg(res < 0, "Failed to handle bad PID");
  ck_assert_msg(errno == EPERM, "Expected EPERM (%d), got %s (%d)", EPERM,
    strerror(errno), errno);

  mark_point();
  sce.sce_pid = getpid();
  res = pr_scoreboard_entry_kill(&sce, 0);
  ck_assert_msg(res == 0, "Failed to send signal 0 to PID %lu: %s",
    (unsigned long) sce.sce_pid, strerror(errno));

  mark_point();
  res = pr_set_scoreboard("none");
  ck_assert_msg(res == 0, "Failed to disable scoreboarding: %s", strerror(errno));
  sce.sce_pid = getpid();
  res = pr_scoreboard_entry_kill(&sce, 0);
  ck_assert_msg(res == 0, "Failed to send signal: %s", strerror(errno));
}
END_TEST

START_TEST (scoreboard_entry_lock_test) {
  int fd = -1, lock_type = -1, res;

  res = pr_scoreboard_entry_lock(fd, lock_type);
  ck_assert_msg(res < 0, "Failed to handle bad lock type");
  ck_assert_msg(errno == EINVAL, "Expected EINVAL (%d), got %s (%d)", EINVAL,
    strerror(errno), errno);

  lock_type = F_RDLCK;
  res = pr_scoreboard_entry_lock(fd, lock_type);
  ck_assert_msg(res < 0, "Failed to handle bad file descriptor");
  ck_assert_msg(errno == EBADF, "Expected EBADF (%d), got %s (%d)", EBADF,
    strerror(errno), errno);

  fd = open(test_file2, O_CREAT|O_EXCL|O_RDWR, S_IRUSR|S_IWUSR);
  ck_assert_msg(fd >= 0, "Failed to open '%s': %s", test_file2, strerror(errno));

  res = pr_scoreboard_entry_lock(fd, lock_type);
  ck_assert_msg(res == 0, "Failed to lock fd %d: %s", fd, strerror(errno));

  lock_type = F_WRLCK;
  res = pr_scoreboard_entry_lock(fd, lock_type);
  ck_assert_msg(res == 0, "Failed to lock fd %d: %s", fd, strerror(errno));

  lock_type = F_UNLCK;
  res = pr_scoreboard_entry_lock(fd, lock_type);
  ck_assert_msg(res == 0, "Failed to lock fd %d: %s", fd, strerror(errno));

  lock_type = F_WRLCK;
  res = pr_scoreboard_entry_lock(fd, lock_type);
  ck_assert_msg(res == 0, "Failed to lock fd %d: %s", fd, strerror(errno));

  /* Note: apparently attempt to lock (again) a file on which a lock
   * (of the same type) is already held will succeed.  Huh.
   */
  res = pr_scoreboard_entry_lock(fd, lock_type);
  ck_assert_msg(res == 0, "Failed to lock fd %d: %s", fd, strerror(errno));

  lock_type = F_RDLCK;
  res = pr_scoreboard_entry_lock(fd, lock_type);
  ck_assert_msg(res == 0, "Failed to lock fd %d: %s", fd, strerror(errno));

  lock_type = F_UNLCK;
  res = pr_scoreboard_entry_lock(fd, lock_type);
  ck_assert_msg(res == 0, "Failed to lock fd %d: %s", fd, strerror(errno));

  (void) unlink(test_file2);
}
END_TEST

START_TEST (scoreboard_disabled_test) {
  register unsigned int i = 0;
  const char *paths[4] = {
    "/dev/null",
    "none",
    "off",
    NULL
  };
  const char *path;

  for (path = paths[i]; path != NULL; path = paths[i++]) {
    int res;
    const char *field, *ok;
    pid_t scoreboard_pid;
    time_t scoreboard_uptime;
    pr_scoreboard_entry_t *score;

    res = pr_set_scoreboard(path);
    ck_assert_msg(res == 0, "Failed set to scoreboard to '%s': %s", path,
      strerror(errno));

    ok = PR_RUN_DIR "/proftpd.scoreboard";

    path = pr_get_scoreboard();
    ck_assert_msg(path != NULL, "Failed to get scoreboard path: %s",
      strerror(errno));
    ck_assert_msg(strcmp(path, ok) == 0,
      "Expected path '%s', got '%s'", ok, path);

    res = pr_open_scoreboard(O_RDONLY);
    ck_assert_msg(res == 0, "Failed to open '%s' scoreboard: %s", path,
      strerror(errno));

    res = pr_scoreboard_scrub();
    ck_assert_msg(res == 0, "Failed to scrub '%s' scoreboard: %s", path,
      strerror(errno));

    scoreboard_pid = pr_scoreboard_get_daemon_pid();
    ck_assert_msg(scoreboard_pid == 0,
      "Expected to get scoreboard PID 0, got %lu",
      (unsigned long) scoreboard_pid);

    scoreboard_uptime = pr_scoreboard_get_daemon_uptime();
    ck_assert_msg(scoreboard_uptime == 0,
      "Expected to get scoreboard uptime 0, got %lu",
      (unsigned long) scoreboard_uptime);

    res = pr_scoreboard_entry_add();
    ck_assert_msg(res == 0, "Failed to add entry to '%s' scoreboard: %s", path,
      strerror(errno));

    score = pr_scoreboard_entry_read();
    ck_assert_msg(score == NULL, "Expected null entry");

    field = pr_scoreboard_entry_get(PR_SCORE_CMD_ARG);
    ck_assert_msg(field == NULL, "Expected null CMD_ARG field");

    res = pr_scoreboard_entry_update(getpid(), PR_SCORE_CWD, "foo", NULL);
    ck_assert_msg(res == 0, "Failed to update CWD field: %s", strerror(errno));

    res = pr_scoreboard_entry_del(FALSE);
    ck_assert_msg(res == 0, "Failed to delete entry from '%s' scoreboard: %s",
      path, strerror(errno));

    res = pr_close_scoreboard(FALSE);
    ck_assert_msg(res == 0, "Failed to close '%s' scoreboard: %s", path,
      strerror(errno));

    /* Internal hack: even calling pr_set_scoreboard() with a NULL
     * argument will set the Scoreboard API internal flag back to true.
     */
    pr_set_scoreboard(NULL);
  }
}
END_TEST

Suite *tests_get_scoreboard_suite(void) {
  Suite *suite;
  TCase *testcase;

  suite = suite_create("scoreboard");

  testcase = tcase_create("base");
  tcase_add_checked_fixture(testcase, set_up, tear_down);

  tcase_add_test(testcase, scoreboard_get_test);
  tcase_add_test(testcase, scoreboard_set_test);
  tcase_add_test(testcase, scoreboard_set_mutex_test);
  tcase_add_test(testcase, scoreboard_open_close_test);
  tcase_add_test(testcase, scoreboard_open_invalid_header_test);
  tcase_add_test(testcase, scoreboard_lock_test);
  tcase_add_test(testcase, scoreboard_delete_test);
  tcase_add_test(testcase, scoreboard_restore_test);
  tcase_add_test(testcase, scoreboard_rewind_test);
  tcase_add_test(testcase, scoreboard_scrub_test);
  tcase_add_test(testcase, scoreboard_get_daemon_pid_test);
  tcase_add_test(testcase, scoreboard_get_daemon_uptime_test);
  tcase_add_test(testcase, scoreboard_entry_add_test);
  tcase_add_test(testcase, scoreboard_entry_del_test);
  tcase_add_test(testcase, scoreboard_entry_read_test);
  tcase_add_test(testcase, scoreboard_entry_get_test);
  tcase_add_test(testcase, scoreboard_entry_update_test);
  tcase_add_test(testcase, scoreboard_entry_kill_test);
  tcase_add_test(testcase, scoreboard_entry_lock_test);
  tcase_add_test(testcase, scoreboard_disabled_test);

  suite_add_tcase(suite, testcase);
  return suite;
}
