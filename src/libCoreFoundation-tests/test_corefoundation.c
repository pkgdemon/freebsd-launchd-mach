/*
 * test_corefoundation.c — Phase libCoreFoundation smoke check.
 *
 * Exercises the bits of CoreFoundation launchctl will actually use:
 *   - CF runtime alive (CFRetain / CFRelease, legacy non-Swift path)
 *   - CFDictionary + CFString basics
 *   - CFPropertyList XML round-trip (the primary feature)
 *
 * Prints COREFOUNDATION-OK on success, COREFOUNDATION-FAIL otherwise.
 * Exit 0 / 1 to match.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <CoreFoundation/CoreFoundation.h>

static int
fail(const char *msg)
{
	fprintf(stderr, "COREFOUNDATION-FAIL: %s\n", msg);
	return 1;
}

int
main(void)
{
	/*
	 * 1. CFDictionary + CFString basics.
	 *    Construct a small dict with a string value; read it back.
	 */
	CFMutableDictionaryRef d = CFDictionaryCreateMutable(
	    kCFAllocatorDefault, 0,
	    &kCFTypeDictionaryKeyCallBacks,
	    &kCFTypeDictionaryValueCallBacks);
	if (d == NULL)
		return fail("CFDictionaryCreateMutable returned NULL");

	CFStringRef key = CFSTR("Label");
	CFStringRef val = CFSTR("com.example.test");
	CFDictionarySetValue(d, key, val);

	CFStringRef got = (CFStringRef)CFDictionaryGetValue(d, key);
	if (got == NULL)
		return fail("CFDictionaryGetValue returned NULL");
	if (CFStringCompare(got, val, 0) != kCFCompareEqualTo)
		return fail("CFDictionaryGetValue returned wrong value");

	/*
	 * 2. Plist XML round-trip — serialize the dict, parse it back,
	 *    confirm the parsed dict has the same key+value.
	 */
	CFErrorRef err = NULL;
	CFDataRef xml = CFPropertyListCreateData(
	    kCFAllocatorDefault, (CFPropertyListRef)d,
	    kCFPropertyListXMLFormat_v1_0, 0, &err);
	if (xml == NULL) {
		if (err != NULL) {
			CFStringRef desc = CFErrorCopyDescription(err);
			char buf[256];
			CFStringGetCString(desc, buf, sizeof(buf),
			    kCFStringEncodingUTF8);
			fprintf(stderr, "CFPropertyListCreateData error: %s\n", buf);
			CFRelease(desc);
			CFRelease(err);
		}
		return fail("CFPropertyListCreateData (XML) returned NULL");
	}
	if (CFDataGetLength(xml) <= 0)
		return fail("XML plist data is empty");

	CFPropertyListRef parsed = CFPropertyListCreateWithData(
	    kCFAllocatorDefault, xml,
	    kCFPropertyListMutableContainersAndLeaves, NULL, &err);
	if (parsed == NULL)
		return fail("CFPropertyListCreateWithData (XML) returned NULL");
	if (CFGetTypeID(parsed) != CFDictionaryGetTypeID())
		return fail("Parsed XML plist isn't a dictionary");

	CFStringRef parsed_val = (CFStringRef)CFDictionaryGetValue(
	    (CFDictionaryRef)parsed, key);
	if (parsed_val == NULL)
		return fail("XML round-trip lost the Label key");
	if (CFStringCompare(parsed_val, val, 0) != kCFCompareEqualTo)
		return fail("XML round-trip mutated the Label value");

	CFRelease(parsed);
	CFRelease(xml);

	/*
	 * 3. Plist binary round-trip — same dict, different format.
	 */
	err = NULL;
	CFDataRef bin = CFPropertyListCreateData(
	    kCFAllocatorDefault, (CFPropertyListRef)d,
	    kCFPropertyListBinaryFormat_v1_0, 0, &err);
	if (bin == NULL)
		return fail("CFPropertyListCreateData (binary) returned NULL");
	if (CFDataGetLength(bin) <= 0)
		return fail("Binary plist data is empty");

	CFPropertyListRef bparsed = CFPropertyListCreateWithData(
	    kCFAllocatorDefault, bin,
	    kCFPropertyListMutableContainersAndLeaves, NULL, &err);
	if (bparsed == NULL)
		return fail("CFPropertyListCreateWithData (binary) returned NULL");
	if (CFGetTypeID(bparsed) != CFDictionaryGetTypeID())
		return fail("Parsed binary plist isn't a dictionary");

	CFStringRef bparsed_val = (CFStringRef)CFDictionaryGetValue(
	    (CFDictionaryRef)bparsed, key);
	if (bparsed_val == NULL)
		return fail("Binary round-trip lost the Label key");
	if (CFStringCompare(bparsed_val, val, 0) != kCFCompareEqualTo)
		return fail("Binary round-trip mutated the Label value");

	CFRelease(bparsed);
	CFRelease(bin);
	CFRelease(d);

	printf("COREFOUNDATION-OK: CFDictionary + XML/binary plist round-trip succeeded\n");
	return 0;
}
