#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
//#include <cpuid.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>

#define BLK "\e[0;30m"
#define RED "\e[0;31m"
#define GRN "\e[0;32m"
#define YEL "\e[0;33m"
#define BLU "\e[0;34m"
#define MAG "\e[0;35m"
#define CYN "\e[0;36m"
#define WHT "\e[0;37m"

#define BLKB "\e[40m"
#define REDB "\e[41m"
#define GRNB "\e[42m"
#define YELB "\e[43m"
#define BLUB "\e[44m"
#define MAGB "\e[45m"
#define CYNB "\e[46m"
#define WHTB "\e[47m"

#define COLOR_RESET "\e[0m"

struct cpuinfo {
	char name [256];
	float ghz;
};

struct fsinfo {
	float free_gbs;
	float total_gbs;
};

struct meminfo {
	float used_gbs;
	float total_gbs;
};

static const char* splash = ""
"     ____  ____  _  _  ____  _____ \n"
"    (  _ \\(_  _)( \\( )( ___)(  _  )\n"
"     )(_) )_)(_  )  (  )__)  )(_)( \n"
"    (____/(____)(_)\\_)(__)  (_____)\n";

int get_cpuinfo(struct cpuinfo* info);
int get_fsinfo(struct fsinfo* info);
int get_meminfo(struct meminfo* info);

int main(void) {
	printf("%s%s%s\n", CYN, splash, COLOR_RESET);

	struct utsname utsname;
	if (uname(&utsname) < 0) {
		fprintf(stderr, "%s[dinfo] Error: Failed to get system info%s\n", RED, COLOR_RESET);
		return 1;
	}

	printf("    %sSystem: %s%s %s %s%s\n", MAG, CYN, utsname.sysname, utsname.release, utsname.machine, COLOR_RESET);

	struct cpuinfo cpuinfo;
	if (get_cpuinfo(&cpuinfo) != 0) {
		fprintf(stderr, "%s[dinfo] Error: get_cpuinfo failed%s\n", RED, COLOR_RESET);
		return 1;
	}

	printf("    %sCPU: %s%s%s\n", MAG, CYN, cpuinfo.name, COLOR_RESET);
	printf("    %sCPU Max Clock Speed: %s%.2f GHz%s\n", MAG, CYN, cpuinfo.ghz, COLOR_RESET);

	struct meminfo meminfo;
	if (get_meminfo(&meminfo) != 0) {
		fprintf(stderr, "%s[dinfo] Error: get_meminfo failed%s\n", RED, COLOR_RESET);
		return 1;
	}

	printf("    %sMemory Usage: %s%.2f GB / %.2f GB%s\n", MAG, CYN, meminfo.used_gbs, meminfo.total_gbs, COLOR_RESET);

	struct fsinfo fsinfo;
	if (get_fsinfo(&fsinfo) != 0) {
		fprintf(stderr, "%s[dinfo] Error: get_fsinfo failed%s\n", RED, COLOR_RESET);
		return 1;
	}

	printf("    %sFilesystem: %s%.2f GB free of %.2f GB%s\n", MAG, CYN, fsinfo.free_gbs, fsinfo.total_gbs, COLOR_RESET);

	printf("    %s--------------------------------%s\n", CYN, COLOR_RESET);
	printf("    %s    %s    %s    %s    %s    %s    %s    %s    %s\n\n", BLKB, REDB, GRNB, YELB, BLUB, MAGB, CYNB, WHTB, COLOR_RESET);

	return 0;
}

int get_cpuinfo(struct cpuinfo* info) {
	const int max_file_size = 131072;

	FILE* file = fopen("/proc/cpuinfo", "r");
	char* buffer = malloc(max_file_size);
	fread(buffer, max_file_size, 1, file);
	fclose(file);

	char* name_str = strstr(buffer, "model name");
	char* processor_str = buffer;
	int cores = 1;
	int length = 0;
	if (name_str != NULL) {
		uint8_t quit = 0;
		while (!quit) {
			if (*name_str != ':')
				name_str++;
			else {
				name_str += 2;
				quit = 1;
			}
		}
		quit = 0;
		while (!quit) {
			if (name_str[length] != '\n')
				length++;
			else
				quit = 1;
		}
	} else {
		while (processor_str != NULL) {
			processor_str += 10;
			processor_str = strstr(processor_str, "processor");
			if (processor_str != NULL) {
				cores++;
			}
		}

		name_str = malloc(128);
		snprintf(name_str, 128, "%d Cores", cores);
		length = 128;
	}

	if (length >= 256) {
		fprintf(stderr, "%s[fsinfo] Error: CPU name too large%s\n", RED, COLOR_RESET);
		return 1;
	}

	memcpy(info->name, name_str, length);
	info->name[length] = 0;

	char max_freq[64];
	file = fopen("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", "r");
	fread(max_freq, 64, 1, file);
	fclose(file);

	float ghz = strtof(max_freq, NULL) / 1000000.0f;
	info->ghz = ghz;

	free(buffer);
	return 0;
}

int get_fsinfo(struct fsinfo* info) {
	const char* cwd = getcwd(NULL, 0);

	struct statvfs fs_stats;
	if (statvfs(cwd, &fs_stats) != 0) {
		fprintf(stderr, "%s[fsinfo] Error: statvfs failed%s\n", RED, COLOR_RESET);
		return 1;
	}

	info->free_gbs = ((float)fs_stats.f_bavail * (float)fs_stats.f_bsize) / 1000000000.0f;
	info->total_gbs = ((float)fs_stats.f_frsize * (float)fs_stats.f_blocks) / 1000000000.0f;

	return 0;
}

int get_meminfo(struct meminfo* info) {
	char* buffer = malloc(8192);
	FILE* file = fopen("/proc/meminfo", "r");
	fread(buffer, 8192, 1, file);
	fclose(file);

	char* total_str = strstr(buffer, "MemTotal:");
	int quit = 0;
	while (!quit) {
		if (*total_str != ':')
			total_str++;
		else {
			total_str++;
			quit = 1;
		}
	}

	quit = 0;
	while (!quit) {
		if (*total_str == ' ')
			total_str++;
		else
			quit = 1;
	}

	info->total_gbs = strtof(total_str, NULL) / 1000000.0f;

	char* free_str = strstr(buffer, "MemAvailable:");
	quit = 0;
	while (!quit) {
		if (*free_str != ':')
			free_str++;
		else {
			free_str++;
			quit = 1;
		}
	}

	quit = 0;
	while (!quit) {
		if (*free_str == ' ')
			free_str++;
		else
			quit = 1;
	}

	info->used_gbs = info->total_gbs - strtof(free_str, NULL) / 1000000.0f;

	free(buffer);
	return 0;
}
