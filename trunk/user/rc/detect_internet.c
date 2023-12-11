/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>

#include "rc.h"

#define DI_STATUS_INIT	2
#define DI_PID_FILE	"/var/run/detect_internet.pid"

static int                   di_poll_mode;
static int                   glob_internet;
static volatile sig_atomic_t link_internet = DI_STATUS_INIT;
static volatile sig_atomic_t link_Previous = DI_STATUS_INIT;
static volatile sig_atomic_t di_pause_received = 0;
static volatile sig_atomic_t di_wait_webuiopen = 0;
static volatile sig_atomic_t di_notify_changed = 0;
static volatile sig_atomic_t di_lostfounddelay = 0;
static volatile sig_atomic_t di_script_running = 0;
static struct   itimerval    di_itv;

static void
di_alarmtimer(unsigned long sec)
{
	di_itv.it_value.tv_sec  = sec;
	di_itv.it_value.tv_usec = 0;
	di_itv.it_interval = di_itv.it_value;
	setitimer(ITIMER_REAL, &di_itv, NULL);
}

static int
di_run_script(int di_script_ontimer)
{
	di_script_running = 1;

	if (set_led_wan(3, 3, 0) == link_internet) {
		const char *di_script_1 = SCRIPT_DETECTINTERNET_A;
		const char *di_script_2 = SCRIPT_DETECTINTERNET_B;
		const char *di_script_3 = SCRIPT_DETECTINTERNET_C;
		const char *di_script_4 = SCRIPT_DETECTINTERNET_D;
		link_internet = DI_STATUS_INIT;
		glob_internet = DI_STATUS_INIT;
		if (check_if_file_exist(di_script_1) && check_if_file_exist(di_script_2) &&
		    check_if_file_exist(di_script_3) && check_if_file_exist(di_script_4)) {
			pid_t di_script_1_status;
			pid_t di_script_2_status;
			pid_t di_script_3_status;
			pid_t di_script_4_status;
			di_script_1_status = system(di_script_1);
			di_script_2_status = system(di_script_2);
			di_script_3_status = system(di_script_3);
			di_script_4_status = system(di_script_4);
			if (di_script_1_status != -1 && WIFEXITED(di_script_1_status) &&
			    di_script_2_status != -1 && WIFEXITED(di_script_2_status) &&
			    di_script_3_status != -1 && WIFEXITED(di_script_3_status) &&
			    di_script_4_status != -1 && WIFEXITED(di_script_4_status)) {
				if (WEXITSTATUS(di_script_2_status) == 0 || WEXITSTATUS(di_script_4_status) == 0) {
					link_internet = 1;
					glob_internet = 1;
				} else if (WEXITSTATUS(di_script_1_status) == 0 || WEXITSTATUS(di_script_3_status) == 0) {
					link_internet = 1;
					glob_internet = 0;
				} else {
					link_internet = 0;
					glob_internet = 0;
				}
			} else {
				logmessage("detect_internet", "detect_internet_script run error");
			}
		} else {
			logmessage("detect_internet", "detect_internet_script not found");
		}

		if (set_led_wan(link_internet, 3, 0) != link_internet)
			logmessage("detect_internet", "set led state error");

		if (nvram_get_int("global_internet") != glob_internet) {
			nvram_set_int_temp("global_internet", glob_internet);
#if defined (APP_SHADOWSOCKS)
			if (nvram_match("ss_enable", "1"))
				doSystem("echo %s > %s", "1", "/tmp/SSP/internetcd");
#endif
		}
	} else {
		link_internet = nvram_get_int("link_internet");
#if defined (APP_SHADOWSOCKS)
		if (nvram_match("ss_enable", "1"))
			doSystem("echo %s > %s", "1", "/tmp/SSP/internetcd");
#endif
	}

	if (di_script_ontimer == 0)
		di_script_running = 0;

	return link_internet;
}

static void
di_on_timer(void)
{
	int           di_all_timeout = nvram_safe_get_int("di_timeout", 2, 1, 8) * 3;
	unsigned long di_run_timeout = (unsigned long)di_all_timeout;
	unsigned long di_period_next = di_itv.it_value.tv_sec;

	if (di_lostfounddelay >= 1)
		--di_lostfounddelay;

	if (di_script_running == 0 && di_pause_received == 0) {
		if (di_run_script(1) != link_internet) {
			link_internet = nvram_get_int("link_internet");
			logmessage("detect_internet", "di_run_script() serious error");
		}
	} else if (di_script_running == 1 && di_pause_received == 0) {
		link_internet = nvram_get_int("link_internet");
		logmessage("detect_internet", "di_run_script() running error");
	}

	if (di_poll_mode == 0 && di_pause_received == 0) {
		if (di_notify_changed == 1 && link_internet != DI_STATUS_INIT && di_lostfounddelay == 0) {
			di_notify_changed = 0;
			if (link_internet != link_Previous) {
				link_Previous = link_internet;
				notify_on_internet_state_changed(link_internet);
			}
		}
		
		if (link_internet == 0 && di_lostfounddelay == 0) {
			if (link_internet != link_Previous) {
				di_notify_changed = 1;
				di_lostfounddelay = nvram_safe_get_int("di_lost_delay", 10, 1, 60);
				di_period_next = di_run_timeout;
			} else {
				di_period_next = (unsigned long)nvram_safe_get_int("di_time_fail", di_all_timeout, di_all_timeout, 60);
			}
		} else if (link_internet == 1 && di_lostfounddelay == 0) {
			if (link_internet != link_Previous) {
				di_notify_changed = 1;
				di_lostfounddelay = nvram_safe_get_int("di_found_delay", 1, 1, 6);
				di_period_next = di_run_timeout;
			} else {
				di_period_next = (unsigned long)nvram_safe_get_int("di_time_done", 300, 60, 600);
			}
		} else {
			di_period_next = di_run_timeout;
		}
	}

	if (di_poll_mode == 1 && di_pause_received == 0 && di_wait_webuiopen == 0) {
		di_wait_webuiopen = 1;
		di_period_next = 0;
	}

	if (di_pause_received == 0 && di_itv.it_value.tv_sec != di_period_next)
		di_alarmtimer(di_period_next);

	//logmessage("detect_internet", "link_Previous=%d link_internet=%d glob_internet=%d",
	//	link_Previous, link_internet, glob_internet);

	di_script_running = 0;
}

static void
di_on_sighup(void)
{
	if (di_poll_mode == 1 && di_pause_received == 0 && di_wait_webuiopen == 1) {
		di_wait_webuiopen = 0;
		di_alarmtimer(1);
	} else if (di_poll_mode == 0 && di_pause_received == 0 && di_script_running == 0) {
		di_run_script(0);
	}
}

static void
di_on_sigusr1(void)
{
	int delay_time;

	delay_time = nvram_get_int("di_notify_delay");
	if (delay_time < 1)
		delay_time = 1;

	di_poll_mode = nvram_safe_get_int("di_poll_mode", 0, 0, 1);
	if (di_poll_mode == 1) {
		di_pause_received = 0;
		di_wait_webuiopen = 0;
		di_notify_changed = 0;
		di_lostfounddelay = 0;
		di_alarmtimer(delay_time);
	} else if (di_poll_mode == 0 && di_notify_changed == 0) {
		di_pause_received = 0;
		di_wait_webuiopen = 0;
		di_lostfounddelay = 0;
		di_alarmtimer(delay_time);
	} else if (di_poll_mode == 0 && di_notify_changed == 1) {
		di_pause_received = 0;
		di_wait_webuiopen = 0;
		di_alarmtimer(delay_time);
	}
}

static void
di_on_sigusr2(void)
{
	di_pause_received = 1;
	di_alarmtimer(0);
}

static void
catch_sig_detect_internet(int sig)
{
	switch (sig)
	{
	case SIGALRM:
		di_on_timer();
		break;
	case SIGHUP:
		di_on_sighup();
		break;
	case SIGUSR1:
		di_on_sigusr1();
		break;
	case SIGUSR2:
		di_on_sigusr2();
		break;
	case SIGTERM:
		di_pause_received = 1;
		di_alarmtimer(0);
		remove(DI_PID_FILE);
		exit(0);
		break;
	}
}

void
stop_detect_internet(void)
{
	remove(DI_PID_FILE);
	doSystem("killall %s %s", "-q", "detect_internet");
}

int
start_detect_internet(int autorun_time)
{
	char arun[16];
	char *di_argv[] = {
		"/sbin/detect_internet",
		NULL,		/* -aX */
		NULL
	};

	stop_detect_internet();

	snprintf(arun, sizeof(arun), "-a%u", autorun_time);
	di_argv[1] = arun;

	return _eval(di_argv, NULL, 0, NULL);
}

void
notify_run_detect_internet(int delay_time)
{
	nvram_set_int_temp("di_notify_delay", delay_time);

	if (!pids("detect_internet"))
		start_detect_internet(delay_time);
	else
		kill_pidfile_s(DI_PID_FILE, SIGUSR1);
}

void
notify_pause_detect_internet(void)
{
	kill_pidfile_s(DI_PID_FILE, SIGUSR2);
}

int
detect_internet_main(int argc, char *argv[])
{
	FILE *fp;
	pid_t pid;
	int c, auto_run_time = 1;
	struct sigaction sa;

	// usage : detect_internet -a X
	if(argc) {
		while ((c = getopt(argc, argv, "a:")) != -1) {
			switch (c) {
				case 'a':
					auto_run_time = atoi(optarg);
				break;
			}
		}
	}

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = catch_sig_detect_internet;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGHUP);
	sigaddset(&sa.sa_mask, SIGUSR1);
	sigaddset(&sa.sa_mask, SIGUSR2);
	sigaddset(&sa.sa_mask, SIGALRM);
	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);
	sigaction(SIGALRM, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGPIPE, &sa, NULL);

	if (daemon(0, 0) < 0) {
		perror("daemon");
		exit(errno);
	}

	if (check_if_file_exist(DI_PID_FILE))
		exit(errno);

	pid = getpid();

	/* never invoke oom killer */
	oom_score_adjust(pid, OOM_SCORE_ADJ_MIN);

	/* write pid */
	if ((fp = fopen(DI_PID_FILE, "w")) != NULL) {
		fprintf(fp, "%d", pid);
		fclose(fp);
	}

	if (auto_run_time < 1)
		auto_run_time = 1;

	di_poll_mode = nvram_safe_get_int("di_poll_mode", 0, 0, 1);

	nvram_set_int_temp("di_notify_delay", 1);

	di_alarmtimer(auto_run_time);

	while (1) {
		pause();
	}

	return 0;
}

