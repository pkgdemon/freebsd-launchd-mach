/*
 * launchd_plist_scan.c — freebsd-launchd-mach (2026-05-16)
 *
 * In-launchd LaunchDaemons plist scanner. Replaces the earlier
 * fork+exec(launchctl bootstrap) bootstrap hack with the Apple-
 * faithful pattern: launchd itself reads /System/Library/LaunchDaemons/
 * + /Library/LaunchDaemons/ at PID-1 startup, parses each .plist via
 * libCoreFoundation, converts the CF tree to launch_data_t, and calls
 * job_import() directly (the same entry launchctl-driven loads use,
 * over Mach IPC).
 *
 * Apple's launchd does the same thing -- their launchd links CF, walks
 * the LaunchDaemons dirs at boot, parses plists, registers jobs. The
 * launchctl-bootstrap external command exists for re-bootstrapping
 * after launchd has started (rare). Initial PID-1 boot is all in-process.
 *
 * Functions ported (with adaptations) from launchctl.c:
 *   - CreateMyPropertyListFromFile (CF XML reader)
 *   - CF2launch_data                (CF tree -> launch_data_t convert)
 *   - myCFDictionaryApplyFunction  (CFDictionaryApply helper)
 * Logging is via launchd_syslog() instead of launchctl_log().
 *
 * Linker dependencies (Makefile): -lCoreFoundation -l_FoundationICU
 * Consumer flags: -D__HAS_APPLE_ICU__=1 -DU_DISABLE_RENAMING=1
 *                 -DAPPLE_ICU_CHANGES=1 -fblocks -fconstant-cfstrings
 */

#include "config.h"
#include "launchd.h"
#include "core.h"
#include "runtime.h"

#include "launchd_plist_scan.h"

#include <CoreFoundation/CoreFoundation.h>

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>

#include "launch.h"
#include "launch_priv.h"

/* --- ported helpers ----------------------------------------------------- */

static launch_data_t cf2launch_data(CFTypeRef cfr);

static void
cf_dict_apply_helper(const void *key, const void *value, void *context)
{
	launch_data_t ik = cf2launch_data(key);
	launch_data_t iv = cf2launch_data(value);
	launch_data_t where = (launch_data_t)context;

	if (ik == NULL || iv == NULL) {
		if (ik != NULL) launch_data_free(ik);
		if (iv != NULL) launch_data_free(iv);
		return;
	}

	launch_data_dict_insert(where, iv, launch_data_get_string(ik));
	launch_data_free(ik);
}

static launch_data_t
cf2launch_data(CFTypeRef cfr)
{
	launch_data_t r = NULL;
	CFTypeID cft = CFGetTypeID(cfr);

	if (cft == CFStringGetTypeID()) {
		char buf[4096];
		if (!CFStringGetCString(cfr, buf, sizeof(buf), kCFStringEncodingUTF8)) {
			return NULL;
		}
		r = launch_data_alloc(LAUNCH_DATA_STRING);
		launch_data_set_string(r, buf);
	} else if (cft == CFBooleanGetTypeID()) {
		r = launch_data_alloc(LAUNCH_DATA_BOOL);
		launch_data_set_bool(r, CFBooleanGetValue(cfr));
	} else if (cft == CFArrayGetTypeID()) {
		CFIndex i, ac = CFArrayGetCount(cfr);
		r = launch_data_alloc(LAUNCH_DATA_ARRAY);
		for (i = 0; i < ac; i++) {
			CFTypeRef v = CFArrayGetValueAtIndex(cfr, i);
			if (v != NULL) {
				launch_data_t iv = cf2launch_data(v);
				if (iv != NULL) {
					launch_data_array_set_index(r, iv, i);
				}
			}
		}
	} else if (cft == CFDictionaryGetTypeID()) {
		r = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
		CFDictionaryApplyFunction(cfr, cf_dict_apply_helper, r);
	} else if (cft == CFDataGetTypeID()) {
		r = launch_data_alloc(LAUNCH_DATA_OPAQUE);
		launch_data_set_opaque(r, CFDataGetBytePtr(cfr), CFDataGetLength(cfr));
	} else if (cft == CFNumberGetTypeID()) {
		long long n;
		double d;
		CFNumberType cfnt = CFNumberGetType(cfr);
		switch (cfnt) {
		case kCFNumberSInt8Type:  case kCFNumberSInt16Type:
		case kCFNumberSInt32Type: case kCFNumberSInt64Type:
		case kCFNumberCharType:   case kCFNumberShortType:
		case kCFNumberIntType:    case kCFNumberLongType:
		case kCFNumberLongLongType:
			CFNumberGetValue(cfr, kCFNumberLongLongType, &n);
			r = launch_data_alloc(LAUNCH_DATA_INTEGER);
			launch_data_set_integer(r, n);
			break;
		case kCFNumberFloat32Type: case kCFNumberFloat64Type:
		case kCFNumberFloatType:   case kCFNumberDoubleType:
			CFNumberGetValue(cfr, kCFNumberDoubleType, &d);
			r = launch_data_alloc(LAUNCH_DATA_REAL);
			launch_data_set_real(r, d);
			break;
		default:
			r = NULL;
			break;
		}
	}
	return r;
}

static CFPropertyListRef
create_plist_from_file(const char *posixfile)
{
	CFPropertyListRef propertyList = NULL;
	CFDataRef resourceData = NULL;
	SInt32 errorCode = 0;
	CFURLRef fileURL;

	fileURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault,
	    (const UInt8 *)posixfile, strlen(posixfile), false);
	if (fileURL == NULL) {
		launchd_syslog(LOG_ERR, "plist scan: CFURLCreateFromFileSystemRepresentation(%s) failed", posixfile);
		return NULL;
	}

	if (!CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault, fileURL,
	    &resourceData, NULL, NULL, &errorCode)) {
		launchd_syslog(LOG_ERR, "plist scan: CFURLCreateDataAndPropertiesFromResource(%s) failed: %d", posixfile, (int)errorCode);
		CFRelease(fileURL);
		return NULL;
	}

	propertyList = CFPropertyListCreateFromXMLData(kCFAllocatorDefault,
	    resourceData, kCFPropertyListMutableContainersAndLeaves, NULL);

	CFRelease(fileURL);
	if (resourceData != NULL) {
		CFRelease(resourceData);
	}
	return propertyList;
}

/* --- public API --------------------------------------------------------- */

/*
 * Load one plist file. Returns true on successful job_import.
 */
static bool
launchd_load_one_plist(const char *path)
{
	CFPropertyListRef plist = create_plist_from_file(path);
	if (plist == NULL) {
		launchd_syslog(LOG_ERR, "plist scan: failed to parse %s", path);
		return false;
	}

	if (CFGetTypeID(plist) != CFDictionaryGetTypeID()) {
		launchd_syslog(LOG_ERR, "plist scan: %s top-level is not a dictionary", path);
		CFRelease(plist);
		return false;
	}

	launch_data_t job = cf2launch_data(plist);
	CFRelease(plist);
	if (job == NULL) {
		launchd_syslog(LOG_ERR, "plist scan: %s CF->launch_data convert failed", path);
		return false;
	}

	job_t j = job_import(job);
	launch_data_free(job);
	if (j == NULL) {
		launchd_syslog(LOG_ERR, "plist scan: job_import failed for %s (errno=%d)", path, errno);
		return false;
	}

	launchd_syslog(LOG_NOTICE | LOG_CONSOLE,
	    "plist scan: loaded %s -> job_t %p", path, (void *)j);

	/*
	 * job_import -> jobmgr_import2 already calls job_dispatch(j, false)
	 * at the end, which evaluates keepalive (RunAtLoad sets start_pending,
	 * so keepalive returns true for RunAtLoad=true). Belt-and-suspenders:
	 * re-dispatch with kickstart=true and log the visible outcome so that
	 * PID-1 boot makes the start-attempt observable without raising the
	 * global syslog mask to DEBUG. struct job_s is opaque outside core.c
	 * so we can only check NULL/non-NULL here -- if "NULL" then the job
	 * was useless/removed; if "ok" the dispatch path was taken (which
	 * either kicked off, found already-active, or watched for keepalive).
	 */
	job_t r = job_dispatch(j, true);
	launchd_syslog(LOG_NOTICE | LOG_CONSOLE,
	    "plist scan: dispatch %s -> %s",
	    path, (r != NULL) ? "ok" : "NULL");
	return true;
}

/*
 * Scan one LaunchDaemons directory, load every .plist found inside.
 * Missing dir is silently ok; we walk both /System and /Library and
 * either may be absent in a minimal build.
 */
static void
launchd_scan_one_dir(const char *dir)
{
	DIR *d = opendir(dir);
	if (d == NULL) {
		if (errno != ENOENT) {
			launchd_syslog(LOG_NOTICE, "plist scan: opendir(%s) failed: %d", dir, errno);
		}
		return;
	}

	int loaded = 0;
	struct dirent *de;
	while ((de = readdir(d)) != NULL) {
		if (de->d_name[0] == '.') continue;
		size_t nlen = strlen(de->d_name);
		if (nlen < 7 || strcmp(de->d_name + nlen - 6, ".plist") != 0) {
			continue;
		}
		char path[1024];
		(void)snprintf(path, sizeof(path), "%s/%s", dir, de->d_name);
		if (launchd_load_one_plist(path)) {
			loaded++;
		}
	}
	closedir(d);
	launchd_syslog(LOG_NOTICE | LOG_CONSOLE,
	    "plist scan: %s -> %d job(s) loaded", dir, loaded);
}

void
launchd_scan_launchdaemons(void)
{
	/*
	 * Apple-canonical search paths, in priority order. /System owns
	 * the OS-provided daemons; /Library is the third-party slot.
	 * Per-user LaunchAgents (~/Library/LaunchAgents) are a per-user
	 * launchd concern, not PID-1.
	 */
	launchd_scan_one_dir("/System/Library/LaunchDaemons");
	launchd_scan_one_dir("/Library/LaunchDaemons");
}
