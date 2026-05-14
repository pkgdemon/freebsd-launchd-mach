/*
 * servers/bootstrap.h — redirect to launchd-842's own bootstrap.h.
 *
 * On macOS, <servers/bootstrap.h> *is* launchd's liblaunch/bootstrap.h
 * — that's the install path. launchd-842's own headers
 * (bootstrap_priv.h, vproc_priv.h) do `#include <servers/bootstrap.h>`
 * expecting exactly that file.
 *
 * Our tree also carries a Phase-G stopgap at
 * src/libmach/include/servers/bootstrap.h — a hand-rolled subset
 * written for the standalone bootstrap_server before launchd-842 was
 * vendored. Its cmd_t / name_t typedefs differ from the real Apple
 * ones, so if it gets pulled into a launchd TU alongside launchd's
 * bootstrap.h the typedefs collide.
 *
 * liblaunch's Makefile puts -I${.CURDIR} (the liblaunch dir) ahead of
 * -I../../libmach/include, so this file wins the <servers/bootstrap.h>
 * lookup and forwards to launchd's canonical header. Both
 * `#include <servers/bootstrap.h>` and `#include "bootstrap.h"` then
 * resolve to the same guarded file — no redefinition.
 */
#include "../bootstrap.h"
